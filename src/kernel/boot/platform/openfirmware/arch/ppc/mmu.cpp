/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "openfirmware.h"

#include <platform_arch.h>
#include <boot/stage2.h>
#include <boot/stdio.h>
#include <arch_cpu.h>
#include <arch_mmu.h>
#include <kernel.h>

#include <OS.h>


page_table_entry_group *sPageTable;
uint32 sPageTableHashMask;


static status_t
insert_memory_range(address_range *ranges, uint32 &numRanges, uint32 maxRanges,
	const void *start, uint32 size)
{
	size = ROUNDUP(size, B_PAGE_SIZE);

	for (uint32 i = 0; i < numRanges; i++) {
		if ((uint32)start == ranges[i].start + ranges[i].size) {
			// append to the existing range
			ranges[i].size += size;
			return B_OK;
		} else if ((uint32)start + size == ranges[i].start) {
			// preprend before the existing range
			ranges[i].start = (uint32)start;
			return B_OK;
		}
	}

	// no range matched, we need to create a new one
	
	if (numRanges >= maxRanges)
		return B_ENTRY_NOT_FOUND;

	ranges[numRanges].start = (uint32)start;
	ranges[numRanges].size = size;
	numRanges++;

	return B_OK;
}


static status_t
insert_physical_memory_range(void *start, uint32 size)
{
	return insert_memory_range(gKernelArgs.physical_memory_range, 
				gKernelArgs.num_physical_memory_ranges, MAX_PHYSICAL_MEMORY_RANGE,
				start, size);
}


static status_t
insert_physical_allocated_range(void *start, uint32 size)
{
	return insert_memory_range(gKernelArgs.physical_allocated_range, 
				gKernelArgs.num_physical_allocated_ranges, MAX_PHYSICAL_ALLOCATED_RANGE,
				start, size);
}


static status_t
insert_virtual_allocated_range(void *start, uint32 size)
{
	return insert_memory_range(gKernelArgs.virtual_allocated_range, 
				gKernelArgs.num_virtual_allocated_ranges, MAX_VIRTUAL_ALLOCATED_RANGE,
				start, size);
}


static status_t
find_physical_memory_ranges(size_t &total)
{
	int memory;
	if (of_getprop(gChosen, "memory", &memory, sizeof(int)) == OF_FAILED)
		return B_ERROR;
	memory = of_instance_to_package(memory);

	total = 0;

	struct of_region regions[64];
	int count = of_getprop(memory, "reg", regions, sizeof(regions));
	if (count == OF_FAILED)
		return B_ERROR;
	count /= sizeof(of_region);

	for (int32 i = 0; i < count; i++) {
		if (regions[i].size <= 0) {
			printf("%ld: empty region", i);
			continue;
		}
		printf("%ld: base = %p, size = %lu\n", i, regions[i].base, regions[i].size);

		total += regions[i].size;

		if (insert_physical_memory_range(regions[i].base, regions[i].size) < B_OK) {
			printf("cannot map physical memory range (num ranges = %lu)!\n", gKernelArgs.num_physical_memory_ranges);
			return B_ERROR;
		}
	}

	return B_OK;
}


static bool
is_in_range(address_range *ranges, uint32 numRanges, void *address, size_t size)
{
	uint32 start = (uint32)address;
	uint32 end = start + size;

	for (uint32 i = 0; i < numRanges; i++) {
		uint32 rangeStart = ranges[i].start;
		uint32 rangeEnd = rangeStart + ranges[i].size;

		if ((start >= rangeStart && start < rangeEnd)
			|| (end >= rangeStart && end < rangeEnd))
			return true;
	}
	return false;
}


static bool
is_virtual_allocated(void *address, size_t size)
{
	return is_in_range(gKernelArgs.virtual_allocated_range, 
				gKernelArgs.num_virtual_allocated_ranges, 
				address, size);
}


static bool
is_physical_allocated(void *address, size_t size)
{
	return is_in_range(gKernelArgs.physical_allocated_range, 
				gKernelArgs.num_physical_allocated_ranges, 
				address, size);
}


static bool
is_physical_memory(void *address, size_t size)
{
	return is_in_range(gKernelArgs.physical_memory_range, 
				gKernelArgs.num_physical_memory_ranges, 
				address, size);
}


static bool
is_physical_memory(void *address)
{
	return is_physical_memory(address, 0);
}


static void
fill_page_table_entry(page_table_entry *entry, uint32 virtualSegmentID, void *virtualAddress, void *physicalAddress, uint8 mode, bool secondaryHash)
{
	// lower 32 bit - set at once
	((uint32 *)entry)[1] = (((uint32)physicalAddress / B_PAGE_SIZE) << 12) | mode;
	/*entry->physical_page_number = (uint32)physicalAddress / B_PAGE_SIZE;
	entry->_reserved0 = 0;
	entry->referenced = false;
	entry->changed = false;
	entry->write_through = (mode >> 6) & 1;
	entry->caching_inhibited = (mode >> 5) & 1;
	entry->memory_coherent = (mode >> 4) & 1;
	entry->guarded = (mode >> 3) & 1;
	entry->_reserved1 = 0;
	entry->page_protection = mode & 0x3;*/
	eieio();
		// we need to make sure that the lower 32 bit were
		// already written when the entry becomes valid

	// upper 32 bit
	entry->virtual_segment_id = virtualSegmentID;
	entry->hash = secondaryHash;
	entry->abbr_page_index = ((uint32)virtualAddress >> 22) & 0x3f;
	entry->valid = true;
}


static void
map_page(void *virtualAddress, void *physicalAddress, uint8 mode)
{
	uint32 virtualSegmentID = get_sr(virtualAddress) & 0xffffff;

	uint32 hash = page_table_entry::PrimaryHash(virtualSegmentID, (uint32)virtualAddress);
	page_table_entry_group *group = &sPageTable[hash & sPageTableHashMask];

	for (int32 i = 0; i < 8; i++) {
		// 8 entries in a group
		if (group->entry[i].valid)
			continue;

		fill_page_table_entry(&group->entry[i], virtualSegmentID, virtualAddress, physicalAddress, mode, false);
		return;
	}

	hash = page_table_entry::SecondaryHash(hash);
	group = &sPageTable[hash & sPageTableHashMask];

	for (int32 i = 0; i < 8; i++) {
		if (group->entry[i].valid)
			continue;

		fill_page_table_entry(&group->entry[i], virtualSegmentID, virtualAddress, physicalAddress, mode, true);
		return;
	}
}


static void
map_range(void *virtualAddress, void *physicalAddress, size_t size, uint8 mode)
{
	for (uint32 offset = 0; offset < size; offset += B_PAGE_SIZE) {
		map_page((void *)(uint32(virtualAddress) + offset), 
			(void *)(uint32(physicalAddress) + offset), mode);
	}
}


static status_t
find_allocated_ranges(void *pageTable, void **_physicalPageTable)
{
	// we have to preserve the OpenFirmware established mappings
	// if we want to continue to use its service after we've
	// taken over (we will probably need less translations once
	// we have proper driver support for the target hardware).
	int mmu;
	if (of_getprop(gChosen, "mmu", &mmu, sizeof(int)) == OF_FAILED)
		return B_ERROR;
	mmu = of_instance_to_package(mmu);

	struct translation_map {
		void	*virtual_address;
		int		length;
		void	*physical_address;
		int		mode;
	} translations[64];
	int length = of_getprop(mmu, "translations", &translations, sizeof(translations));
	if (length == OF_FAILED)
		printf("getting translations failed\n");
	length = length / sizeof(struct translation_map);
	uint32 total = 0;
	printf("found %d translations\n", length);

	for (int i = 0; i < length; i++) {
		struct translation_map *map = &translations[i];
		//printf("%i: map: %p, length %d -> physical: %p, mode %d\n", i, map->virtual_address, map->length, map->physical_address, map->mode);

		// insert range in physical allocated, if it points to physical memory

		if (is_physical_memory(map->physical_address)
			&& insert_physical_allocated_range(map->physical_address,
					map->length) < B_OK) {
			printf("cannot map physical allocated range (num ranges = %lu)!\n", gKernelArgs.num_physical_allocated_ranges);
			return B_ERROR;
		}

		if (map->virtual_address == pageTable) {
			puts("found page table!");
			*_physicalPageTable = map->physical_address;
		}

		// insert range in virtual allocated

		if (insert_virtual_allocated_range(map->virtual_address,
				map->length) < B_OK) {
			printf("cannot map virtual allocated range (num ranges = %lu)!\n", gKernelArgs.num_virtual_allocated_ranges);
		}

		// map range into the page table

		map_range(map->virtual_address, map->physical_address, map->length, map->mode);

		total += map->length;
	}
	//printf("total mapped: %lu\n", total);

	return B_OK;
}


/** Computes the recommended minimal page table size as
 *	described in table 7-22 of the PowerPC "Programming
 *	Environment for 32-Bit Microprocessors".
 *	The page table size ranges from 64 kB (for 8 MB RAM)
 *	to 32 MB (for 4 GB RAM).
 */

static size_t
suggested_page_table_size(size_t total)
{
	uint32 max = 23;	
		// 2^23 == 8 MB

	while (max < 32) {
		if (total <= (1UL << max))
			break;

		max++;
	}

	return 1UL << (max - 7);
		// 2^(23 - 7) == 64 kB
}


static void *
find_physical_memory_range(size_t size)
{
	for (uint32 i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
		if (gKernelArgs.physical_memory_range[i].size > size)
			return (void *)gKernelArgs.physical_memory_range[i].start;
	}
	return NULL;
}


static void *
find_free_physical_range(size_t size)
{
	// just do a simple linear search at the end of the allocated
	// ranges (dumb memory allocation)
	if (gKernelArgs.num_physical_allocated_ranges == 0) {
		if (gKernelArgs.num_physical_memory_ranges == 0)
			return NULL;

		return find_physical_memory_range(size);
	}

	for (uint32 i = 0; i < gKernelArgs.num_physical_allocated_ranges; i++) {
		void *address = (void *)(gKernelArgs.physical_allocated_range[i].start + gKernelArgs.physical_allocated_range[i].size);
		if (!is_physical_allocated(address, size) && is_physical_memory(address, size))
			return address;
	}
	return NULL;
}


static void *
find_free_virtual_range(size_t size)
{
	for (uint32 i = 0; i < gKernelArgs.num_virtual_allocated_ranges; i++) {
		void *address = (void *)(gKernelArgs.virtual_allocated_range[i].start + gKernelArgs.virtual_allocated_range[i].size);
		if (!is_virtual_allocated(address, size))
			return address;
	}
	return NULL;
}


extern "C" void *
arch_mmu_alloc_at(void *virtualAddress, size_t size, uint8 protection)
{
	// we only know page sizes
	size = ROUNDUP(size, B_PAGE_SIZE);

	// set protection to WIMGxPP: -I--xPP
	if (protection & B_WRITE_AREA)
		protection = 0x23;
	else
		protection = 0x21;

	if (virtualAddress == NULL) {
		// find free address large enough to hold "size"
		virtualAddress = find_free_virtual_range(size);
		if (virtualAddress == NULL)
			return NULL;
	} else {
		if (is_virtual_allocated(virtualAddress, size))
			return NULL;
	}

	// we have a free virtual range for the allocation, now
	// have a look for free physical memory as well (we assume
	// that a) there is enough memory, and b) failing is fatal
	// so that we don't have to optimize for these cases :)
	
	void *physicalAddress = find_free_physical_range(size);
	if (physicalAddress == NULL)
		return NULL;

	// everything went fine, so lets mark the space as used.

printf("mmu_alloc: va %p, pa %p, size %u\n", virtualAddress, physicalAddress, size);
	insert_virtual_allocated_range(virtualAddress, size);
	insert_physical_allocated_range(physicalAddress, size);

	map_range(virtualAddress, physicalAddress, size, protection);

	return virtualAddress;
}


extern "C" void *
arch_mmu_alloc(size_t size, uint8 protection)
{
	return arch_mmu_alloc_at(NULL, size, protection);
}


extern "C" status_t
arch_mmu_init(void)
{
	// get map of physical memory (fill in kernel_args structure)
	
	size_t total;
	if (find_physical_memory_ranges(total) < B_OK)
		return B_ERROR;
	printf("total physical memory = %u MB\n", total / (1024*1024));

	// get OpenFirmware's current page table

	void *table;
	size_t tableSize;
	ppc_get_page_table(&table, &tableSize);
	printf("-> table = %p, size = %u\n", table, tableSize);
	if (table == NULL && tableSize == 0) {
		puts("OpenFirmware is in real addressing mode!");
	}

	// can we just keep the page table?
	size_t suggestedTableSize = suggested_page_table_size(total);
	printf("suggested page table size = %u\n", suggestedTableSize);
	if (tableSize < suggestedTableSize) {
		// nah, we need a new one!
		printf("need new page table, size = %u!\n", suggestedTableSize);
		table = of_claim(0, suggestedTableSize, suggestedTableSize);
			// KERNEL_BASE would be better as virtual address, but
			// at least with Apple's OpenFirmware, it makes no
			// difference - we will have to remap it later
		if (table == (void *)OF_FAILED)
			return B_NO_MEMORY;
		printf("new table at: %p\n", table);
		sPageTable = (page_table_entry_group *)table;
		sPageTableHashMask = (suggestedTableSize >> 6) - 1;
	} else {
		// ToDo: we could check if the page table is much too large
		//	and create a smaller one in this case (in order to save 
		//	memory).
		sPageTable = (page_table_entry_group *)table;
		sPageTableHashMask = (tableSize >> 6) - 1;
	}

	// find already allocated ranges of physical memory
	// and the virtual address space

	void *physicalTable;
	if (find_allocated_ranges(table, &physicalTable) < B_OK)
		return B_ERROR;

	// ToDo: take over control of MMU

	return B_OK;
}

