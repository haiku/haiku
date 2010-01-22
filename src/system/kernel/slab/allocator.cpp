/*
 * Copyright 2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */


#include "slab_private.h"

#include <stdio.h>
#include <string.h>

#include <kernel.h> // for ROUNDUP
#include <slab/Slab.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include "ObjectCache.h"
#include "MemoryManager.h"


#define DEBUG_ALLOCATOR
//#define TEST_ALL_CACHES_DURING_BOOT

static const size_t kBlockSizes[] = {
	16, 24, 32, 48, 64, 80, 96, 112,
	128, 160, 192, 224, 256, 320, 384, 448,
	512, 640, 768, 896, 1024, 1280, 1536, 1792,
	2048, 2560, 3072, 3584, 4096, 4608, 5120, 5632,
	6144, 6656, 7168, 7680, 8192,
	0
};

static const size_t kNumBlockSizes = sizeof(kBlockSizes) / sizeof(size_t) - 1;

static object_cache* sBlockCaches[kNumBlockSizes];

static addr_t sBootStrapMemory;
static size_t sBootStrapMemorySize;
static size_t sUsedBootStrapMemory;


static int
size_to_index(size_t size)
{
	if (size <= 16)
		return 0;
	else if (size <= 32)
		return 1 + (size - 16 - 1) / 8;
	else if (size <= 128)
		return 3 + (size - 32 - 1) / 16;
	else if (size <= 256)
		return 9 + (size - 128 - 1) / 32;
	else if (size <= 512)
		return 13 + (size - 256 - 1) / 64;
	else if (size <= 1024)
		return 17 + (size - 512 - 1) / 128;
	else if (size <= 2048)
		return 21 + (size - 1024 - 1) / 256;
	else if (size <= 8192)
		return 25 + (size - 2048 - 1) / 512;

	return -1;
}


void*
block_alloc(size_t size, uint32 flags)
{
	// allocate from the respective object cache, if any
	int index = size_to_index(size);
	if (index >= 0)
		return object_cache_alloc(sBlockCaches[index], flags);

	// the allocation is too large for our object caches -- create an area
	if ((flags & CACHE_DONT_LOCK_KERNEL_SPACE) != 0)
		return NULL;

	void* block;
	area_id area = create_area_etc(VMAddressSpace::KernelID(),
		"alloc'ed block", &block, B_ANY_KERNEL_ADDRESS,
		ROUNDUP(size, B_PAGE_SIZE), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0,
		(flags & CACHE_DONT_WAIT_FOR_MEMORY) != 0 ? CREATE_AREA_DONT_WAIT : 0);
	if (area < 0)
		return NULL;

	return block;
}


void*
block_alloc_early(size_t size)
{
	int index = size_to_index(size);
	if (index < 0)
		return NULL;

	if (sBlockCaches[index] != NULL)
		return object_cache_alloc(sBlockCaches[index], CACHE_DURING_BOOT);

	// No object cache yet. Use the bootstrap memory. This allocation must
	// never be freed!
	size_t neededSize = ROUNDUP(size, sizeof(double));
	if (sUsedBootStrapMemory + neededSize > sBootStrapMemorySize)
		return NULL;
	void* block = (void*)(sBootStrapMemory + sUsedBootStrapMemory);
	sUsedBootStrapMemory += neededSize;

	return block;
}


void
block_free(void* block, uint32 flags)
{
	if (ObjectCache* cache = MemoryManager::CacheForAddress(block)) {
		// a regular small allocation
		ASSERT(cache->object_size >= kBlockSizes[0]);
		ASSERT(cache->object_size <= kBlockSizes[kNumBlockSizes - 1]);
		ASSERT(cache == sBlockCaches[size_to_index(cache->object_size)]);
		object_cache_free(cache, block, flags);
	} else {
		// a large allocation -- look up the area
		VMAddressSpace* addressSpace = VMAddressSpace::Kernel();
		addressSpace->ReadLock();
		VMArea* area = addressSpace->LookupArea((addr_t)block);
		addressSpace->ReadUnlock();

		if (area != NULL && (addr_t)block == area->Base())
			delete_area(area->id);
		else
			panic("freeing unknown block %p from area %p", block, area);
	}
}


void
block_allocator_init_boot(addr_t bootStrapBase, size_t bootStrapSize)
{
	sBootStrapMemory = bootStrapBase;
	sBootStrapMemorySize = bootStrapSize;
	sUsedBootStrapMemory = 0;

	for (int index = 0; kBlockSizes[index] != 0; index++) {
		char name[32];
		snprintf(name, sizeof(name), "block cache: %lu", kBlockSizes[index]);

		sBlockCaches[index] = create_object_cache_etc(name, kBlockSizes[index],
			0, 0, CACHE_DURING_BOOT, NULL, NULL, NULL, NULL);
		if (sBlockCaches[index] == NULL)
			panic("allocator: failed to init block cache");
	}
}


void
block_allocator_init_rest()
{
#ifdef TEST_ALL_CACHES_DURING_BOOT
	for (int index = 0; kBlockSizes[index] != 0; index++) {
		block_free(block_alloc(kBlockSizes[index] - sizeof(boundary_tag)));
	}
#endif
}
