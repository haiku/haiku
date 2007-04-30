/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include <Slab.h>

#include "slab_private.h"

#include <kernel.h> // for ROUNDUP

#include <stdio.h>
#include <string.h>

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

static object_cache *sBlockCaches[kNumBlockSizes];
static int32 sBlockCacheWaste[kNumBlockSizes];

struct boundary_tag {
	uint32 size;
#ifdef DEBUG_ALLOCATOR
	uint32 magic;
#endif
};

struct area_boundary_tag {
	area_id area;
	boundary_tag tag;
};


static const uint32 kBoundaryMagic = 0x6da78d13;


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


void *
block_alloc(size_t size)
{
	int index = size_to_index(size + sizeof(boundary_tag));

	void *block;
	boundary_tag *tag;

	if (index < 0) {
		void *pages;
		area_id area = create_area("alloc'ed block", &pages,
			B_ANY_KERNEL_ADDRESS, ROUNDUP(size + sizeof(area_boundary_tag),
				B_PAGE_SIZE), B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (area < B_OK)
			return NULL;

		area_boundary_tag *areaTag = (area_boundary_tag *)pages;
		areaTag->area = area;
		tag = &areaTag->tag;
		block = areaTag + 1;
	} else {
		tag = (boundary_tag *)object_cache_alloc(sBlockCaches[index], 0);
		if (tag == NULL)
			return NULL;
		atomic_add(&sBlockCacheWaste[index], kBlockSizes[index] - size
			- sizeof(boundary_tag));
		block = tag + 1;
	}

	tag->size = size;

#ifdef DEBUG_ALLOCATOR
	tag->magic = kBoundaryMagic;
#endif

	return block;
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

	int index = size_to_index(tag->size + sizeof(boundary_tag));

	if (index < 0) {
		area_boundary_tag *areaTag = (area_boundary_tag *)(((uint8 *)block)
			- sizeof(area_boundary_tag));
		delete_area(areaTag->area);
	} else {
		atomic_add(&sBlockCacheWaste[index], -(kBlockSizes[index] - tag->size
			- sizeof(boundary_tag)));
		object_cache_free(sBlockCaches[index], tag);
	}
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

	sBlockCacheWaste[index] = 0;
}


static int
show_waste(int argc, char *argv[])
{
	size_t total = 0;

	for (size_t index = 0; index < kNumBlockSizes; index++) {
		if (sBlockCacheWaste[index] > 0) {
			kprintf("%lu: %lu\n", kBlockSizes[index], sBlockCacheWaste[index]);
			total += sBlockCacheWaste[index];
		}
	}

	kprintf("total waste: %lu\n", total);

	return 0;
}


void
block_allocator_init_boot()
{
	for (int index = 0; kBlockSizes[index] != 0; index++)
		block_create_cache(index, true);

	add_debugger_command("show_waste", show_waste,
		"show cache allocator's memory waste");
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

