/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <string.h>

#include <algorithm>

#include <kernel.h>
#include <boot/stage2.h>
#include <boot/platform.h>


static const size_t kChunkSize = 16 * B_PAGE_SIZE;

kernel_args gKernelArgs;
KMessage gBootVolume;

static void* sFirstFree;
static void* sLast;
static size_t sFree = kChunkSize;


static status_t
add_kernel_args_range(void* start, size_t size)
{
	return insert_address_range(gKernelArgs.kernel_args_range,
		&gKernelArgs.num_kernel_args_ranges, MAX_KERNEL_ARGS_RANGE,
		(addr_t)start, size);
}


// #pragma mark - addr_range utility functions


static void
remove_range_index(addr_range* ranges, uint32& numRanges, uint32 index)
{
	if (index + 1 == numRanges) {
		// remove last range
		numRanges--;
		return;
	}

	memmove(&ranges[index], &ranges[index + 1],
		sizeof(addr_range) * (numRanges - 1 - index));
	numRanges--;
}


/*!	Inserts the specified (start, size) pair (aka range) in the
	addr_range array.
	It will extend existing ranges in order to have as little
	ranges in the array as possible.
	Returns B_OK on success, or B_ENTRY_NOT_FOUND if there was
	no free array entry available anymore.
*/
extern "C" status_t
insert_address_range(addr_range* ranges, uint32* _numRanges, uint32 maxRanges,
	uint64 start, uint64 size)
{
	uint32 numRanges = *_numRanges;

	start = ROUNDDOWN(start, B_PAGE_SIZE);
	size = ROUNDUP(size, B_PAGE_SIZE);
	uint64 end = start + size;

	for (uint32 i = 0; i < numRanges; i++) {
		uint64 rangeStart = ranges[i].start;
		uint64 rangeEnd = rangeStart + ranges[i].size;

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
			uint64 joinStart = ranges[j].start;
			uint64 joinEnd = joinStart + ranges[j].size;

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

	ranges[numRanges].start = (uint64)start;
	ranges[numRanges].size = size;
	(*_numRanges)++;

	return B_OK;
}


extern "C" status_t
remove_address_range(addr_range* ranges, uint32* _numRanges, uint32 maxRanges,
	uint64 start, uint64 size)
{
	uint32 numRanges = *_numRanges;

	uint64 end = ROUNDUP(start + size, B_PAGE_SIZE);
	start = ROUNDDOWN(start, B_PAGE_SIZE);

	for (uint32 i = 0; i < numRanges; i++) {
		uint64 rangeStart = ranges[i].start;
		uint64 rangeEnd = rangeStart + ranges[i].size;

		if (start <= rangeStart) {
			if (end <= rangeStart) {
				// no intersection
			} else if (end >= rangeEnd) {
				// remove the complete range
				remove_range_index(ranges, numRanges, i);
				i--;
			} else {
				// remove the head of the range
				ranges[i].start = end;
				ranges[i].size = rangeEnd - end;
			}
		} else if (end >= rangeEnd) {
			if (start < rangeEnd) {
				// remove the tail
				ranges[i].size = start - rangeStart;
			}	// else: no intersection
		} else {
			// rangeStart < start < end < rangeEnd
			// The ugly case: We have to remove something from the middle of
			// the range. We keep the head of the range and insert its tail
			// as a new range.
			ranges[i].size = start - rangeStart;
			return insert_address_range(ranges, _numRanges, maxRanges, end,
				rangeEnd - end);
		}
	}

	*_numRanges = numRanges;
	return B_OK;
}


bool
get_free_address_range(addr_range* ranges, uint32 numRanges, uint64 base,
	uint64 size, uint64* _rangeBase)
{
	uint64 end = base + size - 1;
	if (end < base)
		return false;

	// Note: We don't assume that the ranges are sorted, so we can't do this
	// in a simple loop. Instead we restart the loop whenever our range
	// intersects with an existing one.

	for (uint32 i = 0; i < numRanges;) {
		uint64 rangeStart = ranges[i].start;
		uint64 rangeEnd = ranges[i].start + ranges[i].size - 1;

		if (base <= rangeEnd && rangeStart <= end) {
			base = rangeEnd + 1;
			end = rangeEnd + size;
			if (base == 0 || end < base)
				return false;

			i = 0;
			continue;
		}

		i++;
	}

	*_rangeBase = base;
	return true;
}


bool
is_address_range_covered(addr_range* ranges, uint32 numRanges, uint64 base,
	uint64 size)
{
	// Note: We don't assume that the ranges are sorted, so we can't do this
	// in a simple loop. Instead we restart the loop whenever the start of the
	// given range intersects with an existing one.

	for (uint32 i = 0; i < numRanges;) {
		uint64 rangeStart = ranges[i].start;
		uint64 rangeSize = ranges[i].size;

		if (rangeStart <= base && rangeSize > base - rangeStart) {
			uint64 intersect = std::min(rangeStart + rangeSize - base, size);
			base += intersect;
			size -= intersect;
			if (size == 0)
				return true;

			i = 0;
			continue;
		}

		i++;
	}

	return false;
}


extern "C" uint64
total_address_ranges_size(addr_range* ranges, uint32 numRanges)
{
	uint64 total = 0;
	for (uint32 i = 0; i < numRanges; i++)
		total += ranges[i].size;
	return total;
}


void
sort_address_ranges(addr_range* ranges, uint32 numRanges)
{
	// TODO: This is a pretty sucky bubble sort implementation!
	bool done;

	do {
		done = true;
		for (uint32 i = 1; i < numRanges; i++) {
			if (ranges[i].start < ranges[i - 1].start) {
				done = false;
				addr_range tempRange;
				memcpy(&tempRange, &ranges[i], sizeof(addr_range));
				memcpy(&ranges[i], &ranges[i - 1], sizeof(addr_range));
				memcpy(&ranges[i - 1], &tempRange, sizeof(addr_range));
			}
		}
	} while (!done);
}


// #pragma mark - kernel args range functions


status_t
insert_physical_memory_range(uint64 start, uint64 size)
{
	return insert_address_range(gKernelArgs.physical_memory_range,
		&gKernelArgs.num_physical_memory_ranges, MAX_PHYSICAL_MEMORY_RANGE,
		start, size);
}


status_t
remove_physical_memory_range(uint64 start, uint64 size)
{
	return remove_address_range(gKernelArgs.physical_memory_range,
		&gKernelArgs.num_physical_memory_ranges, MAX_PHYSICAL_MEMORY_RANGE,
		start, size);
}


uint64
total_physical_memory()
{
	return total_address_ranges_size(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges);
}


status_t
insert_physical_allocated_range(uint64 start, uint64 size)
{
	return insert_address_range(gKernelArgs.physical_allocated_range,
		&gKernelArgs.num_physical_allocated_ranges,
		MAX_PHYSICAL_ALLOCATED_RANGE, start, size);
}


status_t
insert_virtual_allocated_range(uint64 start, uint64 size)
{
	return insert_address_range(gKernelArgs.virtual_allocated_range,
		&gKernelArgs.num_virtual_allocated_ranges, MAX_VIRTUAL_ALLOCATED_RANGE,
		start, size);
}


#if B_HAIKU_PHYSICAL_BITS > 32

void
ignore_physical_memory_ranges_beyond_4gb()
{
	// sort
	sort_address_ranges(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges);

	static const uint64 kLimit = (uint64)1 << 32;

	// remove everything past 4 GB
	for (uint32 i = gKernelArgs.num_physical_memory_ranges; i > 0; i--) {
		addr_range& range = gKernelArgs.physical_memory_range[i - 1];
		if (range.start >= kLimit) {
			// the complete range is beyond the limit
			dprintf("ignore_physical_memory_ranges_beyond_4gb(): ignoring "
				"range: %#" B_PRIx64 " - %#" B_PRIx64 "\n", range.start,
				range.start + range.size);
			gKernelArgs.ignored_physical_memory += range.size;
			gKernelArgs.num_physical_memory_ranges = i - 1;
			continue;
		}

		if (kLimit - range.start < range.size) {
			// the range is partially beyond the limit
			dprintf("ignore_physical_memory_ranges_beyond_4gb(): ignoring "
				"range: %#" B_PRIx64 " - %#" B_PRIx64 "\n", kLimit,
				range.start + range.size);
			gKernelArgs.ignored_physical_memory
				+= range.size - (kLimit - range.start);
		}

		break;
	}
}

#endif	// B_HAIKU_PHYSICAL_BITS > 32


// #pragma mark - kernel_args allocations


/*!	This function can be used to allocate memory that is going
	to be passed over to the kernel. For example, the preloaded_image
	structures are allocated this way.
	The boot loader heap doesn't make it into the kernel!
*/
extern "C" void*
kernel_args_malloc(size_t size)
{
	//dprintf("kernel_args_malloc(): %ld bytes (%ld bytes left)\n", size, sFree);

	if (sFirstFree != NULL && size <= sFree) {
		// there is enough space in the current buffer
		void* address = sFirstFree;
		sFirstFree = (void*)((addr_t)sFirstFree + size);
		sLast = address;
		sFree -= size;

		return address;
	}

	if (size > kChunkSize / 2 && sFree < size) {
		// the block is so large, we'll allocate a new block for it
		void* block = NULL;
		if (platform_allocate_region(&block, size, B_READ_AREA | B_WRITE_AREA,
				false) != B_OK) {
			return NULL;
		}

#ifdef _BOOT_PLATFORM_EFI
		addr_t translated_block;
		platform_bootloader_address_to_kernel_address(block, &translated_block);
		if (add_kernel_args_range((void *)translated_block, size) != B_OK)
#else
		if (add_kernel_args_range(block, size) != B_OK)
#endif
			panic("kernel_args max range too low!\n");
		return block;
	}

	// just allocate a new block and "close" the old one
	void* block = NULL;
	if (platform_allocate_region(&block, kChunkSize, B_READ_AREA | B_WRITE_AREA,
			false) != B_OK) {
		return NULL;
	}

	sFirstFree = (void*)((addr_t)block + size);
	sLast = block;
	sFree = kChunkSize - size;
#ifdef _BOOT_PLATFORM_EFI
	addr_t translated_block;
	platform_bootloader_address_to_kernel_address(block, &translated_block);
	if (add_kernel_args_range((void *)translated_block, kChunkSize) != B_OK)
#else
	if (add_kernel_args_range(block, kChunkSize) != B_OK)
#endif
		panic("kernel_args max range too low!\n");

	return block;
}


/*!	Convenience function that copies strdup() functions for the
	kernel args heap.
*/
extern "C" char*
kernel_args_strdup(const char* string)
{
	if (string == NULL || string[0] == '\0')
		return NULL;

	size_t length = strlen(string) + 1;

	char* target = (char*)kernel_args_malloc(length);
	if (target == NULL)
		return NULL;

	memcpy(target, string, length);
	return target;
}


/*!	This function frees a block allocated via kernel_args_malloc().
	It's very simple; it can only free the last allocation. It's
	enough for its current usage in the boot loader, though.
*/
extern "C" void
kernel_args_free(void* block)
{
	if (sLast != block) {
		// sorry, we're dumb
		return;
	}

	sFree = (addr_t)sFirstFree - (addr_t)sLast;
	sFirstFree = block;
	sLast = NULL;
}

