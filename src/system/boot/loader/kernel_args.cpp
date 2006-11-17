/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <kernel.h>
#include <boot/stage2.h>
#include <boot/platform.h>

#include <string.h>


static const size_t kChunkSize = 4 * B_PAGE_SIZE;

kernel_args gKernelArgs;

static void *sFirstFree;
static void *sLast;
static size_t sFree = kChunkSize;


static void
remove_range_index(addr_range *ranges, uint32 &numRanges, uint32 index)
{
	if (index + 1 == numRanges) {
		// remove last range
		numRanges--;
		return;
	}

	memmove(&ranges[index], &ranges[index + 1], sizeof(addr_range) * (numRanges - 1 - index));
	numRanges--;
}


/**	Inserts the specified (start, size) pair (aka range) in the
 *	addr_range array.
 *	It will extend existing ranges in order to have as little
 *	ranges in the array as possible.
 *	Returns B_OK on success, or B_ENTRY_NOT_FOUND if there was
 *	no free array entry available anymore.
 */

extern "C" status_t
insert_address_range(addr_range *ranges, uint32 *_numRanges, uint32 maxRanges,
	addr_t start, uint32 size)
{
	uint32 numRanges = *_numRanges;

	start = ROUNDOWN(start, B_PAGE_SIZE);
	size = ROUNDUP(size, B_PAGE_SIZE);
	addr_t end = start + size;

	for (uint32 i = 0; i < numRanges; i++) {
		addr_t rangeStart = ranges[i].start;
		addr_t rangeEnd = rangeStart + ranges[i].size;

		if (end < rangeStart || start > rangeEnd) {
			// ranges don't intersect or touch each other
			continue;
		}
		if (start >= rangeStart && end <= rangeEnd) {
			// range is already completely covered
			return B_OK;
		}

		if (start < rangeStart) {
			// prepend to the existing range
			ranges[i].start = start;
			ranges[i].size += rangeStart - start;
		}
		if (end > ranges[i].start + ranges[i].size) {
			// append to the existing range
			ranges[i].size = end - ranges[i].start;
		}

		// join ranges if possible

		for (uint32 j = 0; j < numRanges; j++) {
			if (i == j)
				continue;

			rangeStart = ranges[i].start;
			rangeEnd = rangeStart + ranges[i].size;
			addr_t joinStart = ranges[j].start;
			addr_t joinEnd = joinStart + ranges[j].size;

			if (rangeStart <= joinEnd && joinEnd <= rangeEnd) {
				// join range that used to be before the current one, or
				// the one that's now entirely included by the current one
				if (joinStart < rangeStart) {
					ranges[i].size += rangeStart - joinStart;
					ranges[i].start = joinStart;
				}

				remove_range_index(ranges, numRanges, j--);
			} else if (joinStart <= rangeEnd && joinEnd > rangeEnd) {
				// join range that used to be after the current one
				ranges[i].size += joinEnd - rangeEnd;

				remove_range_index(ranges, numRanges, j--);
			}
		}

		*_numRanges = numRanges;
		return B_OK;
	}

	// no range matched, we need to create a new one
	
	if (numRanges >= maxRanges)
		return B_ENTRY_NOT_FOUND;

	ranges[numRanges].start = (addr_t)start;
	ranges[numRanges].size = size;
	(*_numRanges)++;

	return B_OK;
}


static status_t
add_kernel_args_range(void *start, uint32 size)
{
	return insert_address_range(gKernelArgs.kernel_args_range, 
		&gKernelArgs.num_kernel_args_ranges, MAX_KERNEL_ARGS_RANGE,
		(addr_t)start, size);
}


/** This function can be used to allocate memory that is going
 *	to be passed over to the kernel. For example, the preloaded_image
 *	structures are allocated this way.
 *	The boot loader heap doesn't make it into the kernel!
 */

extern "C" void *
kernel_args_malloc(size_t size)
{
	//dprintf("kernel_args_malloc(): %ld bytes (%ld bytes left)\n", size, sFree);

	if (sFirstFree != NULL && size <= sFree) {
		// there is enough space in the current buffer
		void *address = sFirstFree;
		sFirstFree = (void *)((addr_t)sFirstFree + size);
		sLast = address;
		sFree -= size;

		return address;
	}

	if (size > kChunkSize / 2 && sFree < size) {
		// the block is so large, we'll allocate a new block for it
		void *block = NULL;
		if (platform_allocate_region(&block, size, B_READ_AREA | B_WRITE_AREA,
			false) != B_OK) {
			return NULL;
		}

		if (add_kernel_args_range(block, size) != B_OK)
			panic("kernel_args max range to low!\n");
		return block;
	}

	// just allocate a new block and "close" the old one
	void *block = NULL;
	if (platform_allocate_region(&block, kChunkSize,
		B_READ_AREA | B_WRITE_AREA, false) != B_OK) {
		return NULL;
	}

	sFirstFree = (void *)((addr_t)block + size);
	sLast = block;
	sFree = kChunkSize - size;
	if (add_kernel_args_range(block, kChunkSize) != B_OK)
		panic("kernel_args max range to low!\n");

	return block;
}


/** Convenience function that copies strdup() functions for the
 *	kernel args heap.
 */

extern "C" char *
kernel_args_strdup(const char *string)
{
	if (string == NULL || string[0] == '\0')
		return NULL;

	size_t length = strlen(string) + 1;

	char *target = (char *)kernel_args_malloc(length);
	if (target == NULL)
		return NULL;

	memcpy(target, string, length);
	return target;
}


/** This function frees a block allocated via kernel_args_malloc().
 *	It's very simple; it can only free the last allocation. It's
 *	enough for its current usage in the boot loader, though.
 */

extern "C" void
kernel_args_free(void *block)
{
	if (sLast != block) {
		// sorry, we're dumb
		return;
	}

	sFree = (addr_t)sFirstFree - (addr_t)sLast;
	sFirstFree = block;
	sLast = NULL;
}

