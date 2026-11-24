/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "runtime_loader_private.h"

#include <malloc.h>
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
const static size_t kHeapReservationSize = 1 * 1024 * 1024;

static const char* const kLockName = "runtime_loader heap";
static recursive_lock sLock = RECURSIVE_LOCK_INITIALIZER(kLockName);

static void* sLastHeapBase;
static size_t sLastHeapAreaSize;
static area_id sLastHeapArea;

static SimpleAllocator<kAlignment> sAllocator;


//	#pragma mark -


static status_t
add_area(size_t size)
{
	if (sLastHeapAreaSize != 0) {
		// Try to resize the previous area instead of creating a new one.
		status_t status = _kern_resize_area(sLastHeapArea, sLastHeapAreaSize + size);
		if (status == B_OK) {
			sAllocator.AddChunk((uint8*)sLastHeapBase + sLastHeapAreaSize, size);
			sLastHeapAreaSize += size;
			return B_OK;
		}
	}

	void* reservedBase;
	status_t status = _kern_reserve_address_range((addr_t*)&reservedBase, 
		B_RANDOMIZED_ANY_ADDRESS, kHeapReservationSize);
	if (status != B_OK)
		return status;

	void* base = reservedBase;
	area_id area = _kern_create_area("rld heap", &base,
		B_EXACT_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (area < 0) {
		_kern_unreserve_address_range((addr_t)reservedBase, kHeapReservationSize);
		return area;
	}

	sLastHeapBase = base;
	sLastHeapArea = area;
	sLastHeapAreaSize = size;

	sAllocator.AddChunk(base, size);
	return B_OK;
}


static status_t
grow_heap(size_t bytes)
{
	// Add kAlignment so that the heap has enough space for bookkeeping data.
	return add_area(sAllocator.Align(bytes + kAlignment, kHeapGrowthAlignment));
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
	sLastHeapArea = _kern_area_for(sLastHeapBase);
	return B_OK;
}


void*
malloc(size_t size)
{
	return memalign(kAlignment, size);
}


void*
memalign(size_t alignment, size_t size)
{
	if (size == 0)
		return NULL;
	if (alignment <= kAlignment)
		alignment = 0;

	RecursiveLocker _(sLock);

	void* allocated = sAllocator.Allocate(size, alignment);
	if (allocated == NULL) {
		// try to enlarge heap
		if (grow_heap(size + alignment) != B_OK)
			return NULL;

		allocated = sAllocator.Allocate(size, alignment);
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
