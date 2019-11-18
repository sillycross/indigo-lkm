#include "indigo_utils.h"

#include "indigo_nn.generated.h"
#include "nn_params.h"

// Common logic for the NN, used by both the SSE parts and the non-SSE parts
// Compiling with and without SSE generate two different versions,
// so all functions in this header must be static to avoid linking errors.
//

static void __setup_augmented_features(struct indigo_nn* nn, int last_action)
{
    float* input_vec;

    // [NUM_FEATURES - NUM_ACTIONS, NUM_FEATURES) of the input vector are "augmented features"
    // They are initialized to "1 if i = prev_action else 0"
    //
    Assert(0 <= last_action && last_action < NUM_ACTIONS);
    input_vec = indigo_nn_get_address_for_argument(nn, 0);
    memset(input_vec + NUM_FEATURES - NUM_ACTIONS, 0, sizeof(float) * NUM_ACTIONS);
    input_vec[NUM_FEATURES - NUM_ACTIONS + last_action] = 1.0;
}

