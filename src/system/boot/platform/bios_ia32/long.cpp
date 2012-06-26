/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


// Stop this from being included, it conflicts with the x86_64 version.
#define _KERNEL_ARCH_x86_DESCRIPTORS_H


#include "long.h"

#include <KernelExport.h>

#include <arch/x86_64/descriptors.h>
#include <arch_system_info.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stage2.h>
#include <boot/stdio.h>
#include <kernel.h>

#include "debug.h"
#include "mmu.h"


struct gdt_idt_descr {
	uint16 limit;
	addr_t base;
} _PACKED;


/*! Convert a 32-bit address to a 64-bit address. */
static inline uint64
fix_address(uint64 address)
{
	return address - KERNEL_BASE + KERNEL_BASE_64BIT;
}


template<typename Type>
inline void
fix_address(FixedWidthPointer<Type>& p)
{
	if(p != NULL)
		p.SetTo(fix_address(p.Get()));
}


static void
long_gdt_init()
{
	// Allocate memory for the GDT.
	segment_descriptor* gdt = (segment_descriptor*)
		mmu_allocate_page(&gKernelArgs.arch_args.phys_gdt);
	gKernelArgs.arch_args.vir_gdt = fix_address((addr_t)gdt);

	dprintf("GDT at phys 0x%lx, virt 0x%llx\n", gKernelArgs.arch_args.phys_gdt,
		gKernelArgs.arch_args.vir_gdt);

	clear_segment_descriptor(&gdt[0]);

	// Set up code/data segments (TSS segments set up later in the kernel).
	set_segment_descriptor(&gdt[KERNEL_CODE_SEG / 8], DT_CODE_EXECUTE_ONLY,
		DPL_KERNEL);
	set_segment_descriptor(&gdt[KERNEL_DATA_SEG / 8], DT_DATA_WRITEABLE,
		DPL_KERNEL);
	set_segment_descriptor(&gdt[USER_CODE_SEG / 8], DT_CODE_EXECUTE_ONLY,
		DPL_USER);
	set_segment_descriptor(&gdt[USER_DATA_SEG / 8], DT_DATA_WRITEABLE,
		DPL_USER);
}


static void
long_idt_init()
{
	interrupt_descriptor* idt = (interrupt_descriptor*)
		mmu_allocate_page(&gKernelArgs.arch_args.phys_idt);
	gKernelArgs.arch_args.vir_idt = fix_address((addr_t)idt);

	dprintf("IDT at phys 0x%lx, virt 0x%llx\n", gKernelArgs.arch_args.phys_idt,
		gKernelArgs.arch_args.vir_idt);

	// The 32-bit kernel gets an IDT with the loader's exception handlers until
	// it can set up its own. Can't do that here because they won't work after
	// switching to long mode. Therefore, just clear the IDT and leave the
	// kernel to set it up.
	memset(idt, 0, B_PAGE_SIZE);
}


static void
long_mmu_init()
{
	addr_t physicalAddress;

	// Allocate the top level PML4.
	uint64* pml4 = (uint64*)mmu_allocate_page(&gKernelArgs.arch_args.phys_pgdir);
	memset(pml4, 0, B_PAGE_SIZE);
	gKernelArgs.arch_args.vir_pgdir = (uint64)(addr_t)pml4;

	// Identity map the first 1GB of memory, do so using large pages.

	uint64* pdpt = (uint64*)mmu_allocate_page(&physicalAddress);
	memset(pdpt, 0, B_PAGE_SIZE);
	pml4[0] = physicalAddress | 0x3;

	uint64* pageDir = (uint64*)mmu_allocate_page(&physicalAddress);
	memset(pageDir, 0, B_PAGE_SIZE);
	pdpt[0] = physicalAddress | 0x3;

	for (uint32 i = 0; i < 512; i++) {
		pageDir[i] = (i * 0x200000) | 0x83;
	}

	// Allocate tables for the kernel mappings.

	pdpt = (uint64*)mmu_allocate_page(&physicalAddress);
	memset(pdpt, 0, B_PAGE_SIZE);
	pml4[511] = physicalAddress | 0x3;

	pageDir = (uint64*)mmu_allocate_page(&physicalAddress);
	memset(pageDir, 0, B_PAGE_SIZE);
	pdpt[510] = physicalAddress | 0x3;

	// Store the virtual memory usage information.
	gKernelArgs.virtual_allocated_range[0].start = KERNEL_BASE_64BIT;
	gKernelArgs.virtual_allocated_range[0].size = mmu_get_virtual_usage();
	gKernelArgs.num_virtual_allocated_ranges = 1;

	// We can now allocate page tables and duplicate the mappings across from
	// the 32-bit address space to them.
	uint64* pageTable = NULL;
	for (uint32 i = 0; i < gKernelArgs.virtual_allocated_range[0].size
			/ B_PAGE_SIZE; i++) {
		if ((i % 512) == 0) {
			pageTable = (uint64*)mmu_allocate_page(&physicalAddress);
			memset(pageTable, 0, B_PAGE_SIZE);
			pageDir[i / 512] = physicalAddress | 0x3;

			// Just performed another virtual allocation, account for it.
			gKernelArgs.virtual_allocated_range[0].size += B_PAGE_SIZE;
		}

		// Get the physical address to map.
		if (!mmu_get_virtual_mapping(KERNEL_BASE + (i * B_PAGE_SIZE),
				&physicalAddress))
			continue;

		pageTable[i % 512] = physicalAddress | 0x3;
	}

	gKernelArgs.arch_args.virtual_end = ROUNDUP(KERNEL_BASE_64BIT
		+ gKernelArgs.virtual_allocated_range[0].size, 0x200000);

	// Sort the address ranges.
	sort_address_ranges(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges);
	sort_address_ranges(gKernelArgs.physical_allocated_range,
		gKernelArgs.num_physical_allocated_ranges);
	sort_address_ranges(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges);

	dprintf("phys memory ranges:\n");
	for (uint32 i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
		dprintf("    base %#018" B_PRIx64 ", length %#018" B_PRIx64 "\n",
			gKernelArgs.physical_memory_range[i].start,
			gKernelArgs.physical_memory_range[i].size);
	}

	dprintf("allocated phys memory ranges:\n");
	for (uint32 i = 0; i < gKernelArgs.num_physical_allocated_ranges; i++) {
		dprintf("    base %#018" B_PRIx64 ", length %#018" B_PRIx64 "\n",
			gKernelArgs.physical_allocated_range[i].start,
			gKernelArgs.physical_allocated_range[i].size);
	}

	dprintf("allocated virt memory ranges:\n");
	for (uint32 i = 0; i < gKernelArgs.num_virtual_allocated_ranges; i++) {
		dprintf("    base %#018" B_PRIx64 ", length %#018" B_PRIx64 "\n",
			gKernelArgs.virtual_allocated_range[i].start,
			gKernelArgs.virtual_allocated_range[i].size);
	}
}


static void
convert_preloaded_image(preloaded_elf64_image* image)
{
	fix_address(image->next);
	fix_address(image->name);
	fix_address(image->debug_string_table);
	fix_address(image->syms);
	fix_address(image->rel);
	fix_address(image->rela);
	fix_address(image->pltrel);
	fix_address(image->debug_symbols);
}


/*!	Convert all addresses in kernel_args to 64-bit addresses. */
static void
convert_kernel_args()
{
	fix_address(gKernelArgs.boot_volume);
	fix_address(gKernelArgs.vesa_modes);
	fix_address(gKernelArgs.edid_info);
	fix_address(gKernelArgs.debug_output);
	fix_address(gKernelArgs.boot_splash);
	fix_address(gKernelArgs.arch_args.apic);
	fix_address(gKernelArgs.arch_args.hpet);

	convert_preloaded_image(static_cast<preloaded_elf64_image*>(
		gKernelArgs.kernel_image.Pointer()));
	fix_address(gKernelArgs.kernel_image);

	// Iterate over the preloaded images. Must save the next address before
	// converting, as the next pointer will be converted.
	preloaded_image* image = gKernelArgs.preloaded_images;
	fix_address(gKernelArgs.preloaded_images);
	while (image) {
		preloaded_image* next = image->next;
		convert_preloaded_image(static_cast<preloaded_elf64_image*>(image));
		image = next;
	}

	// Set correct kernel stack addresses.
	for (uint32 i = 0; i < gKernelArgs.num_cpus; i++) {
		gKernelArgs.cpu_kstack[i].start
			= fix_address(gKernelArgs.cpu_kstack[i].start);
	}

	// Fix driver settings files.
	driver_settings_file* file = gKernelArgs.driver_settings;
	fix_address(gKernelArgs.driver_settings);
	while (file) {
		driver_settings_file* next = file->next;
		fix_address(file->next);
		fix_address(file->buffer);
		file = next;
	}
}


void
long_start_kernel()
{
	// Check whether long mode is supported.
	cpuid_info info;
	get_current_cpuid(&info, 0x80000001);
	if ((info.regs.edx & (1<<29)) == 0)
		panic("64-bit kernel requires a 64-bit CPU");

	preloaded_elf64_image *image = static_cast<preloaded_elf64_image *>(
		gKernelArgs.kernel_image.Pointer());

	// TODO: x86_64 SMP, disable for now.
	gKernelArgs.num_cpus = 1;

	long_gdt_init();
	long_idt_init();
	long_mmu_init();
	convert_kernel_args();

	debug_cleanup();

	// Calculate the arguments for long_enter_kernel().
	uint64 entry = image->elf_header.e_entry;
	uint64 stackTop = gKernelArgs.cpu_kstack[0].start
		+ gKernelArgs.cpu_kstack[0].size;
	uint64 kernelArgs = (addr_t)&gKernelArgs;

	dprintf("kernel entry at 0x%llx, stack 0x%llx, args 0x%llx\n", entry,
		stackTop, kernelArgs);

	// We're about to enter the kernel -- disable console output.
	stdout = NULL;

	// Load the new GDT. The physical address is used because long_enter_kernel
	// disables 32-bit paging.
	gdt_idt_descr gdtr = { GDT_LIMIT - 1, gKernelArgs.arch_args.phys_gdt };
	asm volatile("lgdt %0" :: "m"(gdtr));

	// Enter the kernel!
	long_enter_kernel(gKernelArgs.arch_args.phys_pgdir, entry, stackTop,
		kernelArgs, 0);
	panic("Shouldn't get here");
}

