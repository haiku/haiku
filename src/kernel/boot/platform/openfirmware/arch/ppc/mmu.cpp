/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "openfirmware.h"

#include <platform_arch.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>
#include <arch_cpu.h>
#include <arch_mmu.h>
#include <kernel.h>

#include <OS.h>


segment_descriptor sSegments[16];
page_table_entry_group *sPageTable;
uint32 sPageTableHashMask;


static void
remove_range_index(address_range *ranges, uint32 &numRanges, uint32 index)
{
	if (index + 1 == numRanges) {
		// remove last range
		numRanges--;
		return;
	}

	memmove(&ranges[index], &ranges[index + 1], sizeof(address_range) * (numRanges - 1 - index));
	numRanges--;
}


static status_t
insert_memory_range(address_range *ranges, uint32 &numRanges, uint32 maxRanges,
	const void *_start, uint32 size)
{
	addr_t start = ROUNDOWN(addr_t(_start), B_PAGE_SIZE);
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
		return B_OK;
	}

	// no range matched, we need to create a new one
	
	if (numRanges >= maxRanges)
		return B_ENTRY_NOT_FOUND;

	ranges[numRanges].start = (addr_t)start;
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
			printf("%ld: empty region\n", i);
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
	addr_t start = (addr_t)address;
	addr_t end = start + size;

	for (uint32 i = 0; i < numRanges; i++) {
		addr_t rangeStart = ranges[i].start;
		addr_t rangeEnd = rangeStart + ranges[i].size;

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
	entry->secondary_hash = secondaryHash;
	entry->abbr_page_index = ((uint32)virtualAddress >> 22) & 0x3f;
	entry->valid = true;
}


static void
map_page(void *virtualAddress, void *physicalAddress, uint8 mode)
{
	uint32 virtualSegmentID = sSegments[addr_t(virtualAddress) >> 28].virtual_segment_id;

	uint32 hash = page_table_entry::PrimaryHash(virtualSegmentID, (uint32)virtualAddress);
	page_table_entry_group *group = &sPageTable[hash & sPageTableHashMask];

	for (int32 i = 0; i < 8; i++) {
		// 8 entries in a group
		if (group->entry[i].valid)
			continue;

		fill_page_table_entry(&group->entry[i], virtualSegmentID, virtualAddress, physicalAddress, mode, false);
		//printf("map: va = %p -> %p, mode = %d, hash = %lu\n", virtualAddress, physicalAddress, mode, hash);
		return;
	}

	hash = page_table_entry::SecondaryHash(hash);
	group = &sPageTable[hash & sPageTableHashMask];

	for (int32 i = 0; i < 8; i++) {
		if (group->entry[i].valid)
			continue;

		fill_page_table_entry(&group->entry[i], virtualSegmentID, virtualAddress, physicalAddress, mode, true);
		//printf("map: va = %p -> %p, mode = %d, second hash = %lu\n", virtualAddress, physicalAddress, mode, hash);
		return;
	}

	panic("out of page table entries! (you would think this could not happen in a boot loader...)\n");
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
find_allocated_ranges(void *pageTable, page_table_entry_group **_physicalPageTable,
	void **_exceptionHandlers)
{
	// we have to preserve the OpenFirmware established mappings
	// if we want to continue to use its service after we've
	// taken over (we will probably need less translations once
	// we have proper driver support for the target hardware).
	int mmu;
	if (of_getprop(gChosen, "mmu", &mmu, sizeof(int)) == OF_FAILED) {
		puts("no OF mmu");
		return B_ERROR;
	}
	mmu = of_instance_to_package(mmu);

	struct translation_map {
		void	*virtual_address;
		int		length;
		void	*physical_address;
		int		mode;
	} translations[64];
	int length = of_getprop(mmu, "translations", &translations, sizeof(translations));
	if (length == OF_FAILED) {
		puts("no OF translations");
		return B_ERROR;
	}
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
			*_physicalPageTable = (page_table_entry_group *)map->physical_address;
		}
		if ((addr_t)map->physical_address <= 0x100 
			&& (addr_t)map->physical_address + map->length >= 0x1000) {
			puts("found exception handlers!");
			*_exceptionHandlers = map->virtual_address;
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
arch_mmu_allocate(void *virtualAddress, size_t size, uint8 protection)
{
	// we only know page sizes
	size = ROUNDUP(size, B_PAGE_SIZE);

	// set protection to WIMGxPP: -I--xPP
	if (protection & B_WRITE_AREA)
		protection = 0x23;
	else
		protection = 0x22;

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


extern "C" status_t
arch_mmu_free(void *address, size_t size)
{
	// ToDo: implement freeing a region!
	return B_OK;
}


static inline void
invalidate_tlb(void)
{
	//asm volatile("tlbia");
		// "tlbia" is obviously not available on every CPU...

	// Note: this flushes the whole 4 GB address space - it
	//		would probably be a good idea to do less here

	addr_t address = 0;
	for (uint32 i = 0; i < 0x100000; i++) {
		asm volatile("tlbie %0" :: "r" (address));
		address += B_PAGE_SIZE;
	}
	tlbsync();
}


extern "C" status_t
arch_mmu_init(void)
{
	// get map of physical memory (fill in kernel_args structure)
	
	size_t total;
	if (find_physical_memory_ranges(total) < B_OK) {
		puts("could not find physical memory ranges!");
		return B_ERROR;
	}
	printf("total physical memory = %u MB\n", total / (1024*1024));

	// get OpenFirmware's current page table

	page_table_entry_group *table;
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
		table = (page_table_entry_group *)of_claim(0, suggestedTableSize, suggestedTableSize);
			// KERNEL_BASE would be better as virtual address, but
			// at least with Apple's OpenFirmware, it makes no
			// difference - we will have to remap it later
		if (table == (void *)OF_FAILED) {
			panic("Could not allocate new page table (size = %ld)!!\n", suggestedTableSize);
			return B_NO_MEMORY;
		}
		printf("new table at: %p\n", table);
		sPageTable = (page_table_entry_group *)table;
		tableSize = suggestedTableSize;
	} else {
		// ToDo: we could check if the page table is much too large
		//	and create a smaller one in this case (in order to save 
		//	memory).
		sPageTable = (page_table_entry_group *)table;
	}
	sPageTableHashMask = tableSize / sizeof(page_table_entry_group) - 1;
	memset(sPageTable, 0, tableSize);

	// set OpenFirmware callbacks - it will ask us for memory after that
	// instead of maintaining it itself

	// ToDo: !

	// turn off address translation via the page table/segment mechanism,
	// identity map the first 256 MB (where our code/data reside)

	printf("MSR: %p\n", (void *)get_msr());

//	block_address_translation bat;

/*	bat.length = BAT_LENGTH_256MB;
	bat.kernel_valid = true;
	bat.memory_coherent = true;
	bat.protection = BAT_READ_WRITE;

	set_ibat0(&bat);
	set_dbat0(&bat);
	isync();
puts("2");*/

	// initialize segment descriptors, but don't set the registers
	// until we're about to take over the page table - we're mapping
	// pages into our table using these values

	for (int32 i = 0; i < 16; i++)
		sSegments[i].virtual_segment_id = i;

	// find already allocated ranges of physical memory
	// and the virtual address space

	page_table_entry_group *physicalTable;
	void *exceptionHandlers = (void *)-1;
	if (find_allocated_ranges(table, &physicalTable, &exceptionHandlers) < B_OK) {
		puts("find_allocated_ranges() failed!");
		//return B_ERROR;
	}

	if (exceptionHandlers == (void *)-1) {
		// ToDo: create mapping for the exception handlers
		puts("no mapping for the exception handlers!");
	}

	// set up new page table and turn on translation again

	for (int32 i = 0; i < 16; i++) {
		ppc_set_segment_register((void *)(i * 0x10000000), sSegments[i]);
			// one segment describes 256 MB of memory
	}

	ppc_set_page_table(physicalTable, tableSize);
	invalidate_tlb();

	// clear BATs
	reset_ibats();
	reset_dbats();

	set_msr(MSR_MACHINE_CHECK_ENABLED | MSR_FP_AVAILABLE 
			| MSR_INST_ADDRESS_TRANSLATION 
			| MSR_DATA_ADDRESS_TRANSLATION);

	// set kernel args

	printf("virt_allocated: %lu\n", gKernelArgs.num_virtual_allocated_ranges);
	printf("phys_allocated: %lu\n", gKernelArgs.num_physical_allocated_ranges);
	printf("phys_memory: %lu\n", gKernelArgs.num_physical_memory_ranges);

	gKernelArgs.arch_args.page_table.start = (addr_t)sPageTable;
	gKernelArgs.arch_args.page_table.size = tableSize;

	gKernelArgs.arch_args.exception_handlers.start = (addr_t)exceptionHandlers;
	gKernelArgs.arch_args.exception_handlers.size = B_PAGE_SIZE;

	return B_OK;
}

