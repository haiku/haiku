/*
 * Copyright 2003-2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2005-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <boot/heap.h>

#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include <boot/platform.h>
#include <util/OpenHashTable.h>


#if KDEBUG
#define DEBUG_ALLOCATIONS
	// if defined, allocated memory is filled with 0xcc and freed memory with 0xdeadbeef
#define DEBUG_MAX_HEAP_USAGE
	// if defined, the maximum heap usage is determined and printed before
	// entering the kernel
#define DEBUG_VALIDATE_HEAP_ON_FREE
	// if defined, the heap integrity is checked on every free
	// (must have DEBUG_MAX_HEAP_USAGE defined also)
#endif

#include <util/SimpleAllocator.h>


//#define TRACE_HEAP
#ifdef TRACE_HEAP
#	define TRACE(format...)	dprintf(format)
#else
#	define TRACE(format...)	do { } while (false)
#endif


const static size_t kAlignment = 8;
	// all memory chunks will be a multiple of this

const static size_t kDefaultHeapSize = (1024 + 512) * 1024;
	// default initial heap size, unless overridden by platform loader
const static size_t kLargeAllocationThreshold = 128 * 1024;
	// allocations of this size or larger are allocated separately


struct LargeAllocation {
	LargeAllocation()
	{
		fAddress = NULL;
		fSize = 0;
	}

	status_t Allocate(size_t size)
	{
		ssize_t actualSize = platform_allocate_heap_region(size, &fAddress);
		if (actualSize < 0)
			return actualSize;

		fSize = actualSize;
		return B_OK;
	}

	void Free()
	{
		platform_free_heap_region(fAddress, fSize);
	}

	void* Address() const
	{
		return fAddress;
	}

	size_t Size() const
	{
		return fSize;
	}

	LargeAllocation*& HashNext()
	{
		return fHashNext;
	}

private:
	void*				fAddress;
	size_t				fSize;
	LargeAllocation*	fHashNext;
};


struct LargeAllocationHashDefinition {
	typedef void*			KeyType;
	typedef	LargeAllocation	ValueType;

	size_t HashKey(void* key) const
	{
		return size_t(key) / kAlignment;
	}

	size_t Hash(LargeAllocation* value) const
	{
		return HashKey(value->Address());
	}

	bool Compare(void* key, LargeAllocation* value) const
	{
		return value->Address() == key;
	}

	LargeAllocation*& GetLink(LargeAllocation* value) const
	{
		return value->HashNext();
	}
};


typedef BOpenHashTable<LargeAllocationHashDefinition> LargeAllocationHashTable;


static void* sHeapBase;
static void* sHeapEnd;
static SimpleAllocator<kAlignment> sAllocator;

static LargeAllocationHashTable sLargeAllocations;


static void*
malloc_large(size_t size)
{
	LargeAllocation* allocation = new(std::nothrow) LargeAllocation;
	if (allocation == NULL) {
		dprintf("malloc_large(): Out of memory!\n");
		return NULL;
	}

	if (allocation->Allocate(size) != B_OK) {
		dprintf("malloc_large(): Out of memory!\n");
		delete allocation;
		return NULL;
	}

	sLargeAllocations.InsertUnchecked(allocation);
	TRACE("malloc_large(%lu) -> %p\n", size, allocation->Address());
	return allocation->Address();
}


static void
free_large(void* address)
{
	LargeAllocation* allocation = sLargeAllocations.Lookup(address);
	if (allocation == NULL)
		panic("free_large(%p): unknown allocation!\n", address);

	sLargeAllocations.RemoveUnchecked(allocation);
	allocation->Free();
	delete allocation;
}


//	#pragma mark -


void
heap_release()
{
	heap_print_statistics();

	LargeAllocation* allocation = sLargeAllocations.Clear(true);
	while (allocation != NULL) {
		LargeAllocation* next = allocation->HashNext();
		allocation->Free();
		allocation = next;
	}

	platform_free_heap_region(sHeapBase, (addr_t)sHeapEnd - (addr_t)sHeapBase);

	sHeapBase = sHeapEnd = NULL;
	memset((void*)&sAllocator, 0, sizeof(sAllocator));
	memset((void*)&sLargeAllocations, 0, sizeof(sLargeAllocations));
}


void
heap_print_statistics()
{
#ifdef DEBUG_MAX_HEAP_USAGE
	dprintf("maximum boot loader heap usage: %" B_PRIu32 ", currently used: %" B_PRIu32 "\n",
		sAllocator.MaxHeapUsage(), sAllocator.MaxHeapSize() - sAllocator.Available());
#endif
}


status_t
heap_init(stage2_args* args)
{
	if (args->heap_size == 0)
		args->heap_size = kDefaultHeapSize;

	void* base;
	ssize_t size = platform_allocate_heap_region(args->heap_size, &base);
	if (size < 0)
		return B_ERROR;

	sHeapBase = base;
	sHeapEnd = (void*)((addr_t)base + size);

	sAllocator.AddChunk(sHeapBase, size);

	if (sLargeAllocations.Init(64) != B_OK)
		return B_NO_MEMORY;

	return B_OK;
}


uint32
heap_available()
{
	return sAllocator.Available();
}


void*
malloc(size_t size)
{
	if (sHeapBase == NULL)
		return NULL;

	if (size >= kLargeAllocationThreshold)
		return malloc_large(size);

	void* allocated = sAllocator.Allocate(size);
	if (allocated == NULL) {
		if (size == 0)
			return allocated;

		if (size > sAllocator.Available()) {
			dprintf("malloc(): Out of memory allocating a block of %ld bytes, "
				"only %" B_PRId32 " left!\n", size, sAllocator.Available());
			return NULL;
		}

		dprintf("malloc(): Out of memory allocating a block of %ld bytes!\n", size);
		return NULL;
	}

	TRACE("malloc(%lu) -> %p\n", size, allocated);
	return allocated;
}


void*
realloc(void* oldBuffer, size_t newSize)
{
	if (newSize == 0) {
		TRACE("realloc(%p, %lu) -> NULL\n", oldBuffer, newSize);
		free(oldBuffer);
		return NULL;
	}

	size_t oldSize = 0;
	if (oldBuffer != NULL) {
		if (oldBuffer >= sHeapBase && oldBuffer < sHeapEnd) {
			oldSize = sAllocator.UsableSize(oldBuffer);
		} else {
			LargeAllocation* allocation = sLargeAllocations.Lookup(oldBuffer);
			if (allocation == NULL) {
				panic("realloc(%p, %zu): unknown allocation!\n", oldBuffer,
					newSize);
			}

			oldSize = allocation->Size();
		}

		// Check if the old buffer still fits, and if it makes sense to keep it.
		if (oldSize >= newSize
			&& (oldSize < 128 || newSize > oldSize / 3)) {
			TRACE("realloc(%p, %lu) old buffer is large enough\n",
				oldBuffer, newSize);
			return oldBuffer;
		}
	}

	void* newBuffer = malloc(newSize);
	if (newBuffer == NULL)
		return NULL;

	if (oldBuffer != NULL) {
		memcpy(newBuffer, oldBuffer, std::min(oldSize, newSize));
		free(oldBuffer);
	}

	TRACE("realloc(%p, %lu) -> %p\n", oldBuffer, newSize, newBuffer);
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

	TRACE("free(%p)\n", allocated);

	if (allocated < sHeapBase || allocated >= sHeapEnd) {
		free_large(allocated);
		return;
	}

	sAllocator.Free(allocated);
}
