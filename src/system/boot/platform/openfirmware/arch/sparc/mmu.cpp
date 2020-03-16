/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010-2011, Haiku, Inc. All Rights Reserved.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de.
 *		Alexander von Gluck, kallisti5@unixzen.com
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

#include "support.h"


#define PAGE_READ_ONLY	0x0002
#define PAGE_READ_WRITE	0x0001

// NULL is actually a possible physical address, so use -1 (which is
// misaligned, so not a valid address) as the invalid physical address.
#define PHYSINVAL ((void *)-1)
//#define PHYSINVAL NULL

//#define TRACE_MMU
#ifdef TRACE_MMU
#   define TRACE(x...) dprintf(x)
#else
#   define TRACE(x...) ;
#endif


unsigned int sMmuInstance;
unsigned int sMemoryInstance;


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
	TRACE("checking for memory...\n");
	intptr_t package = of_instance_to_package(sMemoryInstance);

	total = 0;

	// Memory base addresses are provided in 32 or 64 bit flavors
	// #address-cells and #size-cells matches the number of 32-bit 'cells'
	// representing the length of the base address and size fields
	intptr_t root = of_finddevice("/");
	int32 regSizeCells = of_size_cells(root);
	if (regSizeCells == OF_FAILED) {
		dprintf("finding size of memory cells failed, assume 32-bit.\n");
		regSizeCells = 1;
	}

	int32 regAddressCells = of_address_cells(root);
	if (regAddressCells == OF_FAILED) {
		// Sun Netra T1-105 is missing this, but we can guess that if the size
		// is 64bit, the address also likely is.
		regAddressCells = regSizeCells;
	}

	if (regAddressCells != 2 || regSizeCells != 2) {
		panic("%s: Unsupported OpenFirmware cell count detected.\n"
		"Address Cells: %" B_PRId32 "; Size Cells: %" B_PRId32
		" (CPU > 64bit?).\n", __func__, regAddressCells, regSizeCells);
		return B_ERROR;
	}

	struct of_region<uint64, uint64> regions[64];
	int count = of_getprop(package, "reg", regions, sizeof(regions));
	if (count == OF_FAILED)
		count = of_getprop(sMemoryInstance, "reg", regions, sizeof(regions));
	if (count == OF_FAILED)
		return B_ERROR;
	count /= sizeof(regions[0]);

	for (int32 i = 0; i < count; i++) {
		if (regions[i].size <= 0) {
			TRACE("%d: empty region\n", i);
			continue;
		}
		TRACE("%" B_PRIu32 ": base = %" B_PRIx64 ","
			"size = %" B_PRIx64 "\n", i, regions[i].base, regions[i].size);

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
	uint64 foundBase;
	return !get_free_address_range(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges, (addr_t)address, size,
		&foundBase) || foundBase != (addr_t)address;
}


static bool
is_physical_allocated(void *address, size_t size)
{
	uint64 foundBase;
	return !get_free_address_range(gKernelArgs.physical_allocated_range,
		gKernelArgs.num_physical_allocated_ranges, (addr_t)address, size,
		&foundBase) || foundBase != (addr_t)address;
}


static bool
is_physical_memory(void *address, size_t size = 1)
{
	return is_address_range_covered(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges, (addr_t)address, size);
}


static bool
map_range(void *virtualAddress, void *physicalAddress, size_t size, uint16 mode)
{
	// everything went fine, so lets mark the space as used.
	int status = of_call_method(sMmuInstance, "map", 5, 0, (uint64)mode, size,
		virtualAddress, 0, physicalAddress);

	if (status != 0) {
		dprintf("map_range(base: %p, size: %" B_PRIuSIZE ") "
			"mapping failed\n", virtualAddress, size);
		return false;
	}

	return true;
}


static status_t
find_allocated_ranges(void **_exceptionHandlers)
{
	// we have to preserve the OpenFirmware established mappings
	// if we want to continue to use its service after we've
	// taken over (we will probably need less translations once
	// we have proper driver support for the target hardware).
	intptr_t mmu = of_instance_to_package(sMmuInstance);

	struct translation_map {
		void *PhysicalAddress() {
			int64_t p = data;
#if 0
			// The openboot own "map?" word does not do this, so it must not
			// be needed
			// Sign extend
			p <<= 23;
			p >>= 23;
#endif

			// Keep only PA[40:13]
			// FIXME later CPUs have some more bits here
			p &= 0x000001FFFFFFE000ll;

			return (void*)p;
		}

		int16_t Mode() {
			int16_t mode;
			if (data & 2)
				mode = PAGE_READ_WRITE;
			else
				mode = PAGE_READ_ONLY;
			return mode;
		}

		void	*virtual_address;
		intptr_t length;
		intptr_t data;
	} translations[64];

	int length = of_getprop(mmu, "translations", &translations,
		sizeof(translations));
	if (length == OF_FAILED) {
		dprintf("Error: no OF translations.\n");
		return B_ERROR;
	}
	length = length / sizeof(struct translation_map);
	uint32 total = 0;
	TRACE("found %d translations\n", length);

	for (int i = 0; i < length; i++) {
		struct translation_map *map = &translations[i];
		bool keepRange = true;
		TRACE("%i: map: %p, length %ld -> phy %p mode %d: ", i,
			map->virtual_address, map->length,
			map->PhysicalAddress(), map->Mode());

		// insert range in physical allocated, if it points to physical memory

		if (is_physical_memory(map->PhysicalAddress())
			&& insert_physical_allocated_range((addr_t)map->PhysicalAddress(),
				map->length) != B_OK) {
			dprintf("cannot map physical allocated range "
				"(num ranges = %" B_PRIu32 ")!\n",
				gKernelArgs.num_physical_allocated_ranges);
			return B_ERROR;
		}

		// insert range in virtual allocated

		if (insert_virtual_allocated_range((addr_t)map->virtual_address,
				map->length) != B_OK) {
			dprintf("cannot map virtual allocated range "
				"(num ranges = %" B_PRIu32 ")!\n",
				gKernelArgs.num_virtual_allocated_ranges);
		}

		// insert range in virtual ranges to keep

		if (keepRange) {
			TRACE("keeping\n");

			if (insert_virtual_range_to_keep(map->virtual_address,
					map->length) != B_OK) {
				dprintf("cannot map virtual range to keep "
					"(num ranges = %" B_PRIu32 ")\n",
					gKernelArgs.num_virtual_allocated_ranges);
			}
		} else {
			TRACE("dropping\n");
		}

		total += map->length;
	}
	TRACE("total size kept: %" B_PRIu32 "\n", total);

	// remove the boot loader code from the virtual ranges to keep in the
	// kernel
	if (remove_virtual_range_to_keep(&__text_begin, &_end - &__text_begin)
			!= B_OK) {
		dprintf("%s: Failed to remove boot loader range "
			"from virtual ranges to keep.\n", __func__);
	}

	return B_OK;
}


static void *
find_physical_memory_range(size_t size)
{
	for (uint32 i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
		if (gKernelArgs.physical_memory_range[i].size > size)
			return (void *)(addr_t)gKernelArgs.physical_memory_range[i].start;
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
		void *address
			= (void *)(addr_t)(gKernelArgs.physical_allocated_range[i].start
				+ gKernelArgs.physical_allocated_range[i].size);
		if (!is_physical_allocated(address, size)
			&& is_physical_memory(address, size)) {
			return address;
		}
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
		void *address
			= (void *)(addr_t)(gKernelArgs.virtual_allocated_range[i].start
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
#if 0
	if (!virtualAddress)
		virtualAddress = (void*)KERNEL_BASE;
#endif

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

#if 0
	intptr_t status;

	/* claim the address */
	status = of_call_method(sMmuInstance, "claim", 3, 1, 0, size,
		virtualAddress, &_virtualAddress);
	if (status != 0) {
		dprintf("arch_mmu_allocate(base: %p, size: %" B_PRIuSIZE ") "
			"failed to claim virtual address\n", virtualAddress, size);
		return NULL;
	}

#endif
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

#if 0
	void* _physicalAddress;
	status = of_call_method(sMemoryInstance, "claim", 3, 1, physicalAddress,
		1, size, &_physicalAddress);

	if (status != 0) {
		dprintf("arch_mmu_allocate(base: %p, size: %" B_PRIuSIZE ") "
			"failed to claim physical address\n", physicalAddress, size);
		return NULL;
	}
#endif

	insert_virtual_allocated_range((addr_t)virtualAddress, size);
	insert_physical_allocated_range((addr_t)physicalAddress, size);

	if (!map_range(virtualAddress, physicalAddress, size, protection))
		return NULL;

	return virtualAddress;
}


extern "C" status_t
arch_mmu_free(void *address, size_t size)
{
	// TODO: implement freeing a region!
	return B_OK;
}


//	#pragma mark - OpenFirmware callbacks and public API


#if 0
static int
map_callback(struct of_arguments *args)
{
	void *physicalAddress = (void *)args->Argument(0);
	void *virtualAddress = (void *)args->Argument(1);
	int length = args->Argument(2);
	int mode = args->Argument(3);
	intptr_t &error = args->ReturnValue(0);

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
	// could not find the translation
	return OF_FAILED;
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
#endif


extern "C" status_t
arch_set_callback(void)
{
#if 0
	// set OpenFirmware callbacks - it will ask us for memory after that
	// instead of maintaining it itself

	void *oldCallback = NULL;
	if (of_call_client_function("set-callback", 1, 1, &callback, &oldCallback)
			== OF_FAILED) {
		dprintf("Error: OpenFirmware set-callback failed\n");
		return B_ERROR;
	}
	TRACE("old callback = %p; new callback = %p\n", oldCallback, callback);
#endif

	return B_OK;
}


extern "C" status_t
arch_mmu_init(void)
{
	if (of_getprop(gChosen, "mmu", &sMmuInstance, sizeof(int)) == OF_FAILED) {
		dprintf("%s: Error: no OpenFirmware mmu\n", __func__);
		return B_ERROR;
	}

	if (of_getprop(gChosen, "memory", &sMemoryInstance, sizeof(int)) == OF_FAILED) {
		dprintf("%s: Error: no OpenFirmware memory\n", __func__);
		return B_ERROR;
	}
	// get map of physical memory (fill in kernel_args structure)

	size_t total;
	if (find_physical_memory_ranges(total) != B_OK) {
		dprintf("Error: could not find physical memory ranges!\n");
		return B_ERROR;
	}
	TRACE("total physical memory = %luMB\n", total / (1024 * 1024));

	void *exceptionHandlers = (void *)-1;
	if (find_allocated_ranges(&exceptionHandlers) != B_OK) {
		dprintf("Error: find_allocated_ranges() failed\n");
		return B_ERROR;
	}

#if 0
	if (exceptionHandlers == (void *)-1) {
		// TODO: create mapping for the exception handlers
		dprintf("Error: no mapping for the exception handlers!\n");
	}

	// Set the Open Firmware memory callback. From now on the Open Firmware
	// will ask us for memory.
	arch_set_callback();

	// set up new page table and turn on translation again
	// TODO "set up new page table and turn on translation again" (see PPC)
#endif

	// set kernel args

	TRACE("virt_allocated: %" B_PRIu32 "\n",
		gKernelArgs.num_virtual_allocated_ranges);
	TRACE("phys_allocated: %" B_PRIu32 "\n",
		gKernelArgs.num_physical_allocated_ranges);
	TRACE("phys_memory: %" B_PRIu32 "\n",
		gKernelArgs.num_physical_memory_ranges);

#if 0
	// TODO set gKernelArgs.arch_args content if we have something to put in there
	gKernelArgs.arch_args.page_table.start = (addr_t)sPageTable;
	gKernelArgs.arch_args.page_table.size = tableSize;

	gKernelArgs.arch_args.exception_handlers.start = (addr_t)exceptionHandlers;
	gKernelArgs.arch_args.exception_handlers.size = B_PAGE_SIZE;
#endif

	return B_OK;
}

