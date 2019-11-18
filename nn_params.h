#pragma once

// This file is linked into both LKM and training_output_formatter
// Including any library header file will likely result in error
//

// The input vector length of the NN
//
#define NUM_FEATURES 9

// The number of possible actions to take (which is also the output vector length of the NN)
//
#define NUM_ACTIONS 5

