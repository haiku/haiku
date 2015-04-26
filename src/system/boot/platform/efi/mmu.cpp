/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Copyright 2014, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Copyright 2014, Henry Harrington, henry.harrington@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <algorithm>

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


static uint64_t next_virtual_address = KERNEL_LOAD_BASE_64_BIT + 32 * 1024 * 1024;
static allocated_memory_region *allocated_memory_regions = NULL;


static uint64_t mmu_allocate_page()
{
	EFI_PHYSICAL_ADDRESS addr;
	EFI_STATUS s = kBootServices->AllocatePages(AllocateAnyPages, EfiLoaderData, 1, &addr);
	if (s != EFI_SUCCESS)
		panic("Unabled to allocate memory: %li", s);

	return addr;
}


uint64_t
mmu_generate_post_efi_page_tables(UINTN memory_map_size,
	EFI_MEMORY_DESCRIPTOR *memory_map, UINTN descriptor_size,
	UINTN descriptor_version)
{
	// Generate page tables, matching bios_ia32/long.cpp.
	uint64_t *pml4;
	uint64_t *pdpt;
	uint64_t *pageDir;
	uint64_t *pageTable;

	// Allocate the top level PML4.
	pml4 = NULL;
	if (platform_allocate_region((void**)&pml4, B_PAGE_SIZE, 0, false) != B_OK)
		panic("Failed to allocate PML4.");
	gKernelArgs.arch_args.phys_pgdir = (uint32_t)(addr_t)pml4;
	memset(pml4, 0, B_PAGE_SIZE);
	platform_bootloader_address_to_kernel_address(pml4, &gKernelArgs.arch_args.vir_pgdir);

	// Store the virtual memory usage information.
	gKernelArgs.virtual_allocated_range[0].start = KERNEL_LOAD_BASE_64_BIT;
	gKernelArgs.virtual_allocated_range[0].size = next_virtual_address - KERNEL_LOAD_BASE_64_BIT;
	gKernelArgs.num_virtual_allocated_ranges = 1;
	gKernelArgs.arch_args.virtual_end = ROUNDUP(KERNEL_LOAD_BASE_64_BIT
		+ gKernelArgs.virtual_allocated_range[0].size, 0x200000);

	// Find the highest physical memory address. We map all physical memory
	// into the kernel address space, so we want to make sure we map everything
	// we have available.
	uint64 maxAddress = 0;
	for (UINTN i = 0; i < memory_map_size / descriptor_size; ++i) {
		EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *)((addr_t)memory_map + i * descriptor_size);
		maxAddress = std::max(maxAddress,
				      entry->PhysicalStart + entry->NumberOfPages * 4096);
	}

	// Want to map at least 4GB, there may be stuff other than usable RAM that
	// could be in the first 4GB of physical address space.
	maxAddress = std::max(maxAddress, (uint64)0x100000000ll);
	maxAddress = ROUNDUP(maxAddress, 0x40000000);

	// Currently only use 1 PDPT (512GB). This will need to change if someone
	// wants to use Haiku on a box with more than 512GB of RAM but that's
	// probably not going to happen any time soon.
	if (maxAddress / 0x40000000 > 512)
		panic("Can't currently support more than 512GB of RAM!");

	// Create page tables for the physical map area. Also map this PDPT
	// temporarily at the bottom of the address space so that we are identity
	// mapped.

	pdpt = (uint64*)mmu_allocate_page();
	memset(pdpt, 0, B_PAGE_SIZE);
	pml4[510] = (addr_t)pdpt | kTableMappingFlags;
	pml4[0] = (addr_t)pdpt | kTableMappingFlags;

	for (uint64 i = 0; i < maxAddress; i += 0x40000000) {
		pageDir = (uint64*)mmu_allocate_page();
		memset(pageDir, 0, B_PAGE_SIZE);
		pdpt[i / 0x40000000] = (addr_t)pageDir | kTableMappingFlags;

		for (uint64 j = 0; j < 0x40000000; j += 0x200000) {
			pageDir[j / 0x200000] = (i + j) | kLargePageMappingFlags;
		}
	}

	// Allocate tables for the kernel mappings.

	pdpt = (uint64*)mmu_allocate_page();
	memset(pdpt, 0, B_PAGE_SIZE);
	pml4[511] = (addr_t)pdpt | kTableMappingFlags;

	pageDir = (uint64*)mmu_allocate_page();
	memset(pageDir, 0, B_PAGE_SIZE);
	pdpt[510] = (addr_t)pageDir | kTableMappingFlags;

	// We can now allocate page tables and duplicate the mappings across from
	// the 32-bit address space to them.
	pageTable = NULL; // shush, compiler.
	for (uint32 i = 0; i < gKernelArgs.virtual_allocated_range[0].size
			/ B_PAGE_SIZE; i++) {
		if ((i % 512) == 0) {
			pageTable = (uint64*)mmu_allocate_page();
			memset(pageTable, 0, B_PAGE_SIZE);
			pageDir[i / 512] = (addr_t)pageTable | kTableMappingFlags;
		}

		// Get the physical address to map.
		void *phys;
		if (platform_kernel_address_to_bootloader_address(KERNEL_LOAD_BASE_64_BIT + (i * B_PAGE_SIZE),
								  &phys) != B_OK)
			continue;

		pageTable[i % 512] = (addr_t)phys | kPageMappingFlags;
	}

	return (uint64)pml4;
}


// Called after EFI boot services exit.
// Currently assumes that the memory map is sane... Sorted and no overlapping
// regions.
void
mmu_post_efi_setup(UINTN memory_map_size, EFI_MEMORY_DESCRIPTOR *memory_map, UINTN descriptor_size, UINTN descriptor_version)
{
	// Add physical memory to the kernel args and update virtual addresses for EFI regions..
	addr_t addr = (addr_t)memory_map;
	gKernelArgs.num_physical_memory_ranges = 0;
	for (UINTN i = 0; i < memory_map_size / descriptor_size; ++i) {
		EFI_MEMORY_DESCRIPTOR *entry = (EFI_MEMORY_DESCRIPTOR *)(addr + i * descriptor_size);
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory: {
			// Usable memory.
			// Ignore memory below 1MB and above 512GB.
			uint64_t base = entry->PhysicalStart;
			uint64_t end = entry->PhysicalStart + entry->NumberOfPages * 4096;
			if (base < 0x100000)
				base = 0x100000;
			if (end > (512ull * 1024 * 1024 * 1024))
				end = 512ull * 1024 * 1024 * 1024;
			if (base >= end)
				break;
			uint64_t size = end - base;

			insert_physical_memory_range(base, size);
			// LoaderData memory is bootloader allocated memory, possibly
			// containing the kernel or loaded drivers.
			if (entry->Type == EfiLoaderData)
				insert_physical_allocated_range(base, size);
			break;
		}
		case EfiACPIReclaimMemory:
			// ACPI reclaim -- physical memory we could actually use later
			gKernelArgs.ignored_physical_memory += entry->NumberOfPages * 4096;
			break;
		case EfiRuntimeServicesCode:
		case EfiRuntimeServicesData:
			entry->VirtualStart = entry->PhysicalStart + 0xFFFFFF0000000000ull;
			break;
		}
	}

	// Sort the address ranges.
	sort_address_ranges(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges);
	sort_address_ranges(gKernelArgs.physical_allocated_range,
		gKernelArgs.num_physical_allocated_ranges);
	sort_address_ranges(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges);

	// Switch EFI to virtual mode, using the kernel pmap.
	// Something involving ConvertPointer might need to be done after this?
	// http://wiki.phoenix.com/wiki/index.php/EFI_RUNTIME_SERVICES#SetVirtualAddressMap.28.29
	kRuntimeServices->SetVirtualAddressMap(memory_map_size, descriptor_size, descriptor_version, memory_map);
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

	EFI_PHYSICAL_ADDRESS addr;
	size_t aligned_size = ROUNDUP(size, B_PAGE_SIZE);
	allocated_memory_region *region = new(std::nothrow) allocated_memory_region;

	if (region == NULL)
		return B_NO_MEMORY;

	EFI_STATUS status = kBootServices->AllocatePages(AllocateAnyPages,
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

	size_t aligned_size = ROUNDUP(size, B_PAGE_SIZE);
	allocated_memory_region *region = new(std::nothrow) allocated_memory_region;

	if (!region)
		return B_NO_MEMORY;

	// Addresses above 512GB not supported.
	// Memory map regions above 512GB can be ignored, but if EFI returns pages above
	// that there's nothing that can be done to fix it.
	if (physicalAddress + size > (512ull * 1024 * 1024 * 1024))
		panic("Can't currently support more than 512GB of RAM!");

	region->next = allocated_memory_regions;
	allocated_memory_regions = region;
	region->vaddr = 0;
	region->paddr = physicalAddress;
	region->size = aligned_size;
	region->released = false;

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


extern "C" status_t
platform_bootloader_address_to_kernel_address(void *address, uint64_t *_result)
{
	uint64_t addr = (uint64_t)address;

	for (allocated_memory_region *region = allocated_memory_regions; region; region = region->next) {
		if (region->paddr <= addr && addr < region->paddr + region->size) {
			// Lazily allocate virtual memory.
			if (region->vaddr == 0) {
				region->vaddr = next_virtual_address;
				next_virtual_address += ROUNDUP(region->size, B_PAGE_SIZE);
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

	kBootServices->FreePages((EFI_PHYSICAL_ADDRESS)address, ROUNDUP(size, B_PAGE_SIZE) / B_PAGE_SIZE);

	return B_OK;
}
