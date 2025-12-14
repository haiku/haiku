/*
 * Copyright 2014-2022 Haiku, Inc. All rights reserved.
 * Copyright 2013-2014, Fredrik Holmqvist, fredrik.holmqvist@gmail.com.
 * Copyright 2014, Henry Harrington, henry.harrington@gmail.com.
 * All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include "efi_platform.h"
#include "generic_mmu.h"
#include "mmu.h"
#include "serial.h"
#include "smp.h"


// From entry.S
extern "C" void arch_enter_kernel(uint64 pml4, uint64 entry_point, uint64 stackTop);

// From arch_mmu.cpp
extern void arch_mmu_post_efi_setup(size_t memoryMapSize,
	efi_memory_descriptor *memoryMap, size_t descriptorSize,
	uint32_t descriptorVersion);

extern uint64_t arch_mmu_generate_post_efi_page_tables(size_t memoryMapSize,
	efi_memory_descriptor *memoryMap, size_t descriptorSize,
	uint32_t descriptorVersion);


void
arch_convert_kernel_args(void)
{
	fix_address(gKernelArgs.ucode_data);
	fix_address(gKernelArgs.arch_args.apic);
	fix_address(gKernelArgs.arch_args.hpet);
}


void
arch_start_kernel(addr_t kernelEntry)
{
	// Prepare to exit EFI boot services.
	// Read the memory map.
	// First call is to determine the buffer size.
	size_t memoryMapSize = 0;
	efi_memory_descriptor dummy;
	size_t mapKey;
	size_t descriptorSize;
	uint32_t descriptorVersion;
	if (kBootServices->GetMemoryMap(&memoryMapSize, &dummy, &mapKey,
		&descriptorSize, &descriptorVersion) != EFI_BUFFER_TOO_SMALL) {
		panic("Unable to determine size of system memory map");
	}

	// Allocate a buffer twice as large as needed just in case it gets bigger
	// between calls to ExitBootServices.
	size_t actualMemoryMapSize = memoryMapSize * 2;
	efi_memory_descriptor *memoryMap
		= (efi_memory_descriptor *)kernel_args_malloc(actualMemoryMapSize);

	if (memoryMap == NULL)
		panic("Unable to allocate memory map.");

	// Read (and print) the memory map.
	memoryMapSize = actualMemoryMapSize;
	if (kBootServices->GetMemoryMap(&memoryMapSize, memoryMap, &mapKey,
			&descriptorSize, &descriptorVersion) != EFI_SUCCESS) {
		panic("Unable to fetch system memory map.");
	}

	addr_t addr = (addr_t)memoryMap;
	dprintf("System provided memory map:\n");
	for (size_t i = 0; i < memoryMapSize / descriptorSize; i++) {
		efi_memory_descriptor *entry
			= (efi_memory_descriptor *)(addr + i * descriptorSize);
		dprintf("  phys: 0x%08" PRIx64 "-0x%08" PRIx64
			", virt: 0x%08" PRIx64 "-0x%08" PRIx64
			", type: %s (%#x), attr: %#" PRIx64 "\n",
			entry->PhysicalStart,
			entry->PhysicalStart + entry->NumberOfPages * B_PAGE_SIZE,
			entry->VirtualStart,
			entry->VirtualStart + entry->NumberOfPages * B_PAGE_SIZE,
			memory_region_type_str(entry->Type), entry->Type,
			entry->Attribute);
	}

	// Generate page tables for use after ExitBootServices.
	uint64_t final_pml4 = arch_mmu_generate_post_efi_page_tables(
		memoryMapSize, memoryMap, descriptorSize, descriptorVersion);

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
	serial_disable();

	while (true) {
		if (kBootServices->ExitBootServices(kImage, mapKey) == EFI_SUCCESS) {
			// Disconnect from EFI serial_io / stdio services
			serial_kernel_handoff();
			dprintf("Unhooked from EFI serial services\n");
			break;
		}

		memoryMapSize = actualMemoryMapSize;
		if (kBootServices->GetMemoryMap(&memoryMapSize, memoryMap, &mapKey,
				&descriptorSize, &descriptorVersion) != EFI_SUCCESS) {
			panic("Unable to fetch system memory map.");
		}
	}

	// Update EFI, generate final kernel physical memory map, etc.
	arch_mmu_post_efi_setup(memoryMapSize, memoryMap,
		descriptorSize, descriptorVersion);

	// Re-init and activate serial in a horrific post-EFI landscape. Clowns roam the land freely.
	serial_init();
	serial_enable();

	smp_boot_other_cpus(final_pml4, kernelEntry, (addr_t)&gKernelArgs);

	// Enter the kernel!
	dprintf("arch_enter_kernel(final_pml4: 0x%08" PRIx64 ", kernelArgs: %p, "
		"kernelEntry: 0x%08" B_PRIxADDR ", sp: 0x%08" B_PRIx64 ")\n",
		final_pml4, &gKernelArgs, kernelEntry,
		gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size);

	arch_enter_kernel(final_pml4, kernelEntry,
		gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size);
}
