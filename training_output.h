#pragma once

#include "indigo_utils.h"

#ifndef INDIGO_PRODUCTION
// If uncommented, generate training outputs.
// WARNING: This module must be used single-threadedly (only one socket opened at any time) if this is uncommented.
//
#define GEN_TRAINING_OUTPUTS
#endif  // INDIGO_PRODUCTION

// Constructor for Indigo training output file
//
int WARN_UNUSED indigo_training_output_constructor(void);

// Write into the Indigo training output file
//
void indigo_training_output_write(const char* format, ...);

// Destructor for Indigo training output file
//
void indigo_training_output_destructor(void);
