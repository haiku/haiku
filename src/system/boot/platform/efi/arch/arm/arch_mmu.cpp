/*
 * Copyright 2019-2023 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <algorithm>

#include <arm_mmu.h>
#include <kernel.h>
#include <arch_kernel.h>
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


static constexpr bool kTraceMemoryMap = false;
static constexpr bool kTracePageDirectory = false;


// Ignore memory above 512GB
#define PHYSICAL_MEMORY_LOW		0x00000000
#define PHYSICAL_MEMORY_HIGH	0x8000000000ull

#define USER_VECTOR_ADDR_HIGH	0xffff0000

#define ALIGN_PAGEDIR			(1024 * 16)
#define MAX_PAGE_TABLES			192
#define PAGE_TABLE_AREA_SIZE	(MAX_PAGE_TABLES * ARM_MMU_L2_COARSE_TABLE_SIZE)

static uint32_t *sPageDirectory = NULL;
static uint32_t *sNextPageTable = NULL;
static uint32_t *sLastPageTable = NULL;
static uint32_t *sVectorTable = (uint32_t*)USER_VECTOR_ADDR_HIGH;


static void
dump_page_dir(void)
{
	dprintf("=== Page Directory ===\n");
	for (uint32_t i = 0; i < ARM_MMU_L1_TABLE_ENTRY_COUNT; i++) {
		uint32 directoryEntry = sPageDirectory[i];
		if (directoryEntry != 0) {
			dprintf("virt 0x%08x --> page table 0x%08x type 0x%08x\n",
				i << 20, directoryEntry & ARM_PDE_ADDRESS_MASK,
				directoryEntry & ARM_PDE_TYPE_MASK);
			uint32_t *pageTable = (uint32_t *)(directoryEntry & ARM_PDE_ADDRESS_MASK);
			for (uint32_t j = 0; j < ARM_MMU_L2_COARSE_ENTRY_COUNT; j++) {
				uint32 tableEntry = pageTable[j];
				if (tableEntry != 0) {
					dprintf("virt 0x%08x     --> page 0x%08x type+flags 0x%08x\n",
						(i << 20) | (j << 12),
						tableEntry & ARM_PTE_ADDRESS_MASK,
						tableEntry & (~ARM_PTE_ADDRESS_MASK));
				}
			}
		}
	}
}


static uint32 *
get_next_page_table(void)
{
	uint32 *pageTable = sNextPageTable;
	sNextPageTable += ARM_MMU_L2_COARSE_ENTRY_COUNT;
	if (sNextPageTable >= sLastPageTable)
		panic("ran out of page tables\n");
	return pageTable;
}


static void
map_page(addr_t virtAddr, phys_addr_t physAddr, uint32_t flags)
{
	physAddr &= ~(B_PAGE_SIZE - 1);

	uint32 *pageTable = NULL;
	uint32 pageDirectoryIndex = VADDR_TO_PDENT(virtAddr);
	uint32 pageDirectoryEntry = sPageDirectory[pageDirectoryIndex];

	if (pageDirectoryEntry == 0) {
		pageTable = get_next_page_table();
		sPageDirectory[pageDirectoryIndex] = (uint32_t)pageTable | ARM_MMU_L1_TYPE_COARSE;
	} else {
		pageTable = (uint32 *)(pageDirectoryEntry & ARM_PDE_ADDRESS_MASK);
	}

	uint32 pageTableIndex = VADDR_TO_PTENT(virtAddr);
	pageTable[pageTableIndex] = physAddr | flags | ARM_MMU_L2_TYPE_SMALLNEW;
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


static void
insert_virtual_range_to_keep(uint64 start, uint64 size)
{
	status_t status = insert_address_range(
		gKernelArgs.arch_args.virtual_ranges_to_keep,
		&gKernelArgs.arch_args.num_virtual_ranges_to_keep,
		MAX_VIRTUAL_RANGES_TO_KEEP, start, size);

	if (status == B_ENTRY_NOT_FOUND)
		panic("too many virtual ranges to keep");
	else if (status != B_OK)
		panic("failed to add virtual range to keep");
}


static addr_t
map_range_to_new_area(addr_t start, size_t size, uint32_t flags)
{
	if (size == 0)
		return 0;

	phys_addr_t physAddr = ROUNDDOWN(start, B_PAGE_SIZE);
	size_t alignedSize = ROUNDUP(size + (start - physAddr), B_PAGE_SIZE);
	addr_t virtAddr = get_next_virtual_address(alignedSize);

	map_range(virtAddr, physAddr, alignedSize, flags);
	insert_virtual_range_to_keep(virtAddr, alignedSize);

	return virtAddr + (start - physAddr);
}


static void
map_range_to_new_area(addr_range& range, uint32_t flags)
{
	range.start = map_range_to_new_area(range.start, range.size, flags);
}


static void
map_range_to_new_area(efi_memory_descriptor *entry, uint32_t flags)
{
	uint64_t size = entry->NumberOfPages * B_PAGE_SIZE;
	entry->VirtualStart = map_range_to_new_area(entry->PhysicalStart, size, flags);
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

	if (kTraceMemoryMap) {
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

		dprintf("virt memory ranges to keep:\n");
		for (uint32_t i = 0; i < gKernelArgs.arch_args.num_virtual_ranges_to_keep; i++) {
			uint32 start = gKernelArgs.arch_args.virtual_ranges_to_keep[i].start;
			uint32 size = gKernelArgs.arch_args.virtual_ranges_to_keep[i].size;
			dprintf("    0x%08" B_PRIx32 "-0x%08" B_PRIx32 ", length 0x%08" B_PRIx32 "\n",
				start, start + size, size);
		}
	}
}


static void
arch_mmu_allocate_page_tables(void)
{
	if (platform_allocate_region((void **)&sPageDirectory,
			ARM_MMU_L1_TABLE_SIZE + ALIGN_PAGEDIR + PAGE_TABLE_AREA_SIZE, 0) != B_OK)
		panic("Failed to allocate page directory.");
	sPageDirectory = (uint32 *)ROUNDUP((uint32)sPageDirectory, ALIGN_PAGEDIR);
	memset(sPageDirectory, 0, ARM_MMU_L1_TABLE_SIZE);

	sNextPageTable = (uint32*)((uint32)sPageDirectory + ARM_MMU_L1_TABLE_SIZE);
	sLastPageTable = (uint32*)((uint32)sNextPageTable + PAGE_TABLE_AREA_SIZE);

	memset(sNextPageTable, 0, PAGE_TABLE_AREA_SIZE);

	TRACE("sPageDirectory = 0x%08x\n", (uint32)sPageDirectory);
	TRACE("sNextPageTable = 0x%08x\n", (uint32)sNextPageTable);
	TRACE("sLastPageTable = 0x%08x\n", (uint32)sLastPageTable);
}


static void
arch_mmu_allocate_vector_table(void)
{
	void *vectorTable = NULL;
	if (platform_allocate_region(&vectorTable, B_PAGE_SIZE, 0) != B_OK)
		panic("Failed to allocate vector table.");
	if (platform_assign_kernel_address_for_region(vectorTable, (addr_t)sVectorTable) != B_OK)
		panic("Failed to assign vector table address");

	memset(vectorTable, 0, B_PAGE_SIZE);
}


uint32_t
arch_mmu_generate_post_efi_page_tables(size_t memoryMapSize,
	efi_memory_descriptor *memoryMap, size_t descriptorSize,
	uint32_t descriptorVersion)
{
	arch_mmu_allocate_page_tables();
	arch_mmu_allocate_vector_table();

	build_physical_memory_list(memoryMapSize, memoryMap,
		descriptorSize, descriptorVersion,
		PHYSICAL_MEMORY_LOW, PHYSICAL_MEMORY_HIGH);

	addr_t memoryMapAddr = (addr_t)memoryMap;
	for (size_t i = 0; i < memoryMapSize / descriptorSize; ++i) {
		efi_memory_descriptor* entry =
			(efi_memory_descriptor *)(memoryMapAddr + i * descriptorSize);
		if ((entry->Attribute & EFI_MEMORY_RUNTIME) != 0) {
			map_range_to_new_area(entry,
				ARM_MMU_L2_FLAG_B | ARM_MMU_L2_FLAG_C | ARM_MMU_L2_FLAG_HAIKU_KERNEL_RW);
		}
	}

	void* cookie = NULL;
	addr_t vaddr;
	phys_addr_t paddr;
	size_t size;
	while (mmu_next_region(&cookie, &vaddr, &paddr, &size)) {
		map_range(vaddr, paddr, size,
			ARM_MMU_L2_FLAG_B | ARM_MMU_L2_FLAG_C | ARM_MMU_L2_FLAG_HAIKU_KERNEL_RW);
	}

	map_range_to_new_area(gKernelArgs.arch_args.uart.regs,
		ARM_MMU_L2_FLAG_B | ARM_MMU_L2_FLAG_HAIKU_KERNEL_RW | ARM_MMU_L2_FLAG_XN);

	sort_address_ranges(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges);

	addr_t virtPageDirectory;
	platform_bootloader_address_to_kernel_address((void*)sPageDirectory, &virtPageDirectory);

	gKernelArgs.arch_args.phys_pgdir = (uint32)sPageDirectory;
	gKernelArgs.arch_args.vir_pgdir = (uint32)virtPageDirectory;
	gKernelArgs.arch_args.next_pagetable = (uint32)(sNextPageTable) - (uint32)sPageDirectory;
	gKernelArgs.arch_args.last_pagetable = (uint32)(sLastPageTable) - (uint32)sPageDirectory;

	TRACE("gKernelArgs.arch_args.phys_pgdir     = 0x%08x\n",
		(uint32_t)gKernelArgs.arch_args.phys_pgdir);
	TRACE("gKernelArgs.arch_args.vir_pgdir      = 0x%08x\n",
		(uint32_t)gKernelArgs.arch_args.vir_pgdir);
	TRACE("gKernelArgs.arch_args.next_pagetable = 0x%08x\n",
		(uint32_t)gKernelArgs.arch_args.next_pagetable);
	TRACE("gKernelArgs.arch_args.last_pagetable = 0x%08x\n",
		(uint32_t)gKernelArgs.arch_args.last_pagetable);

	if (kTracePageDirectory)
		dump_page_dir();

	return (uint32_t)sPageDirectory;
}


void
arch_mmu_init()
{
	// empty
}
