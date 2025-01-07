/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <OS.h>

#include <util/SimpleAllocator.h>


static SimpleAllocator<> sAllocator;
const int32	kHeapSize = (1024 + 512) * 1024;


static void*
test_malloc(size_t bytes)
{
	return sAllocator.Allocate(bytes);
}


static void*
test_realloc(void* oldBuffer, size_t size)
{
	return sAllocator.Reallocate(oldBuffer, size);
}


static void
test_free(void* buffer)
{
	sAllocator.Free(buffer);
}


enum {
	OP_INVALID = 0,
	OP_MALLOC,
	OP_REALLOC,
	OP_FREE,
};
struct AllocationOperation {
	uint8 operation;
	// uint8 _unused[3];
	uint32 value;
};
#include "heap_allocation_operations.h"


int
main(int argc, char* argv[])
{
	void* base = malloc(kHeapSize);
	if (base == NULL) {
		fprintf(stderr, "Could not initialize heap.\n");
		return -1;
	}
	sAllocator.AddChunk(base, kHeapSize);

	void** allocations = (void**)malloc(kAllocationsCount * sizeof(void*));
	int allocationCounter = 0;

	const bigtime_t start = system_time();
	for (size_t i = 0; i < B_COUNT_OF(kOperations); i++) {
		switch (kOperations[i].operation) {
			case OP_MALLOC: {
				int index = allocationCounter++;
				allocations[index] = test_malloc(kOperations[i].value);
				break;
			}
			case OP_REALLOC: {
				int index = kOperations[i++].value;
				int newSize = kOperations[i].value;
				allocations[index] = test_realloc(allocations[index], newSize);
				break;
			}
			case OP_FREE: {
				int index = kOperations[i].value;
				test_free(allocations[index]);
				allocations[index] = NULL;
				break;
			}
		}
	}
	printf("completed %" B_PRIuSIZE " operations in %" B_PRIdBIGTIME " usec\n",
		B_COUNT_OF(kOperations), system_time() - start);

	free(base);
	return 0;
}
