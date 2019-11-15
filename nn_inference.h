#pragma once

#include "indigo_utils.h"

struct indigo_nn;

// Information needed for nn_inference to write training log
//
struct nn_training_log_info
{
    u64 socket_id;
    u64 timestamp;
};

struct nn_input_features
{
    u64 delay_ewma;
    u64 send_rate_ewma_sf16k;
    u64 deliver_rate_ewma_sf16k;
    u64 cur_cwnd;
};

// Initialize the NN. Must NOT be called in kernel_fpu_begin/end section
//
int WARN_UNUSED nn_init(struct indigo_nn* nn);

// Execute one inference on given input features, returns new cwnd
// Must be called in kernel_fpu_begin/end section
//
int WARN_UNUSED nn_inference(struct nn_training_log_info* log_info,
                             struct indigo_nn* nn,
                             const struct nn_input_features* input_features,
                             int cwnd);
