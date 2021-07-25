/*
 * Copyright 2016-2020 Haiku, Inc. All rights reserved.
 * Copyright 2014, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Copyright 2014, Henry Harrington, henry.harrington@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <algorithm>

#include <boot/addr_range.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <kernel/kernel.h>

#include "efi_platform.h"
#include "mmu.h"


//#define TRACE_MMU
#ifdef TRACE_MMU
#   define TRACE(x...) dprintf("efi/mmu: " x)
#else
#   define TRACE(x...) ;
#endif


struct memory_region {
	memory_region *next;
	addr_t vaddr;
	phys_addr_t paddr;
	size_t size;

	void dprint(const char * msg) {
 	  dprintf("%s memory_region v: %#lx p: %#lx size: %lu\n", msg, vaddr,
			paddr, size);
	}

	bool matches(phys_addr_t expected_paddr, size_t expected_size) {
		return paddr == expected_paddr && size == expected_size;
	}
};


#if defined(KERNEL_LOAD_BASE_64_BIT)
static addr_t sNextVirtualAddress = KERNEL_LOAD_BASE_64_BIT + 32 * 1024 * 1024;
#elif defined(KERNEL_LOAD_BASE)
static addr_t sNextVirtualAddress = KERNEL_LOAD_BASE + 32 * 1024 * 1024;
#else
#error Unable to find kernel load base on this architecture!
#endif


static memory_region *allocated_regions = NULL;


extern "C" phys_addr_t
mmu_allocate_page()
{
	TRACE("%s: called\n", __func__);

	efi_physical_addr addr;
	efi_status s = kBootServices->AllocatePages(AllocateAnyPages,
		EfiLoaderData, 1, &addr);

	if (s != EFI_SUCCESS)
		panic("Unabled to allocate memory: %li", s);

	return addr;
}


extern "C" addr_t
get_next_virtual_address(size_t size)
{
	TRACE("%s: called. size: %" B_PRIuSIZE "\n", __func__, size);

	addr_t address = sNextVirtualAddress;
	sNextVirtualAddress += ROUNDUP(size, B_PAGE_SIZE);
	return address;
}


extern "C" addr_t
get_current_virtual_address()
{
	TRACE("%s: called\n", __func__);
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
platform_allocate_region(void **_address, size_t size, uint8 /* protection */,
	bool exactAddress)
{
	TRACE("%s: called\n", __func__);

	// We don't have any control over the page tables, give up right away if an
	// exactAddress is wanted.
	if (exactAddress)
		return B_NO_MEMORY;

	efi_physical_addr addr;
	size_t pages = ROUNDUP(size, B_PAGE_SIZE) / B_PAGE_SIZE;
	efi_status status = kBootServices->AllocatePages(AllocateAnyPages,
		EfiLoaderData, pages, &addr);
	if (status != EFI_SUCCESS)
		return B_NO_MEMORY;

	// Addresses above 512GB not supported.
	// Memory map regions above 512GB can be ignored, but if EFI returns pages
	// above that there's nothing that can be done to fix it.
	if (addr + size > (512ull * 1024 * 1024 * 1024))
		panic("Can't currently support more than 512GB of RAM!");

	memory_region *region = new(std::nothrow) memory_region {
		next: allocated_regions,
		vaddr: *_address == NULL ? 0 : (addr_t)*_address,
		paddr: (phys_addr_t)addr,
		size: size
	};

	if (region == NULL) {
		kBootServices->FreePages(addr, pages);
		return B_NO_MEMORY;
	}

#ifdef TRACE_MMU
	//region->dprint("Allocated");
#endif
	allocated_regions = region;
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
	TRACE("%s: called\n", __func__);

	addr_t pageOffset = physicalAddress & (B_PAGE_SIZE - 1);

	physicalAddress -= pageOffset;
	size += pageOffset;

	if (insert_physical_allocated_range(physicalAddress,
			ROUNDUP(size, B_PAGE_SIZE)) != B_OK)
		return B_NO_MEMORY;

	return physicalAddress + pageOffset;
}


static void
convert_physical_ranges()
{
	TRACE("%s: called\n", __func__);

	addr_range *range = gKernelArgs.physical_allocated_range;
	uint32 num_ranges = gKernelArgs.num_physical_allocated_ranges;

	for (uint32 i = 0; i < num_ranges; ++i) {
		// Addresses above 512GB not supported.
		// Memory map regions above 512GB can be ignored, but if EFI returns
		// pages above that there's nothing that can be done to fix it.
		if (range[i].start + range[i].size > (512ull * 1024 * 1024 * 1024))
			panic("Can't currently support more than 512GB of RAM!");

		memory_region *region = new(std::nothrow) memory_region {
			next: allocated_regions,
			vaddr: 0,
			paddr: (phys_addr_t)range[i].start,
			size: (size_t)range[i].size
		};

		if (!region)
			panic("Couldn't add allocated region");

		allocated_regions = region;

		// Clear out the allocated range
		range[i].start = 0;
		range[i].size = 0;
		gKernelArgs.num_physical_allocated_ranges--;
	}
}


extern "C" status_t
platform_bootloader_address_to_kernel_address(void *address, addr_t *_result)
{
	TRACE("%s: called\n", __func__);

	// Convert any physical ranges prior to looking up address
	convert_physical_ranges();

	phys_addr_t addr = (phys_addr_t)address;

	for (memory_region *region = allocated_regions; region;
			region = region->next) {
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
platform_kernel_address_to_bootloader_address(addr_t address, void **_result)
{
	TRACE("%s: called\n", __func__);

	for (memory_region *region = allocated_regions; region;
			region = region->next) {
		if (region->vaddr != 0 && region->vaddr <= address
				&& address < region->vaddr + region->size) {
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
	TRACE("%s: called to release region %p (%" B_PRIuSIZE ")\n", __func__,
		address, size);

	for (memory_region **ref = &allocated_regions; *ref;
			ref = &(*ref)->next) {
		if ((*ref)->matches((phys_addr_t)address, size)) {
			kBootServices->FreePages((efi_physical_addr)address,
				ROUNDUP(size, B_PAGE_SIZE) / B_PAGE_SIZE);
			memory_region* old = *ref;
			//pointer to current allocated_memory_region* now points to next
			*ref = (*ref)->next;
#ifdef TRACE_MMU
			old->dprint("Freeing");
#endif
			delete old;
			return B_OK;
		}
	}
	panic("platform_free_region: Unknown region to free??");
	return B_ERROR; // NOT Reached
}


bool
mmu_next_region(void** cookie, addr_t* vaddr, phys_addr_t* paddr, size_t* size)
{
	if (*cookie == NULL)
		*cookie = &allocated_regions;
	else
		*cookie = ((memory_region*)*cookie)->next;

	memory_region* region = (memory_region*)*cookie;
	if (region == NULL)
		return false;

	if (region->vaddr == 0)
		region->vaddr = get_next_virtual_address(region->size);

	*vaddr = region->vaddr;
	*paddr = region->paddr;
	*size = region->size;
	return true;
}
