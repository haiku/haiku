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


uint32 sPageTableHashMask;


// begin and end of the boot loader
extern "C" uint8 __text_begin;
extern "C" uint8 _end;


static status_t
find_physical_memory_ranges(size_t &total)
{
	int memory;
	dprintf("checking for memory...\n");
	if (of_getprop(gChosen, "memory", &memory, sizeof(int)) == OF_FAILED)
		return B_ERROR;
	int package = of_instance_to_package(memory);

	total = 0;

	// Memory base addresses are provided in 32 or 64 bit flavors
	// #address-cells and #size-cells matches the number of 32-bit 'cells'
	// representing the length of the base address and size fields
	int root = of_finddevice("/");
	int32 regAddressCells = of_address_cells(root);
	int32 regSizeCells = of_size_cells(root);
	if (regAddressCells == OF_FAILED || regSizeCells == OF_FAILED) {
		dprintf("finding base/size length counts failed, assume 32-bit.\n");
		regAddressCells = 1;
		regSizeCells = 1;
	}

	// NOTE : Size Cells of 2 is possible in theory... but I haven't seen it yet.
	if (regAddressCells > 2 || regSizeCells > 1) {
		panic("%s: Unsupported OpenFirmware cell count detected.\n"
		"Address Cells: %" B_PRId32 "; Size Cells: %" B_PRId32
		" (CPU > 64bit?).\n", __func__, regAddressCells, regSizeCells);
		return B_ERROR;
	}

	// On 64-bit PowerPC systems (G5), our mem base range address is larger
	if (regAddressCells == 2) {
		struct of_region<uint64> regions[64];
		int count = of_getprop(package, "reg", regions, sizeof(regions));
		if (count == OF_FAILED)
			count = of_getprop(memory, "reg", regions, sizeof(regions));
		if (count == OF_FAILED)
			return B_ERROR;
		count /= sizeof(regions[0]);

		for (int32 i = 0; i < count; i++) {
			if (regions[i].size <= 0) {
				dprintf("%d: empty region\n", i);
				continue;
			}
			dprintf("%" B_PRIu32 ": base = %" B_PRIu64 ","
				"size = %" B_PRIu32 "\n", i, regions[i].base, regions[i].size);

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

	// Otherwise, normal 32-bit PowerPC G3 or G4 have a smaller 32-bit one
	struct of_region<uint32> regions[64];
	int count = of_getprop(package, "reg", regions, sizeof(regions));
	if (count == OF_FAILED)
		count = of_getprop(memory, "reg", regions, sizeof(regions));
	if (count == OF_FAILED)
		return B_ERROR;
	count /= sizeof(regions[0]);

	for (int32 i = 0; i < count; i++) {
		if (regions[i].size <= 0) {
			dprintf("%d: empty region\n", i);
			continue;
		}
		dprintf("%" B_PRIu32 ": base = %" B_PRIu32 ","
			"size = %" B_PRIu32 "\n", i, regions[i].base, regions[i].size);

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
is_physical_memory(void *address, size_t size)
{
	return is_address_range_covered(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges, (addr_t)address, size);
}


static bool
is_physical_memory(void *address)
{
	return is_physical_memory(address, 1);
}


static void
map_page(void *virtualAddress, void *physicalAddress, uint8 mode)
{
	panic("%s: out of page table entries!\n", __func__);
}


static void
map_range(void *virtualAddress, void *physicalAddress, size_t size, uint8 mode)
{
	for (uint32 offset = 0; offset < size; offset += B_PAGE_SIZE) {
		map_page((void *)(intptr_t(virtualAddress) + offset),
			(void *)(intptr_t(physicalAddress) + offset), mode);
	}
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


//	#pragma mark - OpenFirmware callbacks and public API


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
	dprintf("total physical memory = %luMB\n", total / (1024 * 1024));

	return B_OK;
}

