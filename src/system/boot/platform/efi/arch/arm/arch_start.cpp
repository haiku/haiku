/*
 * Copyright 2019-2020 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <kernel.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include "efi_platform.h"


#define ALIGN_MEMORY_MAP	4


extern "C" void arch_enter_kernel(uint32_t ttbr, struct kernel_args *kernelArgs,
	addr_t kernelEntry, addr_t kernelStackTop);

// From arch_mmu.cpp
extern void arch_mmu_post_efi_setup(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version);

extern uint32_t arch_mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version);


static const char*
memory_region_type_str(int type)
{
	switch (type)	{
	case EfiReservedMemoryType:
		return "ReservedMemoryType";
	case EfiLoaderCode:
		return "LoaderCode";
	case EfiLoaderData:
		return "LoaderData";
	case EfiBootServicesCode:
		return "BootServicesCode";
	case EfiBootServicesData:
		return "BootServicesData";
	case EfiRuntimeServicesCode:
		return "RuntimeServicesCode";
	case EfiRuntimeServicesData:
		return "RuntimeServicesData";
	case EfiConventionalMemory:
		return "ConventionalMemory";
	case EfiUnusableMemory:
		return "UnusableMemory";
	case EfiACPIReclaimMemory:
		return "ACPIReclaimMemory";
	case EfiACPIMemoryNVS:
		return "ACPIMemoryNVS";
	case EfiMemoryMappedIO:
		return "MMIO";
	case EfiMemoryMappedIOPortSpace:
		return "MMIOPortSpace";
	case EfiPalCode:
		return "PalCode";
	case EfiPersistentMemory:
		return "PersistentMemory";
	default:
		return "unknown";
	}
}


void
arch_start_kernel(addr_t kernelEntry)
{
	// Prepare to exit EFI boot services.
	// Read the memory map.
	// First call is to determine the buffer size.
	size_t memory_map_size = 0;
	efi_memory_descriptor dummy;
	efi_memory_descriptor *memory_map;
	size_t map_key;
	size_t descriptor_size;
	uint32_t descriptor_version;
	if (kBootServices->GetMemoryMap(&memory_map_size, &dummy, &map_key,
			&descriptor_size, &descriptor_version) != EFI_BUFFER_TOO_SMALL) {
		panic("Unable to determine size of system memory map");
	}

	// Allocate a buffer twice as large as needed just in case it gets bigger
	// between calls to ExitBootServices.
	size_t actual_memory_map_size = memory_map_size * 2;
	memory_map
		= (efi_memory_descriptor *)kernel_args_malloc(actual_memory_map_size +
			ALIGN_MEMORY_MAP);

	// align memory_map to 4-byte boundary
	// otherwise we get alignment exception when calling GetMemoryMap below
	memory_map = (efi_memory_descriptor *)ROUNDUP((uint32_t)memory_map, ALIGN_MEMORY_MAP);

	if (memory_map == NULL)
		panic("Unable to allocate memory map.");

	// Read (and print) the memory map.
	memory_map_size = actual_memory_map_size;
	if (kBootServices->GetMemoryMap(&memory_map_size, memory_map, &map_key,
			&descriptor_size, &descriptor_version) != EFI_SUCCESS) {
		panic("Unable to fetch system memory map.");
	}

	addr_t addr = (addr_t)memory_map;
	dprintf("System provided memory map:\n");
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor *entry
			= (efi_memory_descriptor *)(addr + i * descriptor_size);
		dprintf("  phys: 0x%08llx-0x%08llx, virt: 0x%08llx-0x%08llx, type: %s (%#x), attr: %#llx\n",
			entry->PhysicalStart,
			entry->PhysicalStart + entry->NumberOfPages * B_PAGE_SIZE,
			entry->VirtualStart,
			entry->VirtualStart + entry->NumberOfPages * B_PAGE_SIZE,
			memory_region_type_str(entry->Type), entry->Type,
			entry->Attribute);
	}

	// Generate page tables for use after ExitBootServices.
	uint32_t final_ttbr0 = arch_mmu_generate_post_efi_page_tables(
		memory_map_size, memory_map, descriptor_size, descriptor_version);

	// Attempt to fetch the memory map and exit boot services.
	// This needs to be done in a loop, as ExitBootServices can change the
	// memory map.
	// Even better: Only GetMemoryMap and ExitBootServices can be called after
	// the first call to ExitBootServices, as the firmware is permitted to
	// partially exit. This is why twice as much space was allocated for the
	// memory map, as it's impossible to allocate more now.
	// A changing memory map shouldn't affect the generated page tables, as
	// they only needed to know about the maximum address, not any specific
	// entry.
	dprintf("Calling ExitBootServices. So long, EFI!\n");
	while (true) {
		if (kBootServices->ExitBootServices(kImage, map_key) == EFI_SUCCESS) {
			// The console was provided by boot services, disable it.
			stdout = NULL;
			stderr = NULL;
			// Can we adjust gKernelArgs.platform_args.serial_base_ports[0]
			// to something fixed in qemu for debugging?
			break;
		}

		memory_map_size = actual_memory_map_size;
		if (kBootServices->GetMemoryMap(&memory_map_size, memory_map, &map_key,
				&descriptor_size, &descriptor_version) != EFI_SUCCESS) {
			panic("Unable to fetch system memory map.");
		}
	}

	// Update EFI, generate final kernel physical memory map, etc.
	arch_mmu_post_efi_setup(memory_map_size, memory_map,
			descriptor_size, descriptor_version);

	//smp_boot_other_cpus(final_pml4, kernelEntry);

	// Enter the kernel!
	dprintf("arch_enter_kernel(ttbr0: 0x%08x, kernelArgs: 0x%08x, "
		"kernelEntry: 0x%08x, sp: 0x%08x)\n",
		final_ttbr0, (uint32_t)&gKernelArgs, (uint32_t)kernelEntry,
		(uint32_t)(gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size));

	arch_enter_kernel(final_ttbr0, &gKernelArgs, kernelEntry,
		gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size);
}
