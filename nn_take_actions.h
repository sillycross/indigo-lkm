#pragma once

// This file is linked into both LKM and training_output_formatter
// Including any library header file will likely result in error
//
 
#include "nn_params.h"

#define CWND_MIN_LIMIT 4
#define CWND_MAX_LIMIT 1000000

// Actions: ["*0.8", "-5", "0", "+5", "*1.25"]
//
static int WARN_UNUSED __take_action(int cwnd, int action)
{
    int new_cwnd;
    Assert(0 <= action && action < NUM_ACTIONS);
    switch (action)
    {
    case 0:
    {
        new_cwnd = cwnd - cwnd / 5;
        break;
    }
    case 1:
    {
        new_cwnd = cwnd - 5;
        break;
    }
    case 2:
    {
        new_cwnd = cwnd;
        break;
    }
    case 3:
    {
        new_cwnd = cwnd + 5;
        break;
    }
    case 4:
    {
        new_cwnd = cwnd + (cwnd >> 2);
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

