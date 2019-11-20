#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define u64 unsigned long long
 
#define GFP_NOWAIT 0
#define WARN_UNUSED
#define Assert assert

static void* kmalloc(int len, int dummy)
{
	return malloc(len);
}

static void kfree(void* ptr)
{
	return free(ptr);
}

typedef void (*fn_type)(u64, u64, u64, u64, u64);

static void function_call_5_params_respecting_stack_alignment(
        void* fn, u64 param1, u64 param2, u64 param3, u64 param4, u64 param5)
{
	((fn_type)fn)(param1, param2, param3, param4, param5);
}

#define FOR_SANITY_CHECKER

#include "../indigo_nn.generated.h"
#include "../nn_params.h"

int main()
{
	struct indigo_nn nn;
	int i;
	float* ptr;
	
	if (!indigo_nn_constructor(&nn))
	{
		assert(!"malloc OOM");
	}
	assert(NUM_FEATURES * sizeof(float) == indigo_nn_get_argument_size_bytes(0));
	ptr = indigo_nn_get_address_for_argument(&nn, 0);
	for (i = 0; i < NUM_FEATURES; i++)
	{
		ptr[i] = 0.05 * i + 0.01;
	}
	
	ptr = indigo_nn_get_address_for_argument(&nn, 1);
	memset(ptr, 0, indigo_nn_get_argument_size_bytes(1));
	
	indigo_nn_run(&nn);
	
	ptr = indigo_nn_get_address_for_result(&nn, 0);
	for (i = 0; i < NUM_ACTIONS; i++)
	{
		printf("%.8e ", ptr[i]);
	}
	printf("\n");
	
	indigo_nn_destructor(&nn);
	return 0;
}

