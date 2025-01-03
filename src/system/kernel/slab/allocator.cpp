/*
 * Copyright 2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "slab_private.h"

#include <stdio.h>
#include <string.h>

#include <algorithm>

#include <debug.h>
#include <heap.h>
#include <kernel.h> // for ROUNDUP
#include <malloc.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include "ObjectCache.h"
#include "MemoryManager.h"


#if USE_SLAB_ALLOCATOR_FOR_MALLOC


//#define TEST_ALL_CACHES_DURING_BOOT

static const size_t kBlockSizes[] = {
	16, 24, 32,
	48, 64, 80, 96, 112, 128,
	160, 192, 224, 256,
	320, 384, 448, 512,
	640, 768, 896, 1024,
	1280, 1536, 1792, 2048,
	2560, 3072, 3584, 4096,
	5120, 6144, 7168, 8192,
	10240, 12288, 14336, 16384,
};

static const size_t kNumBlockSizes = B_COUNT_OF(kBlockSizes);

static object_cache* sBlockCaches[kNumBlockSizes];

static addr_t sBootStrapMemory = 0;
static size_t sBootStrapMemorySize = 0;
static size_t sUsedBootStrapMemory = 0;


RANGE_MARKER_FUNCTION_BEGIN(slab_allocator)


static int
size_to_index(size_t size)
{
	if (size <= 16)
		return 0;
	if (size <= 32)
		return 1 + (size - 16 - 1) / 8;
	if (size <= 128)
		return 3 + (size - 32 - 1) / 16;
	if (size <= 256)
		return 9 + (size - 128 - 1) / 32;
	if (size <= 512)
		return 13 + (size - 256 - 1) / 64;
	if (size <= 1024)
		return 17 + (size - 512 - 1) / 128;
	if (size <= 2048)
		return 21 + (size - 1024 - 1) / 256;
	if (size <= 4096)
		return 25 + (size - 2048 - 1) / 512;
	if (size <= 8192)
		return 29 + (size - 4096 - 1) / 1024;
	if (size <= 16384)
		return 33 + (size - 8192 - 1) / 2048;

	return -1;
}


static void*
block_alloc(size_t size, size_t alignment, uint32 flags)
{
	if (alignment > kMinObjectAlignment) {
		// Make size >= alignment and a power of two. This is sufficient, since
		// all of our object caches with power of two sizes are aligned. We may
		// waste quite a bit of memory, but memalign() is very rarely used
		// in the kernel and always with power of two size == alignment anyway.
		ASSERT((alignment & (alignment - 1)) == 0);
		while (alignment < size)
			alignment <<= 1;
		size = alignment;

		// If we're not using an object cache, make sure that the memory
		// manager knows it has to align the allocation.
		if (size > kBlockSizes[kNumBlockSizes - 1])
			flags |= CACHE_ALIGN_ON_SIZE;
	}

	// allocate from the respective object cache, if any
	int index = size_to_index(size);
	if (index >= 0)
		return object_cache_alloc(sBlockCaches[index], flags);

	// the allocation is too large for our object caches -- ask the memory
	// manager
	void* block;
	if (MemoryManager::AllocateRaw(size, flags, block) != B_OK)
		return NULL;

	return block;
}


void*
block_alloc_early(size_t size)
{
	int index = size_to_index(size);
	if (index >= 0 && sBlockCaches[index] != NULL)
		return object_cache_alloc(sBlockCaches[index], CACHE_DURING_BOOT);

	if (size > SLAB_CHUNK_SIZE_SMALL) {
		// This is a sufficiently large allocation -- just ask the memory
		// manager directly.
		void* block;
		if (MemoryManager::AllocateRaw(size, 0, block) != B_OK)
			return NULL;

		return block;
	}

	// A small allocation, but no object cache yet. Use the bootstrap memory.
	// This allocation must never be freed!
	if (sBootStrapMemorySize - sUsedBootStrapMemory < size) {
		// We need more memory.
		void* block;
		if (MemoryManager::AllocateRaw(SLAB_CHUNK_SIZE_SMALL, 0, block) != B_OK)
			return NULL;
		sBootStrapMemory = (addr_t)block;
		sBootStrapMemorySize = SLAB_CHUNK_SIZE_SMALL;
		sUsedBootStrapMemory = 0;
	}

	size_t neededSize = ROUNDUP(size, sizeof(double));
	if (sUsedBootStrapMemory + neededSize > sBootStrapMemorySize)
		return NULL;
	void* block = (void*)(sBootStrapMemory + sUsedBootStrapMemory);
	sUsedBootStrapMemory += neededSize;

	return block;
}


static void
block_free(void* block, uint32 flags)
{
	if (block == NULL)
		return;

	ObjectCache* cache = MemoryManager::FreeRawOrReturnCache(block, flags);
	if (cache != NULL) {
		// a regular small allocation
		ASSERT(cache->object_size >= kBlockSizes[0]);
		ASSERT(cache->object_size <= kBlockSizes[kNumBlockSizes - 1]);
		ASSERT(cache == sBlockCaches[size_to_index(cache->object_size)]);
		object_cache_free(cache, block, flags);
	}
}


void
block_allocator_init_boot()
{
	for (size_t index = 0; index < kNumBlockSizes; index++) {
		char name[32];
		snprintf(name, sizeof(name), "block allocator: %lu",
			kBlockSizes[index]);

		uint32 flags = CACHE_DURING_BOOT;
		size_t size = kBlockSizes[index];

		// align the power of two objects to their size
		size_t alignment = (size & (size - 1)) == 0 ? size : 0;

		// For the larger allocation sizes disable the object depot, so we don't
		// keep lot's of unused objects around.
		if (size > 2048)
			flags |= CACHE_NO_DEPOT;

		sBlockCaches[index] = create_object_cache_etc(name, size, alignment, 0,
			0, 0, flags, NULL, NULL, NULL, NULL);
		if (sBlockCaches[index] == NULL)
			panic("allocator: failed to init block cache");
	}
}


void
block_allocator_init_rest()
{
#ifdef TEST_ALL_CACHES_DURING_BOOT
	for (int index = 0; kBlockSizes[index] != 0; index++) {
		block_free(block_alloc(kBlockSizes[index] - sizeof(boundary_tag)), 0,
			0);
	}
#endif
}


// #pragma mark - public API


void*
memalign(size_t alignment, size_t size)
{
	return block_alloc(size, alignment, 0);
}


void *
memalign_etc(size_t alignment, size_t size, uint32 flags)
{
	return block_alloc(size, alignment, flags & CACHE_ALLOC_FLAGS);
}


int
posix_memalign(void** _pointer, size_t alignment, size_t size)
{
	if ((alignment & (sizeof(void*) - 1)) != 0 || _pointer == NULL)
		return B_BAD_VALUE;
	*_pointer = block_alloc(size, alignment, 0);
	return 0;
}


void
free_etc(void *address, uint32 flags)
{
	if ((flags & CACHE_DONT_LOCK_KERNEL_SPACE) != 0) {
		deferred_free(address);
		return;
	}

	block_free(address, flags & CACHE_ALLOC_FLAGS);
}


void*
malloc(size_t size)
{
	return block_alloc(size, 0, 0);
}


void
free(void* address)
{
	block_free(address, 0);
}


void*
realloc_etc(void* address, size_t newSize, uint32 flags)
{
	if (newSize == 0) {
		block_free(address, flags);
		return NULL;
	}

	if (address == NULL)
		return block_alloc(newSize, 0, flags);

	size_t oldSize;
	ObjectCache* cache = MemoryManager::GetAllocationInfo(address, oldSize);
	if (cache == NULL && oldSize == 0) {
		panic("block_realloc(): allocation %p not known", address);
		return NULL;
	}

	if (oldSize == newSize)
		return address;

	void* newBlock = block_alloc(newSize, 0, flags);
	if (newBlock == NULL)
		return NULL;

	memcpy(newBlock, address, std::min(oldSize, newSize));

	block_free(address, flags);

	return newBlock;
}


void*
realloc(void* address, size_t newSize)
{
	return realloc_etc(address, newSize, 0);
}


#else


void*
block_alloc_early(size_t size)
{
	panic("block allocator not enabled!");
	return NULL;
}


void
block_allocator_init_boot()
{
}


void
block_allocator_init_rest()
{
}


#endif	// USE_SLAB_ALLOCATOR_FOR_MALLOC


RANGE_MARKER_FUNCTION_END(slab_allocator)
