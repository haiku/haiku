/*
 * Copyright 2019-2021 Haiku, Inc. All rights reserved.
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

#include "mmu.h"
#include "efi_platform.h"

#define ALIGN_PAGEDIR (1024 * 16)
#define MAX_PAGE_TABLES 192
#define PAGE_TABLE_AREA_SIZE (MAX_PAGE_TABLES * ARM_MMU_L2_COARSE_TABLE_SIZE)

static uint32_t *sPageDirectory = NULL;
static uint32_t *sFirstPageTable = NULL;
static uint32_t *sNextPageTable = NULL;
static uint32_t *sLastPageTable = NULL;


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
	uint32 *page_table = sNextPageTable;
	sNextPageTable += ARM_MMU_L2_COARSE_ENTRY_COUNT;
	if (sNextPageTable >= sLastPageTable)
		panic("ran out of page tables\n");
	return page_table;
}


static void
map_page(addr_t virt_addr, phys_addr_t phys_addr, uint32_t flags)
{
	phys_addr &= ~(B_PAGE_SIZE - 1);

	uint32 *pageTable = NULL;
	uint32 pageDirectoryIndex = VADDR_TO_PDENT(virt_addr);
	uint32 pageDirectoryEntry = sPageDirectory[pageDirectoryIndex];

	if (pageDirectoryEntry == 0) {
		pageTable = get_next_page_table();
		sPageDirectory[pageDirectoryIndex] = (uint32_t)pageTable | ARM_MMU_L1_TYPE_COARSE;
	} else {
		pageTable = (uint32 *)(pageDirectoryEntry & ARM_PDE_ADDRESS_MASK);
	}

	uint32 pageTableIndex = VADDR_TO_PTENT(virt_addr);
	pageTable[pageTableIndex] = phys_addr | flags | ARM_MMU_L2_TYPE_SMALLNEW;
}


static void
map_range(addr_t virt_addr, phys_addr_t phys_addr, size_t size, uint32_t flags)
{
	//dprintf("map 0x%08x --> 0x%08x, len=0x%08x, flags=0x%08x\n",
	//	(uint32_t)virt_addr, (uint32_t)phys_addr, (uint32_t)size, flags);

	for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
		map_page(virt_addr + offset, phys_addr + offset, flags);
	}

	ASSERT_ALWAYS(insert_virtual_allocated_range(virt_addr, size) >= B_OK);
}


static void
map_range_to_new_area(addr_range& range, uint32_t flags)
{
	if (range.size == 0) {
		range.start = 0;
		return;
	}

	phys_addr_t phys_addr = range.start;
	addr_t virt_addr = get_next_virtual_address(range.size);

	map_range(virt_addr, phys_addr, range.size, flags);

	if (gKernelArgs.arch_args.num_virtual_ranges_to_keep
		>= MAX_VIRTUAL_RANGES_TO_KEEP)
		panic("too many virtual ranges to keep");

	range.start = virt_addr;

	gKernelArgs.arch_args.virtual_ranges_to_keep[
		gKernelArgs.arch_args.num_virtual_ranges_to_keep++] = range;
}


static void
build_physical_memory_list(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	addr_t addr = (addr_t)memory_map;

	gKernelArgs.num_physical_memory_ranges = 0;

	// First scan: Add all usable ranges
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor *)(addr + i * descriptor_size);
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
			entry->VirtualStart = entry->PhysicalStart;
			break;
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory: {
			// Usable memory.
			uint64_t base = entry->PhysicalStart;
			uint64_t size = entry->NumberOfPages * B_PAGE_SIZE;
			insert_physical_memory_range(base, size);
			break;
		}
		case EfiACPIReclaimMemory:
			// ACPI reclaim -- physical memory we could actually use later
			break;
		case EfiRuntimeServicesCode:
		case EfiRuntimeServicesData:
			entry->VirtualStart = entry->PhysicalStart;
			break;
		case EfiMemoryMappedIO:
			entry->VirtualStart = entry->PhysicalStart;
			break;
		}
	}

	uint64_t initialPhysicalMemory = total_physical_memory();

	// Second scan: Remove everything reserved that may overlap
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor *)(addr + i * descriptor_size);
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory:
			break;
		default:
			uint64_t base = entry->PhysicalStart;
			uint64_t size = entry->NumberOfPages * B_PAGE_SIZE;
			remove_physical_memory_range(base, size);
		}
	}

	gKernelArgs.ignored_physical_memory
		+= initialPhysicalMemory - total_physical_memory();

	sort_address_ranges(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges);
}


static void
build_physical_allocated_list(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	addr_t addr = (addr_t)memory_map;
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor *)(addr + i * descriptor_size);
		switch (entry->Type) {
		case EfiLoaderData: {
			uint64_t base = entry->PhysicalStart;
			uint64_t size = entry->NumberOfPages * B_PAGE_SIZE;
			insert_physical_allocated_range(base, size);
			break;
		}
		default:
			;
		}
	}

	sort_address_ranges(gKernelArgs.physical_allocated_range,
		gKernelArgs.num_physical_allocated_ranges);
}


void
arch_mmu_init()
{
}


void
arch_mmu_post_efi_setup(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	build_physical_allocated_list(memory_map_size, memory_map,
		descriptor_size, descriptor_version);

	// Switch EFI to virtual mode, using the kernel pmap.
	// Something involving ConvertPointer might need to be done after this?
	// http://wiki.phoenix.com/wiki/index.php/EFI_RUNTIME_SERVICES
	kRuntimeServices->SetVirtualAddressMap(memory_map_size, descriptor_size,
		descriptor_version, memory_map);

	dprintf("phys memory ranges:\n");
	for (uint32_t i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
		uint32_t start = (uint32_t)gKernelArgs.physical_memory_range[i].start;
		uint32_t size = (uint32_t)gKernelArgs.physical_memory_range[i].size;
		dprintf("    0x%08x-0x%08x, length 0x%08x\n",
			start, start + size, size);
	}

	dprintf("allocated phys memory ranges:\n");
	for (uint32_t i = 0; i < gKernelArgs.num_physical_allocated_ranges; i++) {
		uint32_t start = (uint32_t)gKernelArgs.physical_allocated_range[i].start;
		uint32_t size = (uint32_t)gKernelArgs.physical_allocated_range[i].size;
		dprintf("    0x%08x-0x%08x, length 0x%08x\n",
			start, start + size, size);
	}

	dprintf("allocated virt memory ranges:\n");
	for (uint32_t i = 0; i < gKernelArgs.num_virtual_allocated_ranges; i++) {
		uint32_t start = (uint32_t)gKernelArgs.virtual_allocated_range[i].start;
		uint32_t size = (uint32_t)gKernelArgs.virtual_allocated_range[i].size;
		dprintf("    0x%08x-0x%08x, length 0x%08x\n",
			start, start + size, size);
	}

	dprintf("virt memory ranges to keep:\n");
	for (uint32_t i = 0; i < gKernelArgs.arch_args.num_virtual_ranges_to_keep; i++) {
		uint32_t start = (uint32_t)gKernelArgs.arch_args.virtual_ranges_to_keep[i].start;
		uint32_t size = (uint32_t)gKernelArgs.arch_args.virtual_ranges_to_keep[i].size;
		dprintf("    0x%08x-0x%08x, length 0x%08x\n",
			start, start + size, size);
	}
}


static void
arch_mmu_allocate_page_tables(void)
{
	if (platform_allocate_region((void **)&sPageDirectory,
		ARM_MMU_L1_TABLE_SIZE + ALIGN_PAGEDIR + PAGE_TABLE_AREA_SIZE, 0, false) != B_OK)
		panic("Failed to allocate page directory.");
	sPageDirectory = (uint32 *)ROUNDUP((uint32)sPageDirectory, ALIGN_PAGEDIR);
	memset(sPageDirectory, 0, ARM_MMU_L1_TABLE_SIZE);

	sFirstPageTable = (uint32*)((uint32)sPageDirectory + ARM_MMU_L1_TABLE_SIZE);
	sNextPageTable = sFirstPageTable;
	sLastPageTable = (uint32*)((uint32)sFirstPageTable + PAGE_TABLE_AREA_SIZE);

	memset(sFirstPageTable, 0, PAGE_TABLE_AREA_SIZE);

	dprintf("sPageDirectory  = 0x%08x\n", (uint32)sPageDirectory);
	dprintf("sFirstPageTable = 0x%08x\n", (uint32)sFirstPageTable);
	dprintf("sLastPageTable  = 0x%08x\n", (uint32)sLastPageTable);
}

uint32_t
arch_mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	addr_t memory_map_addr = (addr_t)memory_map;

	arch_mmu_allocate_page_tables();

	build_physical_memory_list(memory_map_size, memory_map,
		descriptor_size, descriptor_version);

	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry =
			(efi_memory_descriptor *)(memory_map_addr + i * descriptor_size);
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
			map_range(entry->VirtualStart, entry->PhysicalStart,
				entry->NumberOfPages * B_PAGE_SIZE,
				ARM_MMU_L2_FLAG_B | ARM_MMU_L2_FLAG_C | ARM_MMU_L2_FLAG_AP_RW);
			break;
		default:
			;
		}
	}

	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry =
			(efi_memory_descriptor *)(memory_map_addr + i * descriptor_size);
		if ((entry->Attribute & EFI_MEMORY_RUNTIME) != 0)
			map_range(entry->VirtualStart, entry->PhysicalStart,
				entry->NumberOfPages * B_PAGE_SIZE,
				ARM_MMU_L2_FLAG_B | ARM_MMU_L2_FLAG_C | ARM_MMU_L2_FLAG_AP_RW);
	}

	void* cookie = NULL;
	addr_t vaddr;
	phys_addr_t paddr;
	size_t size;
	while (mmu_next_region(&cookie, &vaddr, &paddr, &size)) {
		map_range(vaddr, paddr, size,
			ARM_MMU_L2_FLAG_B | ARM_MMU_L2_FLAG_C | ARM_MMU_L2_FLAG_AP_RW);
	}

	map_range_to_new_area(gKernelArgs.arch_args.uart.regs, ARM_MMU_L2_FLAG_B);

	// identity mapping for page table area
	uint32_t page_table_area = (uint32_t)sFirstPageTable;
	map_range(page_table_area, page_table_area, PAGE_TABLE_AREA_SIZE,
		ARM_MMU_L2_FLAG_B | ARM_MMU_L2_FLAG_C | ARM_MMU_L2_FLAG_AP_RW);

	sort_address_ranges(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges);

	addr_t vir_pgdir;
	platform_bootloader_address_to_kernel_address((void*)sPageDirectory, &vir_pgdir);

	gKernelArgs.arch_args.phys_pgdir = (uint32)sPageDirectory;
	gKernelArgs.arch_args.vir_pgdir = (uint32)vir_pgdir;
	gKernelArgs.arch_args.next_pagetable = (uint32)(sNextPageTable) - (uint32)sPageDirectory;

	dprintf("gKernelArgs.arch_args.phys_pgdir     = 0x%08x\n",
		(uint32_t)gKernelArgs.arch_args.phys_pgdir);
	dprintf("gKernelArgs.arch_args.vir_pgdir      = 0x%08x\n",
		(uint32_t)gKernelArgs.arch_args.vir_pgdir);
	dprintf("gKernelArgs.arch_args.next_pagetable = 0x%08x\n",
		(uint32_t)gKernelArgs.arch_args.next_pagetable);

	//dump_page_dir();

	return (uint32_t)sPageDirectory;
}
