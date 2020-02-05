/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Copyright 2014, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Copyright 2014, Henry Harrington, henry.harrington@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <algorithm>

#include <boot/addr_range.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <kernel/arch/x86/arch_kernel.h>
#include <kernel/kernel.h>

#include "efi_platform.h"
#include "mmu.h"


struct allocated_memory_region {
	allocated_memory_region *next;
	uint64_t vaddr;
	uint64_t paddr;
	size_t size;
	bool released;
};


static addr_t sNextVirtualAddress = KERNEL_LOAD_BASE_64_BIT + 32 * 1024 * 1024;
static allocated_memory_region *allocated_memory_regions = NULL;


extern "C" uint64_t
mmu_allocate_page()
{
	efi_physical_addr addr;
	efi_status s = kBootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &addr);
	if (s != EFI_SUCCESS)
		panic("Unabled to allocate memory: %li", s);

	return addr;
}


extern "C" addr_t
get_next_virtual_address(size_t size)
{
	addr_t address = sNextVirtualAddress;
	sNextVirtualAddress += ROUNDUP(size, B_PAGE_SIZE);
	return address;
}


extern "C" addr_t
get_current_virtual_address()
{
	return sNextVirtualAddress;
}


// Platform allocator.
// The bootloader assumes that bootloader address space == kernel address space.
// This is not true until just before the kernel is booted, so an ugly hack is
// used to cover the difference. platform_allocate_region allocates addresses
// in bootloader space, but can convert them to kernel space. The ELF loader
// accesses kernel memory via Mao(), and much later in the boot process,
// addresses in the kernel argument struct are converted from bootloader
// addresses to kernel addresses.

extern "C" status_t
platform_allocate_region(void **_address, size_t size, uint8 /* protection */, bool exactAddress)
{
	// We don't have any control over the page tables, give up right away if an
	// exactAddress is wanted.
	if (exactAddress)
		return B_NO_MEMORY;

	efi_physical_addr addr;
	size_t aligned_size = ROUNDUP(size, B_PAGE_SIZE);
	allocated_memory_region *region = new(std::nothrow) allocated_memory_region;

	if (region == NULL)
		return B_NO_MEMORY;

	efi_status status = kBootServices->AllocatePages(AllocateAnyPages,
		EfiLoaderData, aligned_size / B_PAGE_SIZE, &addr);
	if (status != EFI_SUCCESS) {
		delete region;
		return B_NO_MEMORY;
	}

	// Addresses above 512GB not supported.
	// Memory map regions above 512GB can be ignored, but if EFI returns pages
	// above that there's nothing that can be done to fix it.
	if (addr + size > (512ull * 1024 * 1024 * 1024))
		panic("Can't currently support more than 512GB of RAM!");

	region->next = allocated_memory_regions;
	allocated_memory_regions = region;
	region->vaddr = 0;
	region->paddr = addr;
	region->size = size;
	region->released = false;

	if (*_address != NULL) {
		region->vaddr = (uint64_t)*_address;
	}

	//dprintf("Allocated region %#lx (requested %p) %#lx %lu\n", region->vaddr, *_address, region->paddr, region->size);

	*_address = (void *)region->paddr;

	return B_OK;
}


/*!
	Neither \a virtualAddress nor \a size need to be aligned, but the function
	will map all pages the range intersects with.
	If physicalAddress is not page-aligned, the returned virtual address will
	have the same "misalignment".
*/
extern "C" addr_t
mmu_map_physical_memory(addr_t physicalAddress, size_t size, uint32 flags)
{
	addr_t pageOffset = physicalAddress & (B_PAGE_SIZE - 1);

	physicalAddress -= pageOffset;
	size += pageOffset;

	if (insert_physical_allocated_range(physicalAddress, ROUNDUP(size, B_PAGE_SIZE)) != B_OK)
		return B_NO_MEMORY;

	return physicalAddress + pageOffset;
}


extern "C" void
mmu_free(void *virtualAddress, size_t size)
{
	addr_t physicalAddress = (addr_t)virtualAddress;
	addr_t pageOffset = physicalAddress & (B_PAGE_SIZE - 1);

	physicalAddress -= pageOffset;
	size += pageOffset;

	size_t aligned_size = ROUNDUP(size, B_PAGE_SIZE);

	for (allocated_memory_region *region = allocated_memory_regions; region; region = region->next) {
		if (region->paddr == physicalAddress && region->size == aligned_size) {
			region->released = true;
			return;
		}
	}
}


static allocated_memory_region *
get_region(void *address, size_t size)
{
	for (allocated_memory_region *region = allocated_memory_regions; region; region = region->next) {
		if (region->paddr == (uint64_t)address && region->size == size) {
			return region;
		}
	}
	return 0;
}


static void
convert_physical_ranges() {
	addr_range *range = gKernelArgs.physical_allocated_range;
	uint32 num_ranges = gKernelArgs.num_physical_allocated_ranges;

	for (uint32 i = 0; i < num_ranges; ++i) {
		allocated_memory_region *region = new(std::nothrow) allocated_memory_region;

		if (!region)
			panic("Couldn't add allocated region");

		// Addresses above 512GB not supported.
		// Memory map regions above 512GB can be ignored, but if EFI returns pages above
		// that there's nothing that can be done to fix it.
		if (range[i].start + range[i].size > (512ull * 1024 * 1024 * 1024))
			panic("Can't currently support more than 512GB of RAM!");

		region->next = allocated_memory_regions;
		allocated_memory_regions = region;
		region->vaddr = 0;
		region->paddr = range[i].start;
		region->size = range[i].size;
		region->released = false;

		// Clear out the allocated range
		range[i].start = 0;
		range[i].size = 0;
		gKernelArgs.num_physical_allocated_ranges--;
	}
}


extern "C" status_t
platform_bootloader_address_to_kernel_address(void *address, uint64_t *_result)
{
	// Convert any physical ranges prior to looking up address
	convert_physical_ranges();

	uint64_t addr = (uint64_t)address;

	for (allocated_memory_region *region = allocated_memory_regions; region; region = region->next) {
		if (region->paddr <= addr && addr < region->paddr + region->size) {
			// Lazily allocate virtual memory.
			if (region->vaddr == 0) {
				region->vaddr = get_next_virtual_address(region->size);
			}
			*_result = region->vaddr + (addr - region->paddr);
			//dprintf("Converted bootloader address %p in region %#lx-%#lx to %#lx\n",
			//	address, region->paddr, region->paddr + region->size, *_result);
			return B_OK;
		}
	}

	return B_ERROR;
}


extern "C" status_t
platform_kernel_address_to_bootloader_address(uint64_t address, void **_result)
{
	for (allocated_memory_region *region = allocated_memory_regions; region; region = region->next) {
		if (region->vaddr != 0 && region->vaddr <= address && address < region->vaddr + region->size) {
			*_result = (void *)(region->paddr + (address - region->vaddr));
			//dprintf("Converted kernel address %#lx in region %#lx-%#lx to %p\n",
			//	address, region->vaddr, region->vaddr + region->size, *_result);
			return B_OK;
		}
	}

	return B_ERROR;
}


extern "C" status_t
platform_free_region(void *address, size_t size)
{
	//dprintf("Release region %p %lu\n", address, size);
	allocated_memory_region *region = get_region(address, size);
	if (!region)
		panic("Unknown region??");

	kBootServices->FreePages((efi_physical_addr)address, ROUNDUP(size, B_PAGE_SIZE) / B_PAGE_SIZE);

	return B_OK;
}
