#include "indigo_utils.h"

#include "indigo_nn.generated.h"
#include "nn_inference.h"
#include "training_output.h"

#include <linux/module.h>
#include <net/tcp.h>
#include <linux/inet_diag.h>
#include <linux/inet.h>
#include <linux/random.h>
#include <linux/win_minmax.h>
#include <asm-generic/div64.h>
#include <asm-generic/int-ll64.h>
#include <asm/fpu/api.h>

// The interval we make decisions
//
#define INDIGO_DECISION_INTERVAL_US 10000

static u32 g_socket_id = 0;

struct indigo_state
{
    u8 m_delay_ewma_initialized;
    u8 m_send_rate_ewma_initialized;
    u8 m_rtt_info_initialized;
    u8 m_deliver_rate_ewma_initialized;
    u8 m_decision_timestamp_initialized;
    u8 m_first_timestamp_initialized;

    // The unique socket ordinal of this socket, for identification purpose in training output
    //
    u32 m_socket_id;

    // The packet # that signals the end of this RTT
    //
    u32 m_end_of_rtt;
    // The number of bytes delivered before the start of this RTT
    //
    u64 m_bytes_delivered_start_of_rtt;
    // The timestamp at the start of this RTT
    //
    u64 m_timestamp_at_start_of_rtt;

    // The deliver rate (bytes delivered this rtt / rtt) ewma, scaled up by 16k
    //
    u64 m_deliver_rate_ewma_sf16k;

    // Information to update the delay (rtt - min_rtt) ewma
    //
    u32 m_min_rtt_us;
    u32 m_delay_ewma;

    // The send rate (bytes on flight / rtt) ewma, scaled up by 16k
    //
    u64 m_send_rate_ewma_sf16k;

    // The timestamp at which we will do the next decision
    //
    u64 m_next_decision_timestamp;

    // The timestamp that the congestion control algorithm is first called
    //
    u64 m_first_timestamp;

    // The neural network
    //
    struct indigo_nn nn;
};

static void __update_initial_timestamp(struct indigo_state* indigo, u64 timestamp_us)
{
    if (!indigo->m_first_timestamp_initialized)
    {
        indigo->m_first_timestamp_initialized = 1;
        indigo->m_first_timestamp = timestamp_us;
    }
}

static void __update_delay_ewma(struct indigo_state* indigo, u32 rtt_us)
{
    u32 delay;
    if (!indigo->m_delay_ewma_initialized || rtt_us < indigo->m_min_rtt_us)
    {
        indigo->m_min_rtt_us = rtt_us;
    }
    delay = rtt_us - indigo->m_min_rtt_us;
    if (!indigo->m_delay_ewma_initialized)
    {
        // Seems stupid since it's always 0.. but just copy the logic from Indigo
        //
        indigo->m_delay_ewma = delay;
        indigo->m_delay_ewma_initialized = 1;
    }
    else
    {
        indigo->m_delay_ewma = ((indigo->m_delay_ewma * 7) >> 3) + (delay >> 3);
    }
}

static void __update_send_rate_ewma(struct indigo_state* indigo, u32 rtt_us, u32 bytes_in_flight)
{
    u64 send_rate_sf16k = div_u64(((u64)bytes_in_flight) << 14, rtt_us);
    if (!indigo->m_send_rate_ewma_initialized)
    {
        indigo->m_send_rate_ewma_sf16k = send_rate_sf16k;
        indigo->m_send_rate_ewma_initialized = 1;
    }
    else
    {
        indigo->m_send_rate_ewma_sf16k = ((indigo->m_send_rate_ewma_sf16k * 7) >> 3) + (send_rate_sf16k >> 3);
    }
}

static void __update_deliver_rate_ewma(struct indigo_state* indigo,
                                       struct tcp_sock* tp,
                                       const struct rate_sample* rs)
{
    u64 rtt_interval_us;
    u32 bytes_delivered_this_rtt;
    u64 deliver_rate_sf16k;

    if (!indigo->m_rtt_info_initialized || !before(rs->prior_delivered, indigo->m_end_of_rtt))
    {
        if (indigo->m_rtt_info_initialized)
        {
            rtt_interval_us = tp->tcp_mstamp - indigo->m_timestamp_at_start_of_rtt;
            bytes_delivered_this_rtt = tp->snd_una - indigo->m_bytes_delivered_start_of_rtt;
            deliver_rate_sf16k = div_u64(((u64)bytes_delivered_this_rtt) << 14, rtt_interval_us);
            if (!indigo->m_deliver_rate_ewma_initialized)
            {
                indigo->m_deliver_rate_ewma_sf16k = deliver_rate_sf16k;
                indigo->m_deliver_rate_ewma_initialized = 1;
            }
            else
            {
                indigo->m_deliver_rate_ewma_sf16k = ((indigo->m_deliver_rate_ewma_sf16k * 7) >> 3) + (deliver_rate_sf16k >> 3);
            }
        }
        indigo->m_end_of_rtt = tp->delivered;
        indigo->m_timestamp_at_start_of_rtt = tp->tcp_mstamp;
        indigo->m_bytes_delivered_start_of_rtt = tp->snd_una;
        indigo->m_rtt_info_initialized = 1;
    }
}

static void __update_cwnd_if_needed(struct indigo_state* indigo,
                                    struct tcp_sock* tp)
{
    u64 cur_timestamp_us = tp->tcp_mstamp;
    struct nn_training_log_info log_info;
    struct nn_input_features features;

    // If we have not made a decision but we have enough knowledge
    // to make the first decision, then make the first decision
    //
    if (!indigo->m_decision_timestamp_initialized &&
            indigo->m_delay_ewma_initialized &&
            indigo->m_send_rate_ewma_initialized &&
            indigo->m_deliver_rate_ewma_initialized)
    {
        indigo->m_decision_timestamp_initialized = 1;
        indigo->m_next_decision_timestamp = cur_timestamp_us;
    }

    // Make the decision if the timestamp has reached the decision point
    //
    if (indigo->m_decision_timestamp_initialized && cur_timestamp_us >= indigo->m_next_decision_timestamp)
    {
        log_info.socket_id = indigo->m_socket_id;
        log_info.timestamp = cur_timestamp_us - indigo->m_first_timestamp;
        features.delay_ewma = indigo->m_delay_ewma;
        features.send_rate_ewma_sf16k = indigo->m_send_rate_ewma_sf16k;
        features.deliver_rate_ewma_sf16k = indigo->m_deliver_rate_ewma_sf16k;
        features.cur_cwnd = tp->snd_cwnd;

        TRACE_DEBUG_VERBOSE("[Indigo Socket %p] Input features: delay_ewma = %llu send_rate_ewma = %llu deliver_rate_ewma = %llu cur_cwnd = %u",
                            tp, features.delay_ewma, features.send_rate_ewma_sf16k, features.deliver_rate_ewma_sf16k, features.cur_cwnd);

        kernel_fpu_begin();
        tp->snd_cwnd = nn_inference(&log_info, &indigo->nn, &features, tp->snd_cwnd);
        kernel_fpu_end();

        indigo->m_next_decision_timestamp = cur_timestamp_us + INDIGO_DECISION_INTERVAL_US;
    }
}

static void indigo_main(struct sock *sk, const struct rate_sample* rs)
{
    struct tcp_sock* tp = tcp_sk(sk);
    struct indigo_state* indigo = inet_csk_ca(sk);

    if (rs->delivered < 0 || rs->interval_us <= 0)
    {
        return; /* Not a valid observation */
    }

    __update_initial_timestamp(indigo, tp->tcp_mstamp);
    __update_delay_ewma(indigo, rs->interval_us);
    __update_send_rate_ewma(indigo, rs->interval_us, tp->snd_nxt - tp->snd_una);
    __update_deliver_rate_ewma(indigo, tp, rs);
    __update_cwnd_if_needed(indigo, tp);

    /*
    TRACE_DEBUG_VERBOSE("[Indigo Socket %p] delay_ewma = %u send_rate_ewma = %llu deliver_rate_ewma = %llu",
                        sk, indigo->m_delay_ewma, indigo->m_send_rate_ewma_sf16k, indigo->m_deliver_rate_ewma_sf16k);

    TRACE_DEBUG_VERBOSE("[Indigo Socket %p] Received rate sample %llu prior_delivered=%u "
                        "prior_in_flight=%d interval_us=%lld delivered=%d RTT=%lld losses=%d"
                        "bytes_in_flight=%d",
                        sk, (unsigned long long)rs->prior_mstamp, rs->prior_delivered, rs->prior_in_flight,
                        (long long)rs->interval_us, (int)rs->delivered, (long long)rs->rtt_us, rs->losses,
                        tp->snd_nxt-tp->snd_una);
    */
}

static void indigo_init(struct sock *sk)
{
    struct tcp_sock* tp = tcp_sk(sk);
    struct indigo_state* indigo = inet_csk_ca(sk);

    TRACE_DEBUG("[Indigo Socket %p] Initializing socket using Indigo..", sk);

    // Assign the unique socket id to indigo_state
    //
    indigo->m_socket_id = __sync_fetch_and_add(&g_socket_id, 1);

    // Set the initial cwnd to 5
    //
    tp->snd_cwnd = 5;

    // Initialize the NN
    //
    if (!nn_init(&indigo->nn))
    {
        TRACE_INFO("Failed to initialize NN: OOM");
        // TODO: init cannot fail.. We need to record this failure in indigo_state
        //
        return;
    }
    TRACE_DEBUG("Indigo initialization completed.");
}

static void indigo_release(struct sock *sk)
{
    struct indigo_state* indigo = inet_csk_ca(sk);
    if (indigo->nn.m_bufferList != NULL)
    {
        indigo_nn_destructor(&indigo->nn);
        TRACE_DEBUG("[Indigo Socket %p] Socket cleanup successfully", sk);
    }
}

// When TCP detects packet loss, it reduces window size. However the packet loss could be proven
// to be false later. It then calls this function to undo the operation.
//
static u32 indigo_undo_cwnd(struct sock *sk)
{
    // Indigo does not specially respond to losses
    //
    return tcp_sk(sk)->snd_cwnd;
}

static u32 indigo_ssthresh(struct sock *sk)
{
    // Indigo does not use ssthresh threshold
    //
    return TCP_INFINITE_SSTHRESH;
}

static struct tcp_congestion_ops tcp_indigo_cong_ops __read_mostly = {
	.flags		= TCP_CONG_NON_RESTRICTED,
    .name		= "tcp_tron",
	.owner		= THIS_MODULE,
    .init		= indigo_init,
    .release    = indigo_release,
    .cong_control	= indigo_main,
    .undo_cwnd	= indigo_undo_cwnd,
    .ssthresh	= indigo_ssthresh,
};

static int __init indigo_register(void)
{
    BUILD_BUG_ON(sizeof(struct indigo_state) > ICSK_CA_PRIV_SIZE);
    if (!indigo_training_output_constructor())
    {
        return -EINVAL;
    }
    TRACE_DEBUG("Registering Indigo Kernel Module..");
    return tcp_register_congestion_control(&tcp_indigo_cong_ops);
}

static void __exit indigo_unregister(void)
{
    indigo_training_output_destructor();
    TRACE_DEBUG("Unregistering Indigo Kernel Module..");
    tcp_unregister_congestion_control(&tcp_indigo_cong_ops);
}

module_init(indigo_register);
module_exit(indigo_unregister);

MODULE_AUTHOR("X");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("X");
 
