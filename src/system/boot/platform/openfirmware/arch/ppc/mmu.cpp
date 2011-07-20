/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <platform_arch.h>
#include <boot/addr_range.h>
#include <boot/kernel_args.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>
#include <platform/openfirmware/openfirmware.h>
#include <arch_cpu.h>
#include <arch_mmu.h>
#include <kernel.h>


// set protection to WIMGNPP: -----PP
// PP:	00 - no access
//		01 - read only
//		10 - read/write
//		11 - read only
#define PAGE_READ_ONLY	0x01
#define PAGE_READ_WRITE	0x02

// NULL is actually a possible physical address...
//#define PHYSINVAL ((void *)-1)
#define PHYSINVAL NULL

//#define TRACE_MMU
#ifdef TRACE_MMU
#   define TRACE(x...) dprintf(x)
#else
#   define TRACE(x...) ;
#endif


segment_descriptor sSegments[16];
page_table_entry_group *sPageTable;
uint32 sPageTableHashMask;


// begin and end of the boot loader
extern "C" uint8 __text_begin;
extern "C" uint8 _end;


static status_t
insert_virtual_range_to_keep(void *start, uint32 size)
{
	return insert_address_range(gKernelArgs.arch_args.virtual_ranges_to_keep,
		&gKernelArgs.arch_args.num_virtual_ranges_to_keep,
		MAX_VIRTUAL_RANGES_TO_KEEP, (addr_t)start, size);
}


static status_t
remove_virtual_range_to_keep(void *start, uint32 size)
{
	return remove_address_range(gKernelArgs.arch_args.virtual_ranges_to_keep,
		&gKernelArgs.arch_args.num_virtual_ranges_to_keep,
		MAX_VIRTUAL_RANGES_TO_KEEP, (addr_t)start, size);
}


static status_t
find_physical_memory_ranges(size_t &total)
{
	int memory, package;
	dprintf("checking for memory...\n");
	if (of_getprop(gChosen, "memory", &memory, sizeof(int)) == OF_FAILED)
		return B_ERROR;
	package = of_instance_to_package(memory);

	total = 0;

	struct of_region regions[64];
	int count;
	count = of_getprop(package, "reg", regions, sizeof(regions));
	if (count == OF_FAILED)
		count = of_getprop(memory, "reg", regions, sizeof(regions));
	if (count == OF_FAILED)
		return B_ERROR;
	count /= sizeof(of_region);

	for (int32 i = 0; i < count; i++) {
		if (regions[i].size <= 0) {
			dprintf("%ld: empty region\n", i);
			continue;
		}
		dprintf("%" B_PRIu32 ": base = %p, size = %" B_PRIu32 "\n", i,
			regions[i].base, regions[i].size);

		total += regions[i].size;

		if (insert_physical_memory_range((addr_t)regions[i].base,
				regions[i].size) != B_OK) {
			dprintf("cannot map physical memory range "
				"(num ranges = %" B_PRIu32 ")!\n",
				gKernelArgs.num_physical_memory_ranges);
			return B_ERROR;
		}
	}

	return B_OK;
}


static bool
is_virtual_allocated(void *address, size_t size)
{
	addr_t foundBase;
	return !get_free_address_range(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges, (addr_t)address, size,
		&foundBase) || foundBase != (addr_t)address;
}


static bool
is_physical_allocated(void *address, size_t size)
{
	phys_addr_t foundBase;
	return !get_free_physical_address_range(
		gKernelArgs.physical_allocated_range,
		gKernelArgs.num_physical_allocated_ranges, (addr_t)address, size,
		&foundBase) || foundBase != (addr_t)address;
}


static bool
is_physical_memory(void *address, size_t size)
{
	return is_physical_address_range_covered(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges, (addr_t)address, size);
}


static bool
is_physical_memory(void *address)
{
	return is_physical_memory(address, 1);
}


static void
fill_page_table_entry(page_table_entry *entry, uint32 virtualSegmentID,
	void *virtualAddress, void *physicalAddress, uint8 mode, bool secondaryHash)
{
	// lower 32 bit - set at once
	((uint32 *)entry)[1]
		= (((uint32)physicalAddress / B_PAGE_SIZE) << 12) | mode;
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
	uint32 virtualSegmentID
		= sSegments[addr_t(virtualAddress) >> 28].virtual_segment_id;

	uint32 hash = page_table_entry::PrimaryHash(virtualSegmentID,
		(uint32)virtualAddress);
	page_table_entry_group *group = &sPageTable[hash & sPageTableHashMask];

	for (int32 i = 0; i < 8; i++) {
		// 8 entries in a group
		if (group->entry[i].valid)
			continue;

		fill_page_table_entry(&group->entry[i], virtualSegmentID,
			virtualAddress, physicalAddress, mode, false);
		//TRACE("map: va = %p -> %p, mode = %d, hash = %lu\n",
		//	virtualAddress, physicalAddress, mode, hash);
		return;
	}

	hash = page_table_entry::SecondaryHash(hash);
	group = &sPageTable[hash & sPageTableHashMask];

	for (int32 i = 0; i < 8; i++) {
		if (group->entry[i].valid)
			continue;

		fill_page_table_entry(&group->entry[i], virtualSegmentID,
			virtualAddress, physicalAddress, mode, true);
		//TRACE("map: va = %p -> %p, mode = %d, second hash = %lu\n",
		//	virtualAddress, physicalAddress, mode, hash);
		return;
	}

	panic("%s: out of page table entries!\n", __func__);
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
find_allocated_ranges(void *oldPageTable, void *pageTable,
	page_table_entry_group **_physicalPageTable, void **_exceptionHandlers)
{
	// we have to preserve the OpenFirmware established mappings
	// if we want to continue to use its service after we've
	// taken over (we will probably need less translations once
	// we have proper driver support for the target hardware).
	int mmu;
	if (of_getprop(gChosen, "mmu", &mmu, sizeof(int)) == OF_FAILED) {
		dprintf("%s: Error: no OpenFirmware mmu\n", __func__);
		return B_ERROR;
	}
	mmu = of_instance_to_package(mmu);

	struct translation_map {
		void	*virtual_address;
		int		length;
		void	*physical_address;
		int		mode;
	} translations[64];

	int length = of_getprop(mmu, "translations", &translations,
		sizeof(translations));
	if (length == OF_FAILED) {
		dprintf("Error: no OF translations.\n");
		return B_ERROR;
	}
	length = length / sizeof(struct translation_map);
	uint32 total = 0;
	dprintf("found %d translations\n", length);

	for (int i = 0; i < length; i++) {
		struct translation_map *map = &translations[i];
		bool keepRange = true;
		TRACE("%i: map: %p, length %d -> physical: %p, mode %d\n", i,
			map->virtual_address, map->length,
			map->physical_address, map->mode);

		// insert range in physical allocated, if it points to physical memory

		if (is_physical_memory(map->physical_address)
			&& insert_physical_allocated_range((addr_t)map->physical_address,
				map->length) != B_OK) {
			dprintf("cannot map physical allocated range "
				"(num ranges = %" B_PRIu32 ")!\n",
				gKernelArgs.num_physical_allocated_ranges);
			return B_ERROR;
		}

		if (map->virtual_address == pageTable) {
			dprintf("%i: found page table at va %p\n", i,
				map->virtual_address);
			*_physicalPageTable
				= (page_table_entry_group *)map->physical_address;
			keepRange = false;
				// we keep it explicitely anyway
		}
		if ((addr_t)map->physical_address <= 0x100
			&& (addr_t)map->physical_address + map->length >= 0x1000) {
			dprintf("%i: found exception handlers at va %p\n", i,
				map->virtual_address);
			*_exceptionHandlers = map->virtual_address;
			keepRange = false;
				// we keep it explicitely anyway
		}
		if (map->virtual_address == oldPageTable)
			keepRange = false;

		// insert range in virtual allocated

		if (insert_virtual_allocated_range((addr_t)map->virtual_address,
				map->length) != B_OK) {
			dprintf("cannot map virtual allocated range "
				"(num ranges = %" B_PRIu32 ")!\n",
				gKernelArgs.num_virtual_allocated_ranges);
		}

		// map range into the page table

		map_range(map->virtual_address, map->physical_address, map->length,
			map->mode);

		// insert range in virtual ranges to keep

		if (keepRange) {
			TRACE("%i: keeping free range starting at va %p\n", i,
				map->virtual_address);

			if (insert_virtual_range_to_keep(map->virtual_address,
					map->length) != B_OK) {
				dprintf("cannot map virtual range to keep "
					"(num ranges = %" B_PRIu32 ")\n",
					gKernelArgs.num_virtual_allocated_ranges);
			}
		}

		total += map->length;
	}
	dprintf("total size kept: %" B_PRIu32 "\n", total);

	// remove the boot loader code from the virtual ranges to keep in the
	// kernel
	if (remove_virtual_range_to_keep(&__text_begin, &_end - &__text_begin)
			!= B_OK) {
		dprintf("%s: Failed to remove boot loader range "
			"from virtual ranges to keep.\n", __func__);
	}

	return B_OK;
}


/*!	Computes the recommended minimal page table size as
	described in table 7-22 of the PowerPC "Programming
	Environment for 32-Bit Microprocessors".
	The page table size ranges from 64 kB (for 8 MB RAM)
	to 32 MB (for 4 GB RAM).
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
	return PHYSINVAL;
}


static void *
find_free_physical_range(size_t size)
{
	// just do a simple linear search at the end of the allocated
	// ranges (dumb memory allocation)
	if (gKernelArgs.num_physical_allocated_ranges == 0) {
		if (gKernelArgs.num_physical_memory_ranges == 0)
			return PHYSINVAL;

		return find_physical_memory_range(size);
	}

	for (uint32 i = 0; i < gKernelArgs.num_physical_allocated_ranges; i++) {
		void *address = (void *)(gKernelArgs.physical_allocated_range[i].start
			+ gKernelArgs.physical_allocated_range[i].size);
		if (!is_physical_allocated(address, size)
			&& is_physical_memory(address, size))
			return address;
	}
	return PHYSINVAL;
}


static void *
find_free_virtual_range(void *base, size_t size)
{
	if (base && !is_virtual_allocated(base, size))
		return base;

	void *firstFound = NULL;
	void *firstBaseFound = NULL;
	for (uint32 i = 0; i < gKernelArgs.num_virtual_allocated_ranges; i++) {
		void *address = (void *)(gKernelArgs.virtual_allocated_range[i].start
			+ gKernelArgs.virtual_allocated_range[i].size);
		if (!is_virtual_allocated(address, size)) {
			if (!base)
				return address;

			if (firstFound == NULL)
				firstFound = address;
			if (address >= base
				&& (firstBaseFound == NULL || address < firstBaseFound)) {
				firstBaseFound = address;
			}
		}
	}
	return (firstBaseFound ? firstBaseFound : firstFound);
}


extern "C" void *
arch_mmu_allocate(void *_virtualAddress, size_t size, uint8 _protection,
	bool exactAddress)
{
	// we only know page sizes
	size = ROUNDUP(size, B_PAGE_SIZE);

	uint8 protection = 0;
	if (_protection & B_WRITE_AREA)
		protection = PAGE_READ_WRITE;
	else
		protection = PAGE_READ_ONLY;

	// If no address is given, use the KERNEL_BASE as base address, since
	// that avoids trouble in the kernel, when we decide to keep the region.
	void *virtualAddress = _virtualAddress;
	if (!virtualAddress)
		virtualAddress = (void*)KERNEL_BASE;

	// find free address large enough to hold "size"
	virtualAddress = find_free_virtual_range(virtualAddress, size);
	if (virtualAddress == NULL)
		return NULL;

	// fail if the exact address was requested, but is not free
	if (exactAddress && _virtualAddress && virtualAddress != _virtualAddress) {
		dprintf("arch_mmu_allocate(): exact address requested, but virtual "
			"range (base: %p, size: %" B_PRIuSIZE ") is not free.\n",
			_virtualAddress, size);
		return NULL;
	}

	// we have a free virtual range for the allocation, now
	// have a look for free physical memory as well (we assume
	// that a) there is enough memory, and b) failing is fatal
	// so that we don't have to optimize for these cases :)

	void *physicalAddress = find_free_physical_range(size);
	if (physicalAddress == PHYSINVAL) {
		dprintf("arch_mmu_allocate(base: %p, size: %" B_PRIuSIZE ") "
			"no free physical address\n", virtualAddress, size);
		return NULL;
	}

	// everything went fine, so lets mark the space as used.

	dprintf("mmu_alloc: va %p, pa %p, size %" B_PRIuSIZE "\n", virtualAddress,
		physicalAddress, size);
	insert_virtual_allocated_range((addr_t)virtualAddress, size);
	insert_physical_allocated_range((addr_t)physicalAddress, size);

	map_range(virtualAddress, physicalAddress, size, protection);

	return virtualAddress;
}


extern "C" status_t
arch_mmu_free(void *address, size_t size)
{
	// TODO: implement freeing a region!
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
		asm volatile("tlbie %0" : : "r" (address));
		address += B_PAGE_SIZE;
	}
	tlbsync();
}


//	#pragma mark - OpenFirmware callbacks and public API


static int
map_callback(struct of_arguments *args)
{
	void *physicalAddress = (void *)args->Argument(0);
	void *virtualAddress = (void *)args->Argument(1);
	int length = args->Argument(2);
	int mode = args->Argument(3);
	int &error = args->ReturnValue(0);

	// insert range in physical allocated if needed

	if (is_physical_memory(physicalAddress)
		&& insert_physical_allocated_range((addr_t)physicalAddress, length)
			!= B_OK) {
		error = -1;
		return OF_FAILED;
	}

	// insert range in virtual allocated

	if (insert_virtual_allocated_range((addr_t)virtualAddress, length)
			!= B_OK) {
		error = -2;
		return OF_FAILED;
	}

	// map range into the page table

	map_range(virtualAddress, physicalAddress, length, mode);

	return B_OK;
}


static int
unmap_callback(struct of_arguments *args)
{
/*	void *address = (void *)args->Argument(0);
	int length = args->Argument(1);
	int &error = args->ReturnValue(0);
*/
	// TODO: to be implemented

	return OF_FAILED;
}


static int
translate_callback(struct of_arguments *args)
{
	addr_t virtualAddress = (addr_t)args->Argument(0);
	int &error = args->ReturnValue(0);
	int &physicalAddress = args->ReturnValue(1);
	int &mode = args->ReturnValue(2);

	// Find page table entry for this address

	uint32 virtualSegmentID
		= sSegments[addr_t(virtualAddress) >> 28].virtual_segment_id;

	uint32 hash = page_table_entry::PrimaryHash(virtualSegmentID,
		(uint32)virtualAddress);
	page_table_entry_group *group = &sPageTable[hash & sPageTableHashMask];
	page_table_entry *entry = NULL;

	for (int32 i = 0; i < 8; i++) {
		entry = &group->entry[i];

		if (entry->valid
			&& entry->virtual_segment_id == virtualSegmentID
			&& entry->secondary_hash == false
			&& entry->abbr_page_index == ((virtualAddress >> 22) & 0x3f))
			goto success;
	}

	hash = page_table_entry::SecondaryHash(hash);
	group = &sPageTable[hash & sPageTableHashMask];

	for (int32 i = 0; i < 8; i++) {
		entry = &group->entry[i];

		if (entry->valid
			&& entry->virtual_segment_id == virtualSegmentID
			&& entry->secondary_hash == true
			&& entry->abbr_page_index == ((virtualAddress >> 22) & 0x3f))
			goto success;
	}

	// could not find the translation
	error = B_ENTRY_NOT_FOUND;
	return OF_FAILED;

success:
	// we found the entry in question
	physicalAddress = (int)(entry->physical_page_number * B_PAGE_SIZE);
	mode = (entry->write_through << 6)		// WIMGxPP
		| (entry->caching_inhibited << 5)
		| (entry->memory_coherent << 4)
		| (entry->guarded << 3)
		| entry->page_protection;
	error = B_OK;

	return B_OK;
}


static int
alloc_real_mem_callback(struct of_arguments *args)
{
/*	addr_t minAddress = (addr_t)args->Argument(0);
	addr_t maxAddress = (addr_t)args->Argument(1);
	int length = args->Argument(2);
	int mode = args->Argument(3);
	int &error = args->ReturnValue(0);
	int &physicalAddress = args->ReturnValue(1);
*/
	// ToDo: to be implemented

	return OF_FAILED;
}


/** Dispatches the callback to the responsible function */

static int
callback(struct of_arguments *args)
{
	const char *name = args->name;
	TRACE("OF CALLBACK: %s\n", name);

	if (!strcmp(name, "map"))
		return map_callback(args);
	else if (!strcmp(name, "unmap"))
		return unmap_callback(args);
	else if (!strcmp(name, "translate"))
		return translate_callback(args);
	else if (!strcmp(name, "alloc-real-mem"))
		return alloc_real_mem_callback(args);

	return OF_FAILED;
}


extern "C" status_t
arch_set_callback(void)
{
	// set OpenFirmware callbacks - it will ask us for memory after that
	// instead of maintaining it itself

	void *oldCallback = NULL;
	if (of_call_client_function("set-callback", 1, 1, &callback, &oldCallback)
			== OF_FAILED) {
		dprintf("Error: OpenFirmware set-callback failed\n");
		return B_ERROR;
	}
	TRACE("old callback = %p; new callback = %p\n", oldCallback, callback);

	return B_OK;
}


extern "C" status_t
arch_mmu_init(void)
{
	// get map of physical memory (fill in kernel_args structure)

	size_t total;
	if (find_physical_memory_ranges(total) != B_OK) {
		dprintf("Error: could not find physical memory ranges!\n");
		return B_ERROR;
	}
	dprintf("total physical memory = %" B_PRId32 "MB\n", total / (1024 * 1024));

	// get OpenFirmware's current page table

	page_table_entry_group *oldTable;
	page_table_entry_group *table;
	size_t tableSize;
	ppc_get_page_table(&table, &tableSize);

	oldTable = table;

	bool realMode = false;

	// TODO: read these values out of the OF settings
	// NOTE: I've only ever seen -1 (0xffffffff) for these values in
	//       OpenFirmware.. even after loading the bootloader -- Alex
	addr_t realBase = 0;
	addr_t realSize = 0x400000;

	// can we just keep the page table?
	size_t suggestedTableSize = suggested_page_table_size(total);
	dprintf("suggested page table size = %" B_PRIuSIZE "\n",
		suggestedTableSize);
	if (tableSize < suggestedTableSize) {
		// nah, we need a new one!
		dprintf("need new page table, size = %" B_PRIuSIZE "!\n",
			suggestedTableSize);
		table = (page_table_entry_group *)of_claim(NULL, suggestedTableSize,
			suggestedTableSize);
			// KERNEL_BASE would be better as virtual address, but
			// at least with Apple's OpenFirmware, it makes no
			// difference - we will have to remap it later
		if (table == (void *)OF_FAILED) {
			panic("Could not allocate new page table "
				"(size = %" B_PRIuSIZE ")!!\n", suggestedTableSize);
			return B_NO_MEMORY;
		}
		if (table == NULL) {
			// work-around for the broken Pegasos OpenFirmware
			dprintf("broken OpenFirmware detected (claim doesn't work)\n");
			realMode = true;

			addr_t tableBase = 0;
			for (int32 i = 0; tableBase < realBase + realSize * 3; i++) {
				tableBase = suggestedTableSize * i;
			}

			table = (page_table_entry_group *)tableBase;
		}

		dprintf("new table at: %p\n", table);
		sPageTable = table;
		tableSize = suggestedTableSize;
	} else {
		// ToDo: we could check if the page table is much too large
		//	and create a smaller one in this case (in order to save
		//	memory).
		sPageTable = table;
	}

	sPageTableHashMask = tableSize / sizeof(page_table_entry_group) - 1;
	if (sPageTable != oldTable)
		memset(sPageTable, 0, tableSize);

	// turn off address translation via the page table/segment mechanism,
	// identity map the first 256 MB (where our code/data reside)

	dprintf("MSR: %p\n", (void *)get_msr());

	#if 0
	block_address_translation bat;

	bat.length = BAT_LENGTH_256MB;
	bat.kernel_valid = true;
	bat.memory_coherent = true;
	bat.protection = BAT_READ_WRITE;

	set_ibat0(&bat);
	set_dbat0(&bat);
	isync();
	#endif

	// initialize segment descriptors, but don't set the registers
	// until we're about to take over the page table - we're mapping
	// pages into our table using these values

	for (int32 i = 0; i < 16; i++)
		sSegments[i].virtual_segment_id = i;

	// find already allocated ranges of physical memory
	// and the virtual address space

	page_table_entry_group *physicalTable = NULL;
	void *exceptionHandlers = (void *)-1;
	if (find_allocated_ranges(oldTable, table, &physicalTable,
			&exceptionHandlers) != B_OK) {
		dprintf("Error: find_allocated_ranges() failed\n");
		return B_ERROR;
	}

#if 0
	block_address_translation bats[8];
	getibats(bats);
	for (int32 i = 0; i < 8; i++) {
		printf("page index %u, length %u, ppn %u\n", bats[i].page_index,
			bats[i].length, bats[i].physical_block_number);
	}
#endif

	if (physicalTable == NULL) {
		dprintf("%s: Didn't find physical address of page table\n", __func__);
		if (!realMode)
			return B_ERROR;

		// Pegasos work-around
		#if 0
		map_range((void *)realBase, (void *)realBase,
			realSize * 2, PAGE_READ_WRITE);
		map_range((void *)(total - realSize), (void *)(total - realSize),
			realSize, PAGE_READ_WRITE);
		map_range((void *)table, (void *)table, tableSize, PAGE_READ_WRITE);
		#endif
		insert_physical_allocated_range(realBase, realSize * 2);
		insert_virtual_allocated_range(realBase, realSize * 2);
		insert_physical_allocated_range(total - realSize, realSize);
		insert_virtual_allocated_range(total - realSize, realSize);
		insert_physical_allocated_range((addr_t)table, tableSize);
		insert_virtual_allocated_range((addr_t)table, tableSize);

		// QEMU OpenHackware work-around
		insert_physical_allocated_range(0x05800000, 0x06000000 - 0x05800000);
		insert_virtual_allocated_range(0x05800000, 0x06000000 - 0x05800000);

		physicalTable = table;
	}

	if (exceptionHandlers == (void *)-1) {
		// TODO: create mapping for the exception handlers
		dprintf("Error: no mapping for the exception handlers!\n");
	}

	// Set the Open Firmware memory callback. From now on the Open Firmware
	// will ask us for memory.
	arch_set_callback();

	// set up new page table and turn on translation again

	for (int32 i = 0; i < 16; i++) {
		ppc_set_segment_register((void *)(i * 0x10000000), sSegments[i]);
			// one segment describes 256 MB of memory
	}

	ppc_set_page_table(physicalTable, tableSize);
	invalidate_tlb();

	if (!realMode) {
		// clear BATs
		reset_ibats();
		reset_dbats();
		ppc_sync();
		isync();
	}

	set_msr(MSR_MACHINE_CHECK_ENABLED | MSR_FP_AVAILABLE
		| MSR_INST_ADDRESS_TRANSLATION | MSR_DATA_ADDRESS_TRANSLATION);

	// set kernel args

	dprintf("virt_allocated: %" B_PRIu32 "\n",
		gKernelArgs.num_virtual_allocated_ranges);
	dprintf("phys_allocated: %" B_PRIu32 "\n",
		gKernelArgs.num_physical_allocated_ranges);
	dprintf("phys_memory: %" B_PRIu32 "\n",
		gKernelArgs.num_physical_memory_ranges);

	gKernelArgs.arch_args.page_table.start = (addr_t)sPageTable;
	gKernelArgs.arch_args.page_table.size = tableSize;

	gKernelArgs.arch_args.exception_handlers.start = (addr_t)exceptionHandlers;
	gKernelArgs.arch_args.exception_handlers.size = B_PAGE_SIZE;

	return B_OK;
}

