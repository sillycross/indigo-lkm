#include "nn_inference.h"
#include "indigo_utils.h"
#include "training_output.h"

#include "nn_common.private.h"
#include "nn_take_actions.h"

// The input feature normalization factors (from the Python Indigo).
//
static const float x_normalize_factor[4] =
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
    for (i = 0; i < NUM_FEATURES; i++)
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
