/*
 * Copyright 2019-2021 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <algorithm>

#include <arm_mmu.h>
#include <kernel.h>
#include <arch_kernel.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <efi/types.h>
#include <efi/boot-services.h>

#include "mmu.h"
#include "efi_platform.h"


static void
map_range(addr_t virt_addr, phys_addr_t phys_addr, size_t size, uint32_t flags)
{
	ASSERT_ALWAYS(insert_virtual_allocated_range(virt_addr, size) >= B_OK);
}


static void
build_physical_memory_list(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	addr_t addr = (addr_t)memory_map;

	gKernelArgs.num_physical_memory_ranges = 0;

	// First scan: Add all usable ranges
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor *)(addr + i * descriptor_size);
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
			entry->VirtualStart = entry->PhysicalStart;
			break;
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory: {
			// Usable memory.
			uint64_t base = entry->PhysicalStart;
			uint64_t size = entry->NumberOfPages * B_PAGE_SIZE;
			insert_physical_memory_range(base, size);
			break;
		}
		case EfiACPIReclaimMemory:
			// ACPI reclaim -- physical memory we could actually use later
			break;
		case EfiRuntimeServicesCode:
		case EfiRuntimeServicesData:
			entry->VirtualStart = entry->PhysicalStart;
			break;
		case EfiMemoryMappedIO:
			entry->VirtualStart = entry->PhysicalStart;
			break;
		}
	}

	uint64_t initialPhysicalMemory = total_physical_memory();

	// Second scan: Remove everything reserved that may overlap
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor *)(addr + i * descriptor_size);
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory:
			break;
		default:
			uint64_t base = entry->PhysicalStart;
			uint64_t size = entry->NumberOfPages * B_PAGE_SIZE;
			remove_physical_memory_range(base, size);
		}
	}

	gKernelArgs.ignored_physical_memory
		+= initialPhysicalMemory - total_physical_memory();

	sort_address_ranges(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges);
}


static void
build_physical_allocated_list(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	addr_t addr = (addr_t)memory_map;
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor *)(addr + i * descriptor_size);
		switch (entry->Type) {
		case EfiLoaderData: {
			uint64_t base = entry->PhysicalStart;
			uint64_t size = entry->NumberOfPages * B_PAGE_SIZE;
			insert_physical_allocated_range(base, size);
			break;
		}
		default:
			;
		}
	}

	sort_address_ranges(gKernelArgs.physical_allocated_range,
		gKernelArgs.num_physical_allocated_ranges);
}


void
arch_mmu_init()
{
}


void
arch_mmu_post_efi_setup(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	build_physical_allocated_list(memory_map_size, memory_map,
		descriptor_size, descriptor_version);

	// Switch EFI to virtual mode, using the kernel pmap.
	// Something involving ConvertPointer might need to be done after this?
	// http://wiki.phoenix.com/wiki/index.php/EFI_RUNTIME_SERVICES
	//kRuntimeServices->SetVirtualAddressMap(memory_map_size, descriptor_size,
	//	descriptor_version, memory_map);

	dprintf("phys memory ranges:\n");
	for (uint32_t i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
		uint32_t start = (uint32_t)gKernelArgs.physical_memory_range[i].start;
		uint32_t size = (uint32_t)gKernelArgs.physical_memory_range[i].size;
		dprintf("    0x%08x-0x%08x, length 0x%08x\n",
			start, start + size, size);
	}

	dprintf("allocated phys memory ranges:\n");
	for (uint32_t i = 0; i < gKernelArgs.num_physical_allocated_ranges; i++) {
		uint32_t start = (uint32_t)gKernelArgs.physical_allocated_range[i].start;
		uint32_t size = (uint32_t)gKernelArgs.physical_allocated_range[i].size;
		dprintf("    0x%08x-0x%08x, length 0x%08x\n",
			start, start + size, size);
	}

	dprintf("allocated virt memory ranges:\n");
	for (uint32_t i = 0; i < gKernelArgs.num_virtual_allocated_ranges; i++) {
		uint32_t start = (uint32_t)gKernelArgs.virtual_allocated_range[i].start;
		uint32_t size = (uint32_t)gKernelArgs.virtual_allocated_range[i].size;
		dprintf("    0x%08x-0x%08x, length 0x%08x\n",
			start, start + size, size);
	}
}


uint32_t
arch_mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	addr_t memory_map_addr = (addr_t)memory_map;

	build_physical_memory_list(memory_map_size, memory_map,
		descriptor_size, descriptor_version);

	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry =
			(efi_memory_descriptor *)(memory_map_addr + i * descriptor_size);
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
			map_range(entry->VirtualStart, entry->PhysicalStart,
				entry->NumberOfPages * B_PAGE_SIZE, 0);
			break;
		default:
			;
		}
	}

	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry =
			(efi_memory_descriptor *)(memory_map_addr + i * descriptor_size);
		if ((entry->Attribute & EFI_MEMORY_RUNTIME) != 0)
			map_range(entry->VirtualStart, entry->PhysicalStart,
				entry->NumberOfPages * B_PAGE_SIZE, 0);
	}

	void* cookie = NULL;
	addr_t vaddr;
	phys_addr_t paddr;
	size_t size;
	while (mmu_next_region(&cookie, &vaddr, &paddr, &size)) {
		map_range(vaddr, paddr, size, 0);
	}

	// identity mapping for the debug uart
	map_range(0x09000000, 0x09000000, B_PAGE_SIZE, 0);

	sort_address_ranges(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges);

	return 0;
}
