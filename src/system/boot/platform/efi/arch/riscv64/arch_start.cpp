/*
 * Copyright 2019-2020 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include "efi_platform.h"


extern "C" void arch_enter_kernel(struct kernel_args *kernelArgs,
	addr_t kernelEntry, addr_t kernelStackTop);

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
		= (efi_memory_descriptor *)kernel_args_malloc(actual_memory_map_size);

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
		dprintf("  %#lx-%#lx  %#lx %#x %#lx\n", entry->PhysicalStart,
			entry->PhysicalStart + entry->NumberOfPages * B_PAGE_SIZE,
			entry->VirtualStart, entry->Type, entry->Attribute);
	}

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
	//arch_mmu_post_efi_setup(memory_map_size, memory_map,
	//		descriptor_size, descriptor_version);

	//smp_boot_other_cpus(final_pml4, kernelEntry);

	// Enter the kernel!
	arch_enter_kernel(&gKernelArgs, kernelEntry,
		gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size);
}
