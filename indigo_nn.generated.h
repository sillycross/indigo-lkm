// Generated, DO NOT EDIT!
//

#pragma once
#ifndef FOR_SANITY_CHECKER
#include "indigo_utils.h"
#endif

void __xla___graph(
	void* result, const void* run_options,
	const void** args, void** temps, void* profile_counters);

// Buffer Layout Information
//
static const int ____indigo_nn_x_numBuffers = 19;
static const int ____indigo_nn_x_bufferOffsets[19] = {
	-1, -1, 0, -1, -1, -1, -1, -1, -1, -1, 256, 512, 576, 640, -1, -1, -1, -1, 704
};
static const int ____indigo_nn_x_bufferSizes[19] = {
	-1, -1, 256, -1, -1, -1, -1, -1, -1, -1, 256, 36, 20, 16, -1, -1, -1, -1, 2232
};
static const int ____indigo_nn_x_bufferLength = 2944;

// Parameters Layout Information
//
static const int ____indigo_nn_x_numArgs = 2;
static const int ____indigo_nn_x_argIndexes[2] = {
	11, 10
};
static const int ____indigo_nn_x_numResults = 2;
static const int ____indigo_nn_x_resultIndexArrayIndex = 13;

// Default runtime options
//
static const u64 ____indigo_nn_x_runtimeOptions[8] = {
	0, 4294967295, 0, 0, 0, 0, 0, 0
};

struct indigo_nn {
	void** m_bufferList;
};

// Member functions to set parameters and retrieve results
// TODO: We currently assume all parameter types are floats
//
static inline float* indigo_nn_get_address_for_argument(struct indigo_nn* nn, int index) {
	Assert(0 <= index && index < ____indigo_nn_x_numArgs && nn->m_bufferList[____indigo_nn_x_argIndexes[index]] != NULL);
	return (float*)(nn->m_bufferList[____indigo_nn_x_argIndexes[index]]);
}

static inline int indigo_nn_get_argument_size_bytes(int index) {
	Assert(0 <= index && index < ____indigo_nn_x_numArgs && ____indigo_nn_x_bufferSizes[____indigo_nn_x_argIndexes[index]] != -1);
	return ____indigo_nn_x_bufferSizes[____indigo_nn_x_argIndexes[index]];
}

static inline float* indigo_nn_get_address_for_result(struct indigo_nn* nn, int index) {
	Assert(0 <= index && index < ____indigo_nn_x_numResults);
	return ((float**)(nn->m_bufferList[____indigo_nn_x_resultIndexArrayIndex]))[index];
}

// Member functions to construct, destruct and run the NN
//
static inline int WARN_UNUSED indigo_nn_constructor(struct indigo_nn* nn) {
	int i;
	uintptr_t start;
	nn->m_bufferList = (void**)kmalloc(____indigo_nn_x_bufferLength + 216, GFP_NOWAIT);
	if (nn->m_bufferList == NULL) { return 0; }
	start = ((uintptr_t)(nn->m_bufferList)) + 152;
	start = (start + 63) >> 6 << 6;
	for (i = 0; i < ____indigo_nn_x_numBuffers; i++) {
		if(____indigo_nn_x_bufferOffsets[i] == -1) {
			nn->m_bufferList[i] = NULL;
		} else {
			nn->m_bufferList[i] = ((char*)start) + ____indigo_nn_x_bufferOffsets[i];
		}
	}
	return 1;
}

static inline void indigo_nn_destructor(struct indigo_nn* nn) {
	Assert(nn->m_bufferList != NULL);
	kfree(nn->m_bufferList);
	nn->m_bufferList = NULL;
}

static inline void indigo_nn_run(struct indigo_nn* nn) {
	Assert(nn->m_bufferList != NULL);
	function_call_5_params_respecting_stack_alignment(
		(void*)__xla___graph,
		(u64)(nn->m_bufferList[____indigo_nn_x_resultIndexArrayIndex]),
		(u64)(____indigo_nn_x_runtimeOptions),
		(u64)(NULL),
		(u64)(nn->m_bufferList),
		(u64)(NULL));
}
