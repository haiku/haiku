/*
 * Copyright 2019-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include <arch_cpu_defs.h>

#include "arch_traps.h"
#include "efi_platform.h"
#include "generic_mmu.h"
#include "mmu.h"
#include "serial.h"
#include "smp.h"


// From entry.S
extern "C" void arch_enter_kernel(uint64 satp, addr_t kernelArgs,
	addr_t kernelEntry, addr_t kernelStackTop);

// From arch_mmu.cpp
extern void arch_mmu_post_efi_setup(size_t memoryMapSize,
	efi_memory_descriptor *memoryMap, size_t descriptorSize,
	uint32_t descriptorVersion);

extern uint64_t arch_mmu_generate_post_efi_page_tables(size_t memoryMapSize,
	efi_memory_descriptor *memoryMap, size_t descriptorSize,
	uint32_t descriptorVersion);


#include <arch/riscv64/arch_uart_sifive.h>


void
arch_convert_kernel_args(void)
{
	fix_address(gKernelArgs.arch_args.fdt);
}


void
arch_start_kernel(addr_t kernelEntry)
{
	// Allocate virtual memory for kernel args
	struct kernel_args *kernelArgs = NULL;
	if (platform_allocate_region((void **)&kernelArgs,
			sizeof(struct kernel_args), 0) != B_OK)
		panic("Failed to allocate kernel args.");

	addr_t virtKernelArgs;
	platform_bootloader_address_to_kernel_address((void*)kernelArgs,
		&virtKernelArgs);

	// EFI assumed to be SBI booted
	gKernelArgs.arch_args.machine_platform = kPlatformSbi;

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
	for (size_t i = 0; i < memoryMapSize / descriptorSize; ++i) {
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
	uint64_t satp = arch_mmu_generate_post_efi_page_tables(
		memoryMapSize, memoryMap, descriptorSize, descriptorVersion);
	dprintf("SATP: 0x%016" B_PRIx64 "\n", satp);

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

	arch_traps_init();

	// Update EFI, generate final kernel physical memory map, etc.
	arch_mmu_post_efi_setup(memoryMapSize, memoryMap,
		descriptorSize, descriptorVersion);

	// Re-init and activate serial in a horrific post-EFI landscape. Clowns roam the land freely.
	serial_init();
	serial_enable();

	// Copy final kernel args
	// This should be the last step before jumping to the kernel
	// as there are some fixups happening to kernel_args even in the last minute
	memcpy(kernelArgs, &gKernelArgs, sizeof(struct kernel_args));

	smp_boot_other_cpus(satp, kernelEntry, virtKernelArgs);

	// Enter the kernel!
	dprintf("arch_enter_kernel(satp: %#" B_PRIxADDR ", kernelArgs: %#" B_PRIxADDR
		", kernelEntry: %#" B_PRIxADDR ", sp: %#" B_PRIxADDR ")\n",	satp,
		(addr_t)&kernelArgs, (addr_t)kernelEntry, kernelArgs->cpu_kstack[0].start
			+ kernelArgs->cpu_kstack[0].size);

	arch_enter_kernel(satp, virtKernelArgs, kernelEntry,
		kernelArgs->cpu_kstack[0].start + kernelArgs->cpu_kstack[0].size);
}
