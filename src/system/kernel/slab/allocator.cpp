/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include <Slab.h>

#include "slab_private.h"

#include <stdio.h>

#define DEBUG_ALLOCATOR
// #define TEST_ALL_CACHES_DURING_BOOT

static const size_t kBlockSizes[] = {
	16, 24, 32, 48, 64, 80, 96, 112,
	128, 160, 192, 224, 256, 320, 384, 448,
	512, 640, 768, 896, 1024, 1280, 1536, 1792,
	2048, 2560, 3072, 3584, 4096, 4608, 5120, 5632,
	6144, 6656, 7168, 7680, 8192,
	0
};

static const size_t kNumBlockSizes = sizeof(kBlockSizes) / sizeof(size_t) - 1;

static object_cache *sBlockCaches[kNumBlockSizes];

struct boundary_tag {
	uint32 size;
#ifdef DEBUG_ALLOCATOR
	uint32 magic;
#endif
};

static const uint32 kBoundaryMagic = 0x6da78d13;


static object_cache *
size_to_cache(size_t size)
{
	if (size <= 16)
		return sBlockCaches[0];
	else if (size <= 32)
		return sBlockCaches[1 + (size - 16 - 1) / 8];
	else if (size <= 128)
		return sBlockCaches[3 + (size - 32 - 1) / 16];
	else if (size <= 256)
		return sBlockCaches[9 + (size - 128 - 1) / 32];
	else if (size <= 512)
		return sBlockCaches[13 + (size - 256 - 1) / 64];
	else if (size <= 1024)
		return sBlockCaches[17 + (size - 512 - 1) / 128];
	else if (size <= 2048)
		return sBlockCaches[21 + (size - 1024 - 1) / 256];
	else if (size <= 8192)
		return sBlockCaches[25 + (size - 2048 - 1) / 512];

	return NULL;
}


void *
block_alloc(size_t size)
{
	object_cache *cache = size_to_cache(size + sizeof(boundary_tag));

	void *block;

	if (cache) {
		block = object_cache_alloc(cache, 0);
	} else {
		// TODO create areas
		panic("allocator: unimplemented");
		return NULL;
	}

	boundary_tag *tag = (boundary_tag *)block;

	tag->size = size;

#ifdef DEBUG_ALLOCATOR
	tag->magic = kBoundaryMagic;
#endif

	return ((uint8 *)block) + sizeof(boundary_tag);
}


void
block_free(void *block)
{
	boundary_tag *tag = (boundary_tag *)(((uint8 *)block)
		- sizeof(boundary_tag));

#ifdef DEBUG_ALLOCATOR
	if (tag->magic != kBoundaryMagic)
		panic("allocator: boundary tag magic doesn't match this universe");
#endif

	object_cache *cache = size_to_cache(tag->size);
	if (cache == NULL)
		panic("allocator: unimplemented");

	object_cache_free(cache, tag);
}


static void
block_create_cache(size_t index, bool boot)
{
	char name[32];
	snprintf(name, sizeof(name), "block cache: %lu", kBlockSizes[index]);

	sBlockCaches[index] = create_object_cache_etc(name, kBlockSizes[index],
		0, 0, boot ? CACHE_DURING_BOOT : 0, NULL, NULL, NULL, NULL);
	if (sBlockCaches[index] == NULL)
		panic("allocator: failed to init block cache");
}


void
block_allocator_init_boot()
{
	for (int index = 0; kBlockSizes[index] != 0; index++) {
		if (kBlockSizes[index] > 256)
			break;

		block_create_cache(index, true);
	}
}


void
block_allocator_init_rest()
{
	for (int index = 0; kBlockSizes[index] != 0; index++) {
		if (kBlockSizes[index] <= 256)
			continue;

		block_create_cache(index, false);
	}

#ifdef TEST_ALL_CACHES_DURING_BOOT
	for (int index = 0; kBlockSizes[index] != 0; index++) {
		block_free(block_alloc(kBlockSizes[index] - sizeof(boundary_tag)));
	}
#endif
}

