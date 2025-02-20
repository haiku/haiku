/*
 * Copyright 2021-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <algorithm>

#include <kernel.h>
#include <arch_kernel.h>
#include <arch/cpu.h>
#include <arch/x86/descriptors.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <efi/types.h>
#include <efi/boot-services.h>

#include "efi_platform.h"
#include "generic_mmu.h"
#include "mmu.h"


//#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


//#define TRACE_MEMORY_MAP
//#define TRACE_PAGE_DIRECTORY

// Ignore memory below 1M and above 64GB (maximum amount of physical memory on x86 with PAE)
#define PHYSICAL_MEMORY_LOW		0x00100000
#define PHYSICAL_MEMORY_HIGH	0x1000000000ull

#define VADDR_TO_PDENT(va)		(((va) / B_PAGE_SIZE) / 1024)
#define VADDR_TO_PTENT(va)		(((va) / B_PAGE_SIZE) % 1024)
#define X86_PDE_ADDRESS_MASK	0xfffff000
#define X86_PTE_ADDRESS_MASK	0xfffff000

#define ALIGN_PAGEDIR			B_PAGE_SIZE


struct gdt_idt_descr {
	uint16_t	limit;
	uint32_t	base;
} _PACKED;


static const uint32_t kDefaultPageTableFlags = 0x07;      // present, user, R/W


static uint32_t *sPageDirectory = NULL;


#ifdef TRACE_PAGE_DIRECTORY
static void
dump_page_dir(void)
{
	dprintf("=== Page Directory ===\n");
	for (uint32_t i = 0; i < 1024; i++) {
		uint32_t directoryEntry = sPageDirectory[i];
		if (directoryEntry != 0) {
			dprintf("virt 0x%08x --> page table 0x%08x type 0x%08x\n",
				i << 22, directoryEntry & X86_PDE_ADDRESS_MASK,
				directoryEntry & (~X86_PDE_ADDRESS_MASK));
			uint32_t *pageTable = (uint32_t *)(directoryEntry & X86_PDE_ADDRESS_MASK);
			for (uint32_t j = 0; j < 1024; j++) {
				uint32_t tableEntry = pageTable[j];
				if (tableEntry != 0) {
					dprintf("virt 0x%08x     --> page 0x%08x type+flags 0x%08x\n",
						(i << 22) | (j << 12),
						tableEntry & X86_PTE_ADDRESS_MASK,
						tableEntry & (~X86_PTE_ADDRESS_MASK));
				}
			}
		}
	}
}
#endif


static uint32_t *
get_next_page_table(void)
{
	uint32_t *pageTable = (uint32_t *)mmu_allocate_page();
	memset(pageTable, 0, B_PAGE_SIZE);
	return pageTable;
}


void
arch_mmu_init_gdt(gdt_idt_descr &bootGDTDescriptor)
{
	segment_descriptor *bootGDT = NULL;

	if (platform_allocate_region((void **)&bootGDT,
			BOOT_GDT_SEGMENT_COUNT * sizeof(segment_descriptor), 0) != B_OK) {
		panic("Failed to allocate GDT.\n");
	}

	STATIC_ASSERT(BOOT_GDT_SEGMENT_COUNT > KERNEL_CODE_SEGMENT
		&& BOOT_GDT_SEGMENT_COUNT > KERNEL_DATA_SEGMENT
		&& BOOT_GDT_SEGMENT_COUNT > USER_CODE_SEGMENT
		&& BOOT_GDT_SEGMENT_COUNT > USER_DATA_SEGMENT);

	// set up a new gdt

	// put standard segment descriptors in GDT
	clear_segment_descriptor(&bootGDT[0]);

	// seg 0x08 - kernel 4GB code
	set_segment_descriptor(&bootGDT[KERNEL_CODE_SEGMENT], 0, 0xffffffff,
		DT_CODE_READABLE, DPL_KERNEL);

	// seg 0x10 - kernel 4GB data
	set_segment_descriptor(&bootGDT[KERNEL_DATA_SEGMENT], 0, 0xffffffff,
		DT_DATA_WRITEABLE, DPL_KERNEL);

	// seg 0x1b - ring 3 user 4GB code
	set_segment_descriptor(&bootGDT[USER_CODE_SEGMENT], 0, 0xffffffff,
		DT_CODE_READABLE, DPL_USER);

	// seg 0x23 - ring 3 user 4GB data
	set_segment_descriptor(&bootGDT[USER_DATA_SEGMENT], 0, 0xffffffff,
		DT_DATA_WRITEABLE, DPL_USER);

	addr_t virtualGDT;
	platform_bootloader_address_to_kernel_address(bootGDT, &virtualGDT);

	bootGDTDescriptor.limit = BOOT_GDT_SEGMENT_COUNT * sizeof(segment_descriptor);
	bootGDTDescriptor.base = (uint32_t)virtualGDT;

	TRACE("gdt phys 0x%08x virt 0x%08" B_PRIxADDR " desc 0x%08x\n",
		(uint32_t)bootGDT, virtualGDT,
		(uint32_t)&gBootGDTDescriptor);
	TRACE("gdt limit=%d base=0x%08x\n",
		bootGDTDescriptor.limit, bootGDTDescriptor.base);
}


static void
map_page(addr_t virtAddr, phys_addr_t physAddr, uint32_t flags)
{
	physAddr &= ~(B_PAGE_SIZE - 1);

	uint32_t *pageTable = NULL;
	uint32_t pageDirectoryIndex = VADDR_TO_PDENT(virtAddr);
	uint32_t pageDirectoryEntry = sPageDirectory[pageDirectoryIndex];

	if (pageDirectoryEntry == 0) {
		//TRACE("get next page table for address 0x%08" B_PRIxADDR "\n",
		//	virtAddr);
		pageTable = get_next_page_table();
		sPageDirectory[pageDirectoryIndex] = (uint32_t)pageTable | kDefaultPageTableFlags;
	} else {
		pageTable = (uint32_t *)(pageDirectoryEntry & X86_PDE_ADDRESS_MASK);
	}

	uint32_t pageTableIndex = VADDR_TO_PTENT(virtAddr);
	pageTable[pageTableIndex] = physAddr | flags;
}


static void
map_range(addr_t virtAddr, phys_addr_t physAddr, size_t size, uint32_t flags)
{
	//TRACE("map 0x%08" B_PRIxADDR " --> 0x%08" B_PRIxPHYSADDR
	//	", len=0x%08" B_PRIxSIZE ", flags=0x%08" PRIx32 "\n",
	//	virtAddr, physAddr, size, flags);

	for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
		map_page(virtAddr + offset, physAddr + offset, flags);
	}

	if (virtAddr >= KERNEL_LOAD_BASE)
		ASSERT_ALWAYS(insert_virtual_allocated_range(virtAddr, size) >= B_OK);
}


void
arch_mmu_post_efi_setup(size_t memoryMapSize,
	efi_memory_descriptor *memoryMap, size_t descriptorSize,
	uint32_t descriptorVersion)
{
	build_physical_allocated_list(memoryMapSize, memoryMap,
		descriptorSize, descriptorVersion);

	// Switch EFI to virtual mode, using the kernel pmap.
	kRuntimeServices->SetVirtualAddressMap(memoryMapSize, descriptorSize,
		descriptorVersion, memoryMap);

#ifdef TRACE_MEMORY_MAP
	dprintf("phys memory ranges:\n");
	for (uint32_t i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
		uint64 start = gKernelArgs.physical_memory_range[i].start;
		uint64 size = gKernelArgs.physical_memory_range[i].size;
		dprintf("    0x%08" B_PRIx64 "-0x%08" B_PRIx64 ", length 0x%08" B_PRIx64 "\n",
			start, start + size, size);
	}

	dprintf("allocated phys memory ranges:\n");
	for (uint32_t i = 0; i < gKernelArgs.num_physical_allocated_ranges; i++) {
		uint64 start = gKernelArgs.physical_allocated_range[i].start;
		uint64 size = gKernelArgs.physical_allocated_range[i].size;
		dprintf("    0x%08" B_PRIx64 "-0x%08" B_PRIx64 ", length 0x%08" B_PRIx64 "\n",
			start, start + size, size);
	}

	dprintf("allocated virt memory ranges:\n");
	for (uint32_t i = 0; i < gKernelArgs.num_virtual_allocated_ranges; i++) {
		uint64 start = gKernelArgs.virtual_allocated_range[i].start;
		uint64 size = gKernelArgs.virtual_allocated_range[i].size;
		dprintf("    0x%08" B_PRIx64 "-0x%08" B_PRIx64 ", length 0x%08" B_PRIx64 "\n",
			start, start + size, size);
	}
#endif
}


static void
arch_mmu_allocate_page_directory(void)
{
	if (platform_allocate_region((void **)&sPageDirectory,
			B_PAGE_SIZE + ALIGN_PAGEDIR, 0) != B_OK)
		panic("Failed to allocate page directory.");
	sPageDirectory = (uint32_t *)ROUNDUP((uint32_t)sPageDirectory, ALIGN_PAGEDIR);
	memset(sPageDirectory, 0, B_PAGE_SIZE);

	TRACE("sPageDirectory  = 0x%08x\n", (uint32_t)sPageDirectory);
}


uint32_t
arch_mmu_generate_post_efi_page_tables(size_t memoryMapSize,
	efi_memory_descriptor *memoryMap, size_t descriptorSize,
	uint32_t descriptorVersion)
{
	build_physical_memory_list(memoryMapSize, memoryMap,
		descriptorSize, descriptorVersion,
		PHYSICAL_MEMORY_LOW, PHYSICAL_MEMORY_HIGH);

	//TODO: find out how to map EFI runtime services
	//they are not mapped for now because the kernel doesn't use them anyway
#if 0
	addr_t memoryMapAddr = (addr_t)memoryMap;
	for (size_t i = 0; i < memoryMapSize / descriptorSize; ++i) {
		efi_memory_descriptor* entry =
			(efi_memory_descriptor *)(memoryMapAddr + i * descriptorSize);
		if ((entry->Attribute & EFI_MEMORY_RUNTIME) != 0)
			map_range(entry->VirtualStart, entry->PhysicalStart,
				entry->NumberOfPages * B_PAGE_SIZE,
				kDefaultPageFlags);
	}
#endif

	void* cookie = NULL;
	addr_t vaddr;
	phys_addr_t paddr;
	size_t size;
	while (mmu_next_region(&cookie, &vaddr, &paddr, &size)) {
		map_range(vaddr, paddr, size,
			kDefaultPageFlags);
	}

	// identity mapping for first 1MB
	map_range((addr_t)0, (phys_addr_t)0, 1024*1024, kDefaultPageFlags);

	sort_address_ranges(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges);

	// Map the page directory into kernel space at 0xffc00000-0xffffffff
	// this enables a mmu trick where the 4 MB region that this pgdir entry
	// represents now maps the 4MB of potential pagetables that the pgdir
	// points to. Thrown away later in VM bringup, but useful for now.
	sPageDirectory[1023] = (uint32_t)sPageDirectory | kDefaultPageFlags;

	addr_t virtPageDirectory;
	platform_bootloader_address_to_kernel_address((void*)sPageDirectory, &virtPageDirectory);

	gKernelArgs.arch_args.phys_pgdir = (uint32_t)sPageDirectory;
	gKernelArgs.arch_args.vir_pgdir = (uint32_t)virtPageDirectory;
	gKernelArgs.arch_args.page_hole = 0xffc00000;
	gKernelArgs.arch_args.virtual_end
		= gKernelArgs.virtual_allocated_range[gKernelArgs.num_virtual_allocated_ranges-1].start
		+ gKernelArgs.virtual_allocated_range[gKernelArgs.num_virtual_allocated_ranges-1].size;

	TRACE("gKernelArgs.arch_args.phys_pgdir  = 0x%08" B_PRIx32 "\n",
		gKernelArgs.arch_args.phys_pgdir);
	TRACE("gKernelArgs.arch_args.vir_pgdir   = 0x%08" B_PRIx64 "\n",
		gKernelArgs.arch_args.vir_pgdir);
	TRACE("gKernelArgs.arch_args.page_hole   = 0x%08" B_PRIx64 "\n",
		gKernelArgs.arch_args.page_hole);
	TRACE("gKernelArgs.arch_args.virtual_end = 0x%08" B_PRIx64 "\n",
		gKernelArgs.arch_args.virtual_end);

#ifdef TRACE_PAGE_DIRECTORY
	dump_page_dir();
#endif

	return (uint32_t)sPageDirectory;
}


void
arch_mmu_init(void)
{
	arch_mmu_allocate_page_directory();
}
