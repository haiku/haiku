/*
 * Copyright 2021-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <kernel.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include "efi_platform.h"
#include "generic_mmu.h"
#include "mmu.h"
#include "serial.h"
#include "smp.h"


struct gdt_idt_descr {
	uint16  limit;
	void*   base;
} _PACKED;


extern "C" typedef void (*enter_kernel_t)(uint32_t, addr_t, addr_t, addr_t,
	struct gdt_idt_descr *);


// From entry.S
extern "C" void arch_enter_kernel(uint32_t pageDirectory, addr_t kernelArgs,
	addr_t kernelEntry, addr_t kernelStackTop, struct gdt_idt_descr *gdtDescriptor);

// From arch_mmu.cpp
extern void arch_mmu_init_gdt(gdt_idt_descr &bootGDTDescriptor);

extern void arch_mmu_post_efi_setup(size_t memoryMapSize,
	efi_memory_descriptor *memoryMap, size_t descriptorSize,
	uint32_t descriptorVersion);

extern uint32_t arch_mmu_generate_post_efi_page_tables(size_t memoryMapSize,
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
	gdt_idt_descr bootGDTDescriptor;
	arch_mmu_init_gdt(bootGDTDescriptor);

	// Copy entry.S trampoline to lower 1M
	enter_kernel_t enter_kernel = (enter_kernel_t)0xa000;
	memcpy((void *)enter_kernel, (void *)arch_enter_kernel, B_PAGE_SIZE);

	// Allocate virtual memory for kernel args
	struct kernel_args *kernelArgs = NULL;
	if (platform_allocate_region((void **)&kernelArgs,
			sizeof(struct kernel_args), 0, false) != B_OK)
		panic("Failed to allocate kernel args.");

	addr_t virtKernelArgs;
	platform_bootloader_address_to_kernel_address((void*)kernelArgs,
		&virtKernelArgs);

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
	uint32_t pageDirectory = arch_mmu_generate_post_efi_page_tables(
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

	serial_enable();

	// Copy final kernel args
	// This should be the last step before jumping to the kernel
	// as there are some fixups happening to kernel_args even in the last minute
	memcpy(kernelArgs, &gKernelArgs, sizeof(struct kernel_args));

	smp_boot_other_cpus(pageDirectory, kernelEntry, virtKernelArgs);

	// Enter the kernel!
	dprintf("enter_kernel(pageDirectory: 0x%08x, kernelArgs: 0x%08x, "
		"kernelEntry: 0x%08x, sp: 0x%08x, bootGDTDescriptor: 0x%08x)\n",
		pageDirectory, (uint32_t)virtKernelArgs, (uint32_t)kernelEntry,
		(uint32_t)(gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size),
		(uint32_t)&bootGDTDescriptor);

	enter_kernel(pageDirectory, virtKernelArgs, kernelEntry,
		gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size,
		&bootGDTDescriptor);
}
