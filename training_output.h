#pragma once

#include "indigo_utils.h"

#ifndef INDIGO_PRODUCTION
// If uncommented, generate training outputs.
//
#define GEN_TRAINING_OUTPUTS
#endif  // INDIGO_PRODUCTION

// Constructor for Indigo training output file
//
int WARN_UNUSED indigo_training_output_constructor(void);

// Take the spinlock to write the output file
//
void indigo_training_output_get_lock(void);

// Write into the Indigo training output file
//
void indigo_training_output_write(const char* format, ...);

// Unlock after writing the output file
//
void indigo_training_output_unlock(void);

// Destructor for Indigo training output file
//
void indigo_training_output_destructor(void);
