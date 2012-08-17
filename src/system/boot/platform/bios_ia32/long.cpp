/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include "long.h"

#include <algorithm>

#include <KernelExport.h>

// Include the x86_64 version of descriptors.h
#define __x86_64__
#include <arch/x86/descriptors.h>
#undef __x86_64__

#include <arch_system_info.h>
#include <boot/platform.h>
#include <boot/heap.h>
#include <boot/stage2.h>
#include <boot/stdio.h>
#include <kernel.h>

#include "debug.h"
#include "smp.h"
#include "mmu.h"


static const uint64 kTableMappingFlags = 0x7;
static const uint64 kLargePageMappingFlags = 0x183;
static const uint64 kPageMappingFlags = 0x103;
	// Global, R/W, Present

extern "C" void long_enter_kernel(int currentCPU, uint64 stackTop);

extern uint32 gLongPhysicalGDT;
extern uint64 gLongVirtualGDT;
extern uint32 gLongPhysicalPML4;
extern uint64 gLongKernelEntry;


/*! Convert a 32-bit address to a 64-bit address. */
static inline uint64
fix_address(uint64 address)
{
	return address - KERNEL_LOAD_BASE + KERNEL_LOAD_BASE_64_BIT;
}


template<typename Type>
inline void
fix_address(FixedWidthPointer<Type>& p)
{
	if (p != NULL)
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

	// Used by long_enter_kernel().
	gLongPhysicalGDT = gKernelArgs.arch_args.phys_gdt;
	gLongVirtualGDT = gKernelArgs.arch_args.vir_gdt;
}


static void
long_idt_init()
{
	interrupt_descriptor* idt = (interrupt_descriptor*)
		mmu_allocate_page(&gKernelArgs.arch_args.phys_idt);
	gKernelArgs.arch_args.vir_idt = fix_address((addr_t)idt);

	dprintf("IDT at phys %#lx, virt %#llx\n", gKernelArgs.arch_args.phys_idt,
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
	uint64* pml4;
	uint64* pdpt;
	uint64* pageDir;
	uint64* pageTable;
	addr_t physicalAddress;

	// Allocate the top level PML4.
	pml4 = (uint64*)mmu_allocate_page(&gKernelArgs.arch_args.phys_pgdir);
	memset(pml4, 0, B_PAGE_SIZE);
	gKernelArgs.arch_args.vir_pgdir = fix_address((uint64)(addr_t)pml4);

	// Store the virtual memory usage information.
	gKernelArgs.virtual_allocated_range[0].start = KERNEL_LOAD_BASE_64_BIT;
	gKernelArgs.virtual_allocated_range[0].size = mmu_get_virtual_usage();
	gKernelArgs.num_virtual_allocated_ranges = 1;
	gKernelArgs.arch_args.virtual_end = ROUNDUP(KERNEL_LOAD_BASE_64_BIT
		+ gKernelArgs.virtual_allocated_range[0].size, 0x200000);

	// Find the highest physical memory address. We map all physical memory
	// into the kernel address space, so we want to make sure we map everything
	// we have available.
	uint64 maxAddress = 0;
	for (uint32 i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
		maxAddress = std::max(maxAddress,
			gKernelArgs.physical_memory_range[i].start
				+ gKernelArgs.physical_memory_range[i].size);
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

	pdpt = (uint64*)mmu_allocate_page(&physicalAddress);
	memset(pdpt, 0, B_PAGE_SIZE);
	pml4[510] = physicalAddress | kTableMappingFlags;
	pml4[0] = physicalAddress | kTableMappingFlags;

	for (uint64 i = 0; i < maxAddress; i += 0x40000000) {
		pageDir = (uint64*)mmu_allocate_page(&physicalAddress);
		memset(pageDir, 0, B_PAGE_SIZE);
		pdpt[i / 0x40000000] = physicalAddress | kTableMappingFlags;

		for (uint64 j = 0; j < 0x40000000; j += 0x200000) {
			pageDir[j / 0x200000] = (i + j) | kLargePageMappingFlags;
		}

		mmu_free(pageDir, B_PAGE_SIZE);
	}

	mmu_free(pdpt, B_PAGE_SIZE);

	// Allocate tables for the kernel mappings.

	pdpt = (uint64*)mmu_allocate_page(&physicalAddress);
	memset(pdpt, 0, B_PAGE_SIZE);
	pml4[511] = physicalAddress | kTableMappingFlags;

	pageDir = (uint64*)mmu_allocate_page(&physicalAddress);
	memset(pageDir, 0, B_PAGE_SIZE);
	pdpt[510] = physicalAddress | kTableMappingFlags;

	// We can now allocate page tables and duplicate the mappings across from
	// the 32-bit address space to them.
	pageTable = NULL;
	for (uint32 i = 0; i < gKernelArgs.virtual_allocated_range[0].size
			/ B_PAGE_SIZE; i++) {
		if ((i % 512) == 0) {
			if (pageTable)
				mmu_free(pageTable, B_PAGE_SIZE);

			pageTable = (uint64*)mmu_allocate_page(&physicalAddress);
			memset(pageTable, 0, B_PAGE_SIZE);
			pageDir[i / 512] = physicalAddress | kTableMappingFlags;
		}

		// Get the physical address to map.
		if (!mmu_get_virtual_mapping(KERNEL_LOAD_BASE + (i * B_PAGE_SIZE),
				&physicalAddress))
			continue;

		pageTable[i % 512] = physicalAddress | kPageMappingFlags;
	}

	if (pageTable)
		mmu_free(pageTable, B_PAGE_SIZE);
	mmu_free(pageDir, B_PAGE_SIZE);
	mmu_free(pdpt, B_PAGE_SIZE);

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

	gLongPhysicalPML4 = gKernelArgs.arch_args.phys_pgdir;
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
	while (image != NULL) {
		preloaded_image* next = image->next;
		convert_preloaded_image(static_cast<preloaded_elf64_image*>(image));
		image = next;
	}

	// Set correct kernel args range addresses.
	dprintf("kernel args ranges:\n");
	for (uint32 i = 0; i < gKernelArgs.num_kernel_args_ranges; i++) {
		gKernelArgs.kernel_args_range[i].start = fix_address(
			gKernelArgs.kernel_args_range[i].start);
		dprintf("    base %#018" B_PRIx64 ", length %#018" B_PRIx64 "\n",
			gKernelArgs.kernel_args_range[i].start,
			gKernelArgs.kernel_args_range[i].size);
	}

	// Fix driver settings files.
	driver_settings_file* file = gKernelArgs.driver_settings;
	fix_address(gKernelArgs.driver_settings);
	while (file != NULL) {
		driver_settings_file* next = file->next;
		fix_address(file->next);
		fix_address(file->buffer);
		file = next;
	}
}


static void
long_smp_start_kernel(void)
{
	uint32 cpu = smp_get_current_cpu();

	// Important.  Make sure supervisor threads can fault on read only pages...
	asm("movl %%eax, %%cr0" : : "a" ((1 << 31) | (1 << 16) | (1 << 5) | 1));
	asm("cld");
	asm("fninit");

	// Fix our kernel stack address.
	gKernelArgs.cpu_kstack[cpu].start
		= fix_address(gKernelArgs.cpu_kstack[cpu].start);

	long_enter_kernel(cpu, gKernelArgs.cpu_kstack[cpu].start
		+ gKernelArgs.cpu_kstack[cpu].size);

	panic("Shouldn't get here");
}


void
long_start_kernel()
{
	// Check whether long mode is supported.
	cpuid_info info;
	get_current_cpuid(&info, 0x80000001);
	if ((info.regs.edx & (1 << 29)) == 0)
		panic("64-bit kernel requires a 64-bit CPU");

	preloaded_elf64_image *image = static_cast<preloaded_elf64_image *>(
		gKernelArgs.kernel_image.Pointer());

	smp_init_other_cpus();

	long_gdt_init();
	long_idt_init();
	long_mmu_init();
	convert_kernel_args();

	debug_cleanup();

	// Save the kernel entry point address.
	gLongKernelEntry = image->elf_header.e_entry;
	dprintf("kernel entry at %#llx\n", gLongKernelEntry);

	// Fix our kernel stack address.
	gKernelArgs.cpu_kstack[0].start
		= fix_address(gKernelArgs.cpu_kstack[0].start);

	// We're about to enter the kernel -- disable console output.
	stdout = NULL;

	smp_boot_other_cpus(long_smp_start_kernel);

	// Enter the kernel!
	long_enter_kernel(0, gKernelArgs.cpu_kstack[0].start
		+ gKernelArgs.cpu_kstack[0].size);

	panic("Shouldn't get here");
}
