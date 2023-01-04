/*
 * Copyright 2019-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#include "efi_platform.h"
#include "generic_mmu.h"
#include "mmu.h"
#include "serial.h"

#include "aarch64.h"

extern "C" void arch_enter_kernel(
	struct kernel_args* kernelArgs, addr_t kernelEntry, addr_t kernelStackTop);

extern const char* granule_type_str(int tg);

extern uint32_t arch_mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor* memory_map, size_t descriptor_size, uint32_t descriptor_version);

extern void arch_mmu_post_efi_setup(size_t memory_map_size, efi_memory_descriptor* memory_map,
	size_t descriptor_size, uint32_t descriptor_version);

extern void arch_mmu_setup_EL1(uint64 tcr);


void
arch_convert_kernel_args(void)
{
	fix_address(gKernelArgs.arch_args.fdt);
}


void
arch_start_kernel(addr_t kernelEntry)
{
	// Prepare to exit EFI boot services.
	// Read the memory map.
	// First call is to determine the buffer size.
	size_t memory_map_size = 0;
	efi_memory_descriptor dummy;
	efi_memory_descriptor* memory_map;
	size_t map_key;
	size_t descriptor_size;
	uint32_t descriptor_version;
	if (kBootServices->GetMemoryMap(
			&memory_map_size, &dummy, &map_key, &descriptor_size, &descriptor_version)
		!= EFI_BUFFER_TOO_SMALL) {
		panic("Unable to determine size of system memory map");
	}

	// Allocate a buffer twice as large as needed just in case it gets bigger
	// between calls to ExitBootServices.
	size_t actual_memory_map_size = memory_map_size * 2;
	memory_map = (efi_memory_descriptor*) kernel_args_malloc(actual_memory_map_size);

	if (memory_map == NULL)
		panic("Unable to allocate memory map.");

	// Read (and print) the memory map.
	memory_map_size = actual_memory_map_size;
	if (kBootServices->GetMemoryMap(
			&memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version)
		!= EFI_SUCCESS) {
		panic("Unable to fetch system memory map.");
	}

	addr_t addr = (addr_t) memory_map;
	efi_physical_addr loaderCode = 0LL;
	dprintf("System provided memory map:\n");
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor*) (addr + i * descriptor_size);
		dprintf("  phys: 0x%0lx-0x%0lx, virt: 0x%0lx-0x%0lx, size = 0x%0lx, type: %s (%#x), attr: "
				"%#lx\n",
			entry->PhysicalStart, entry->PhysicalStart + entry->NumberOfPages * B_PAGE_SIZE,
			entry->VirtualStart, entry->VirtualStart + entry->NumberOfPages * B_PAGE_SIZE,
			entry->NumberOfPages * B_PAGE_SIZE, memory_region_type_str(entry->Type), entry->Type,
			entry->Attribute);
		if (entry->Type == EfiLoaderCode)
			loaderCode = entry->PhysicalStart;
	}
	// This is where our efi loader got relocated, therefore we need to use this
	// offset for properly align symbols
	dprintf("Efi loader symbols offset: 0x%0lx:\n", loaderCode);

	/*
	*   "The AArch64 exception model is made up of a number of exception levels
	*    (EL0 - EL3), with EL0 and EL1 having a secure and a non-secure
	*    counterpart.  EL2 is the hypervisor level and exists only in non-secure
	*    mode. EL3 is the highest priority level and exists only in secure mode."
	*
	*	"2.3 UEFI System Environment and Configuration
	*    The resident UEFI boot-time environment shall use the highest non-secure
	*    privilege level available. The exact meaning of this is architecture
	*    dependent, as detailed below."

	*	"2.3.1 AArch64 Exception Levels
	*    On AArch64 UEFI shall execute as 64-bit code at either EL1 or EL2,
	*    depending on whether or not virtualization is available at OS load time."
	*/
	uint64 el = arch_exception_level();
	dprintf("Current Exception Level EL%1lx\n", el);
	dprintf("TTBR0: %" B_PRIx64 " TTBRx: %" B_PRIx64 " SCTLR: %" B_PRIx64 " TCR: %" B_PRIx64 "\n",
		arch_mmu_base_register(), arch_mmu_base_register(true), _arch_mmu_get_sctlr(),
		_arch_mmu_get_tcr());

	if (arch_mmu_enabled()) {
		dprintf("MMU Enabled, Granularity %s, bits %d\n", granule_type_str(arch_mmu_user_granule()),
			arch_mmu_user_address_bits());

		dprintf("Kernel entry accessibility W: %x R: %x\n", arch_mmu_write_access(kernelEntry),
			arch_mmu_read_access(kernelEntry));

		if (el == 1) {
			// Disable CACHE & MMU before dealing with TTBRx
			arch_cache_disable();
		}
	}

	// Generate page tables for use after ExitBootServices.
	arch_mmu_generate_post_efi_page_tables(
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
	serial_disable();
	while (true) {
		if (kBootServices->ExitBootServices(kImage, map_key) == EFI_SUCCESS) {
			// Disconnect from EFI serial_io / stdio services
			serial_kernel_handoff();
			dprintf("Unhooked from EFI serial services\n");
			break;
		}

		memory_map_size = actual_memory_map_size;
		if (kBootServices->GetMemoryMap(
				&memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version)
			!= EFI_SUCCESS) {
			panic("Unable to fetch system memory map.");
		}
	}

	// Update EFI, generate final kernel physical memory map, etc.
	arch_mmu_post_efi_setup(memory_map_size, memory_map, descriptor_size, descriptor_version);
	serial_enable();

	switch (el) {
		case 1:
			arch_mmu_setup_EL1(READ_SPECIALREG(TCR_EL1));
			break;
		case 2:
			arch_mmu_setup_EL1(READ_SPECIALREG(TCR_EL2));
			arch_cache_disable();
			_arch_transition_EL2_EL1();
			break;
		default:
			panic("Unexpected Exception Level\n");
			break;
	}

	arch_cache_enable();

	// smp_boot_other_cpus(final_pml4, kernelEntry, (addr_t)&gKernelArgs);

	if (arch_mmu_read_access(kernelEntry)
		&& arch_mmu_read_access(gKernelArgs.cpu_kstack[0].start)) {
		// Enter the kernel!
		arch_enter_kernel(&gKernelArgs, kernelEntry,
			gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size);
	} else {
		// _arch_exception_panic("Kernel or Stack memory not accessible\n", __LINE__);
		panic("Kernel or Stack memory not accessible\n");
	}
}
