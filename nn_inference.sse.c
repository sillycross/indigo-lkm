#include "nn_inference.h"
#include "indigo_utils.h"
#include "training_output.h"

#include "nn_common.private.h"

// The input feature normalization factors (from the Python Indigo).
//
static const float x_normalize_factor[NUM_FEATURES] =
{
    // The additional 1000 is because Python Indigo expects ms, we have us
    //
    1.0f / (200.0 * 1000),
    // The (1<<14) is because we scaled up by 16K.
    // The 1000 is the ms-to-us conversion.
    // The 0.008 is the additional constant factor used in Python Indigo.
    //
    (1000.0f * 0.008) / (200.0 * (1 << 14)),
    (1000.0f * 0.008) / (200.0 * (1 << 14)),
    // No special modifier for this one
    //
    1.0f / 5000
};

static void __normalize_input_features(struct indigo_nn* nn,
                                       const struct nn_input_features* input_features)
{
    float* input_vec = indigo_nn_get_address_for_argument(nn, 0);
    input_vec[0] = input_features->delay_ewma * x_normalize_factor[0];
    input_vec[1] = input_features->deliver_rate_ewma_sf16k * x_normalize_factor[1];
    input_vec[2] = input_features->send_rate_ewma_sf16k * x_normalize_factor[2];
    input_vec[3] = input_features->cur_cwnd * x_normalize_factor[3];
}

#define CWND_MIN_LIMIT 4
#define CWND_MAX_LIMIT 1000000

// Actions: ["/2.0", "-10.0", "+0.0", "+10.0", "*2.0"]
//
static int WARN_UNUSED __take_action(int cwnd, int action)
{
    int new_cwnd;
    Assert(0 <= action && action < NUM_ACTIONS);
    switch (action)
    {
    case 0:
    {
        new_cwnd = cwnd >> 1;
        break;
    }
    case 1:
    {
        new_cwnd = cwnd - 10;
        break;
    }
    case 2:
    {
        new_cwnd = cwnd;
        break;
    }
    case 3:
    {
        new_cwnd = cwnd + 10;
        break;
    }
    case 4:
    {
        new_cwnd = cwnd * 2;
        break;
    }
    default:
    {
        Assert(0);
        __builtin_unreachable();
    }
    }
    if (new_cwnd < CWND_MIN_LIMIT)
    {
        TRACE_DEBUG("NN decision attempted to reduce cwnd from %d to %d. Clamped cwnd to %d.",
                    cwnd, new_cwnd, CWND_MIN_LIMIT);
        new_cwnd = CWND_MIN_LIMIT;
    }
    if (new_cwnd > CWND_MAX_LIMIT)
    {
        TRACE_DEBUG("NN decision attempted to raise cwnd from %d to %d. Clamped cwnd to %d.",
                     cwnd, new_cwnd, CWND_MAX_LIMIT);
        new_cwnd = CWND_MAX_LIMIT;
    }
    TRACE_DEBUG_VERBOSE("NN decision: cwnd from %d to %d", cwnd, new_cwnd);
    return new_cwnd;
}

int WARN_UNUSED nn_inference(struct nn_training_log_info* log_info,
                             struct indigo_nn* nn,
                             const struct nn_input_features* input_features,
                             int cwnd)
{
    float* input_vec = indigo_nn_get_address_for_argument(nn, 0);
    float* output_vec;
    float* lstm_state_out;
    float* lstm_state_in;
    float max_prob;
    int action_to_take;
    int i;

    // Setup input features. The augmented features has been initialized at last invocation
    //
    __normalize_input_features(nn, input_features);
    indigo_nn_run(nn);

    // The action to take is argmax(action_probs)
    //
    output_vec = indigo_nn_get_address_for_result(nn, 0);
    for (i = 0; i < NUM_ACTIONS; i++)
    {
        if (i == 0 || output_vec[i] > max_prob)
        {
            max_prob = output_vec[i];
            action_to_take = i;
        }
    }

#ifdef GEN_TRAINING_OUTPUTS
    indigo_training_output_get_lock();
    // Log the training output
    // Line format: timestamp, cur_cwnd, action_taken, input_vector
    //
    indigo_training_output_write("%llu %llu %d %d",
                                 log_info->socket_id, log_info->timestamp, cwnd, action_to_take);
    for (i = 0; i < NUM_FEATURES + NUM_ACTIONS; i++)
    {
        // Log the floats as binary values because we cannot print float easily in kernel.
        // We can parse them back to float when we process the file.
        //
        u32* as_u32 = (u32*)(input_vec + i);
        indigo_training_output_write(" %u", *as_u32);
    }
    indigo_training_output_write("\n");
    indigo_training_output_unlock();
#endif

    // Setup LSTM state for next inference
    //
    lstm_state_out = indigo_nn_get_address_for_result(nn, 1);
    lstm_state_in = indigo_nn_get_address_for_argument(nn, 1);
    memcpy(lstm_state_in, lstm_state_out, indigo_nn_get_argument_size_bytes(1));

    // Setup augmented features for next inference
    //
    __setup_augmented_features(nn, action_to_take);

    // Return the new congestion window size
    //
    return __take_action(cwnd, action_to_take);
}
