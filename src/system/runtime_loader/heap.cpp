/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "runtime_loader_private.h"

#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <locks.h>
#include <syscalls.h>

#include <util/SimpleAllocator.h>


#if __cplusplus >= 201103L
#include <cstddef>
const static size_t kAlignment = alignof(max_align_t);
#else
const static size_t kAlignment = 8;
#endif
	// all memory chunks will be a multiple of this

const static size_t kInitialHeapSize = 64 * 1024;
const static size_t kHeapGrowthAlignment = 32 * 1024;

static const char* const kLockName = "runtime_loader heap";
static recursive_lock sLock = RECURSIVE_LOCK_INITIALIZER(kLockName);

static SimpleAllocator<kAlignment> sAllocator;


//	#pragma mark -


static status_t
add_area(size_t size)
{
	void* base;
	area_id area = _kern_create_area("rld heap", &base,
		B_RANDOMIZED_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < 0)
		return area;

	sAllocator.AddChunk(base, size);
	return B_OK;
}


static status_t
grow_heap(size_t bytes)
{
	return add_area(sAllocator.Align(kAlignment + bytes, kHeapGrowthAlignment));
}


//	#pragma mark -


status_t
heap_init()
{
	return add_area(kInitialHeapSize);
}


status_t
heap_reinit_after_fork()
{
	recursive_lock_init(&sLock, kLockName);
	return B_OK;
}


void*
malloc(size_t size)
{
	if (size == 0)
		return NULL;

	RecursiveLocker _(sLock);

	void* allocated = sAllocator.Allocate(size);
	if (allocated == NULL) {
		// try to enlarge heap
		if (grow_heap(size) != B_OK)
			return NULL;

		allocated = sAllocator.Allocate(size);
		if (allocated == NULL) {
			TRACE(("no allocation chunk found after growing the heap\n"));
			return NULL;
		}
	}

	TRACE(("malloc(%lu) -> %p\n", size, allocatedAddress));
	return allocated;
}


void*
realloc(void* oldBuffer, size_t newSize)
{
	RecursiveLocker _(sLock);

	void* newBuffer = sAllocator.Reallocate(oldBuffer, newSize);
	if (oldBuffer == newBuffer) {
		TRACE(("realloc(%p, %lu) old buffer is large enough\n",
			oldBuffer, newSize));
	} else {
		TRACE(("realloc(%p, %lu) -> %p\n", oldBuffer, newSize, newBuffer));
	}
	return newBuffer;
}


void*
calloc(size_t numElements, size_t size)
{
	void* address = malloc(numElements * size);
	if (address != NULL)
		memset(address, 0, numElements * size);

	return address;
}


void
free(void* allocated)
{
	if (allocated == NULL)
		return;

	RecursiveLocker _(sLock);

	TRACE(("free(%p)\n", allocated));

	sAllocator.Free(allocated);
}
