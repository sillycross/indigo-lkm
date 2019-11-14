#include "nn_inference.h"
#include "indigo_utils.h"

#include "nn_common.private.h"

int WARN_UNUSED nn_init(struct indigo_nn* nn)
{
    float* lstm_state;

    // indigo_nn_constructor uses vmalloc which may yield CPU,
    // so this function MUST NOT be called inside kernel_fpu_begin/end section
    //
    if (!indigo_nn_constructor(nn))
    {
        return 0;
    }

    // Initialize augmented features.
    // The initial prev_action = NUM_ACTIONS - 1 (the *2.0 choice)
    //
    Assert(indigo_nn_get_argument_size_bytes(0) == (NUM_FEATURES + NUM_ACTIONS) * sizeof(float));
    __setup_augmented_features(nn, NUM_ACTIONS - 1);

    // Initialize LSTM state to 0
    //
    lstm_state = indigo_nn_get_address_for_argument(nn, 1);
    memset(lstm_state, 0, indigo_nn_get_argument_size_bytes(1));
    return 1;
}

