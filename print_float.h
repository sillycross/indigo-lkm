#pragma once

#include "indigo_utils.h"

#define FLOAT_LEN 9

// Serialize float 'value' to string 'buf' (for debug print purpose only).
// 'value' must be less than 10^(FLOAT_LEN-2). 'buf' must be at least FLOAT_LEN long.
//
void print_float(char* buf /*out*/, float value);
