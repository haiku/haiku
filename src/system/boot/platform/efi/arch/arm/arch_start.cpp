/*
 * Copyright 2019-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <kernel.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include "efi_platform.h"
#include "mmu.h"


#define ALIGN_MEMORY_MAP	4


extern "C" typedef void (*arch_enter_kernel_t)(uint32_t, addr_t, addr_t, addr_t);


extern "C" void arch_enter_kernel(uint32_t ttbr, addr_t kernelArgs,
	addr_t kernelEntry, addr_t kernelStackTop);

// From arch_mmu.cpp
extern void arch_mmu_post_efi_setup(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version);

extern uint32_t arch_mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version);


void
arch_convert_kernel_args(void)
{
	fix_address(gKernelArgs.arch_args.fdt);
}


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


static void *
allocate_trampoline_page(void)
{
	void *trampolinePage = NULL;
	if (platform_allocate_lomem(&trampolinePage, B_PAGE_SIZE) == B_OK)
		return trampolinePage;

	trampolinePage = (void *)get_next_virtual_address(B_PAGE_SIZE);
	if (platform_allocate_region(&trampolinePage, B_PAGE_SIZE, 0, true) == B_OK)
		return trampolinePage;

	trampolinePage = NULL;
	if (platform_allocate_region(&trampolinePage, B_PAGE_SIZE, 0, false) != B_OK)
		return NULL;

	if (platform_free_region(trampolinePage, B_PAGE_SIZE) != B_OK)
		return NULL;

	if (platform_allocate_region(&trampolinePage, B_PAGE_SIZE, 0, true) != B_OK)
		return NULL;

	ASSERT_ALWAYS((uint32_t)trampolinePage >= 0x88000000);
	return trampolinePage;
}


void
arch_start_kernel(addr_t kernelEntry)
{
	// Allocate virtual memory for kernel args
	struct kernel_args *kernelArgs = NULL;
	if (platform_allocate_region((void **)&kernelArgs,
			sizeof(struct kernel_args), 0, false) != B_OK)
		panic("Failed to allocate kernel args.");

	addr_t virtKernelArgs;
	platform_bootloader_address_to_kernel_address((void*)kernelArgs,
		&virtKernelArgs);

	// Allocate identity mapped region for entry.S trampoline
	void *trampolinePage = allocate_trampoline_page();
	if (trampolinePage == NULL)
		panic("Failed to allocate trampoline page.");

	memcpy(trampolinePage, (void *)arch_enter_kernel, B_PAGE_SIZE);
	arch_enter_kernel_t enter_kernel = (arch_enter_kernel_t)trampolinePage;

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

	// Copy final kernel args
	// This should be the last step before jumping to the kernel
	// as there are some fixups happening to kernel_args even in the last minute
	memcpy(kernelArgs, &gKernelArgs, sizeof(struct kernel_args));

	//smp_boot_other_cpus(final_pml4, kernelEntry);

	// Enter the kernel!
	dprintf("enter_kernel(ttbr0: 0x%08x, kernelArgs: 0x%08x, "
		"kernelEntry: 0x%08x, sp: 0x%08x)\n",
		final_ttbr0, (uint32_t)virtKernelArgs, (uint32_t)kernelEntry,
		(uint32_t)(gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size));

	enter_kernel(final_ttbr0, virtKernelArgs, kernelEntry,
		gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size);
}
