/*
 * Copyright 2019-2023 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */

#include <boot/platform.h>
#include <boot/stage2.h>

#include "efi_platform.h"
#include "generic_mmu.h"
#include "mmu.h"

#include "aarch64.h"
#include "arch_mmu.h"

// #define TRACE_MMU
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

ARMv8TranslationRegime::TranslationDescriptor translation4Kb48bits = {
	{L0_SHIFT, L0_ADDR_MASK, false, true, false },
	{L1_SHIFT, Ln_ADDR_MASK, true, true,  false },
	{L2_SHIFT, Ln_ADDR_MASK, true, true,  false },
	{L3_SHIFT, Ln_ADDR_MASK, false, false, true }
};


ARMv8TranslationRegime CurrentRegime(translation4Kb48bits);
/* ARM port */
static uint64_t* sPageDirectory = NULL;
// static uint64_t* sFirstPageTable = NULL;
static uint64_t* sNextPageTable = NULL;
// static uint64_t* sLastPageTable = NULL;


const char*
granule_type_str(int tg)
{
	switch (tg) {
		case TG_4KB:
			return "4KB";
		case TG_16KB:
			return "16KB";
		case TG_64KB:
			return "64KB";
		default:
			return "Invalid Granule";
	}
}


void
arch_mmu_dump_table(uint64* table, uint8 currentLevel)
{
	ARMv8TranslationTableDescriptor ttd(table);

	if (currentLevel >= CurrentRegime.MaxLevels()) {
		// This should not happen
		panic("Too many levels ...");
		return;
	}

	uint64 EntriesPerLevel = arch_mmu_entries_per_granularity(CurrentRegime.Granularity());
	for (uint i = 0 ; i < EntriesPerLevel; i++) {
		if (!ttd.IsInvalid()) {
			TRACE("Level %d, @%0lx: TTD %016lx\t", currentLevel, ttd.Location(), ttd.Value());
			if (ttd.IsTable() && currentLevel < 3) {
				TRACE("Table! Next Level:\n");
				arch_mmu_dump_table(ttd.Dereference(), currentLevel + 1);
			}
			if (ttd.IsBlock() || (ttd.IsPage() && currentLevel == 3)) {
				TRACE("Block/Page");

				if (i & 1) { // 2 entries per row
					TRACE("\n");
				} else {
					TRACE("\t");
				}
			}
		}
		ttd.Next();
	}
}


void
arch_mmu_dump_present_tables()
{
	uint64 address = arch_mmu_base_register();
	dprintf("Under TTBR0: %lx\n", address);

	arch_mmu_dump_table(reinterpret_cast<uint64*>(address), 0);

	/* We are willing to transition, but still in EL2, present MMU configuration
	 * for user is present in EL2 by TTBR0_EL2. Kernel side is not active, but
	 * allocated under sPageDirectory, defined under TTBR1_EL1.
	 */
	dprintf("Under allocated TTBR1_EL1:\n");
	arch_mmu_dump_table(sPageDirectory, 0);
}


void arch_mmu_setup_EL1(uint64 tcr) {

	// Enable TTBR1
	tcr &= ~TCR_EPD1_DISABLE;

	// Set space for kernel space
	tcr &= ~T1SZ_MASK; // Clear
	// TODO: Compiler dependency?
	tcr |= TCR_T1SZ(__builtin_popcountl(KERNEL_BASE));

	// Set the maximum PA size to the maximum supported by the hardware.
	uint64_t pa_size = READ_SPECIALREG(ID_AA64MMFR0_EL1) & ID_AA64MMFR0_PA_RANGE_MASK;

	// PA size of 4 petabytes required 64KB paging granules, which
	// we don't support, so clamp the maximum to 256 terabytes.
	if (pa_size == ID_AA64MMFR0_PA_RANGE_4P)
		pa_size = ID_AA64MMFR0_PA_RANGE_256T;
	tcr &= ~IPS_MASK;
	tcr |= pa_size << TCR_IPS_SHIFT;

	// Flush the cache so that we don't receive unexpected writebacks later.
	_arch_cache_clean_poc();

	WRITE_SPECIALREG(TCR_EL1, tcr);

	// Invalidate all TLB entries. Also ensures that all memory traffic has
	// resolved, and flushes the instruction pipeline.
	_arch_mmu_invalidate_tlb_all(arch_exception_level());
}


uint64
map_region(addr_t virt_addr, addr_t  phys_addr, size_t size,
	uint32_t level, uint64_t flags, uint64* descriptor)
{
	ARMv8TranslationTableDescriptor ttd(descriptor);

	if (level >= CurrentRegime.MaxLevels()) {
		panic("Too many levels at mapping\n");
	}

	uint64 currentLevelSize = CurrentRegime.EntrySize(level);

	ttd.JumpTo(CurrentRegime.DescriptorIndex(virt_addr, level));

	uint64 remainingSizeInTable = CurrentRegime.TableSize(level)
		- currentLevelSize * CurrentRegime.DescriptorIndex(virt_addr, level);

	TRACE("Level %x, Processing desc %lx indexing %lx\n",
		level, reinterpret_cast<uint64>(descriptor), ttd.Location());

	if (ttd.IsInvalid()) {
		// If the physical has the same alignment we could make a block here
		// instead of using a complete next level table
		if (size >= currentLevelSize && CurrentRegime.Aligned(phys_addr, level)) {
			// Set it as block or page
			if (CurrentRegime.BlocksAllowed(level)) {
				ttd.SetAsBlock(reinterpret_cast<uint64*>(phys_addr), flags);
			} else {
				// Most likely in Level 3...
				ttd.SetAsPage(reinterpret_cast<uint64*>(phys_addr), flags);
			}

			// Expand!
			int64 expandedSize = (size > remainingSizeInTable)?remainingSizeInTable:size;

			do {
				phys_addr += currentLevelSize;
				expandedSize -= currentLevelSize;
				if (expandedSize > 0) {
					ttd.Next();
					if (CurrentRegime.BlocksAllowed(level)) {
						ttd.SetAsBlock(reinterpret_cast<uint64*>(phys_addr), flags);
					} else {
						// Most likely in Level 3...
						ttd.SetAsPage(reinterpret_cast<uint64*>(phys_addr), flags);
					}
				}
			} while (expandedSize > 0);

			return (size > remainingSizeInTable)?(size - remainingSizeInTable):0;

		} else {
			// Set it to next level
			uint64 offset = 0;
			uint64 remainingSize = size;
			do {
				uint64* page = NULL;
				if (ttd.IsInvalid()) {
					// our region is too small would need to create a level below
					page = CurrentRegime.AllocatePage();
					ttd.SetToTable(page, flags);
				} else if (ttd.IsTable()) {
					// Next table is allocated, follow it
					page = ttd.Dereference();
				} else {
					panic("Required contiguous descriptor in use by Block/Page for %lx\n", ttd.Location());
				}

				uint64 unprocessedSize = map_region(virt_addr + offset,
					phys_addr + offset, remainingSize, level + 1, flags, page);

				offset = remainingSize - unprocessedSize;

				remainingSize = unprocessedSize;

				ttd.Next();

			} while (remainingSize > 0);

			return 0;
		}

	} else {

		if ((ttd.IsBlock() && CurrentRegime.BlocksAllowed(level))
			|| (ttd.IsPage() && CurrentRegime.PagesAllowed(level))
		) {
			// TODO: Review, overlap? expand?
			panic("Re-setting a Block/Page descriptor for %lx\n", ttd.Location());
			return 0;
		} else if (ttd.IsTable() && CurrentRegime.TablesAllowed(level)) {
			// Next Level
			map_region(virt_addr, phys_addr, size, level + 1, flags, ttd.Dereference());
			return 0;
		} else {
			panic("All descriptor types processed for %lx\n", ttd.Location());
			return 0;
		}
	}
}


static void
map_range(addr_t virt_addr, phys_addr_t phys_addr, size_t size, uint64_t flags)
{
	TRACE("map 0x%0lx --> 0x%0lx, len=0x%0lx, flags=0x%0lx\n",
		(uint64_t)virt_addr, (uint64_t)phys_addr, (uint64_t)size, flags);

	// TODO: Review why we get ranges with 0 size ...
	if (size == 0) {
		TRACE("Requesing 0 size map\n");
		return;
	}

	// TODO: Review this case
	if (phys_addr == READ_SPECIALREG(TTBR1_EL1)) {
		TRACE("Trying to map the TTBR itself?!\n");
		return;
	}

	if (arch_mmu_read_access(virt_addr) && arch_mmu_read_access(virt_addr + size)) {
		TRACE("Range already covered in current MMU\n");
		return;
	}

	uint64 address;

	if (arch_mmu_is_kernel_address(virt_addr)) {
		// Use TTBR1
		address = READ_SPECIALREG(TTBR1_EL1);
	} else {
		// ok, but USE instead TTBR0
		if (arch_exception_level() == 1)
			address = READ_SPECIALREG(TTBR0_EL1);
		else
			address = READ_SPECIALREG(TTBR0_EL2);
	}

	map_region(virt_addr, phys_addr, size, 0, flags, reinterpret_cast<uint64*>(address));

// 	for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
// 		map_page(virt_addr + offset, phys_addr + offset, flags);
// 	}

	ASSERT_ALWAYS(insert_virtual_allocated_range(virt_addr, size) >= B_OK);
}


void
arch_mmu_init()
{
	// Stub
}


void
arch_mmu_post_efi_setup(size_t memory_map_size,
	efi_memory_descriptor* memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	build_physical_allocated_list(memory_map_size, memory_map,
		descriptor_size, descriptor_version);

	// Switch EFI to virtual mode, using the kernel pmap.
	kRuntimeServices->SetVirtualAddressMap(memory_map_size, descriptor_size,
		descriptor_version, memory_map);

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
			uint64 start = gKernelArgs.arch_args.virtual_ranges_to_keep[i].start;
			uint64 size = gKernelArgs.arch_args.virtual_ranges_to_keep[i].size;
			dprintf("    0x%08" B_PRIx64 "-0x%08" B_PRIx64 ", length 0x%08" B_PRIx64 "\n",
				start, start + size, size);
		}
	}
}


void
arch_mmu_allocate_kernel_page_tables(void)
{
	uint64* page = NULL;
	uint64 ttbr1 = READ_SPECIALREG(TTBR1_EL1);

	// Trust possible previous allocations of TTBR1
	// only if we come from a preset EL1 context
	if (ttbr1 != 0ll) {
		if (arch_exception_level() == 1) {
			page = reinterpret_cast<uint64*>(ttbr1);
			TRACE("Reusing TTBR1_EL1 present : %" B_PRIx64 "\n", ttbr1);
		} else if (arch_exception_level() == 2) {
			TRACE("Ignoring EL1 TTBR1(%" B_PRIx64") tables\n", ttbr1);
		}
	}

	// NOTE: On devices supporting multiple translation base registers, TTBR0 must
	// be used solely.
	if (page == NULL) {
		page = CurrentRegime.AllocatePage();
		if (page != NULL) {
			WRITE_SPECIALREG(TTBR1_EL1, page);
		} else {
			panic("Not enough memory for kernel initial page\n");
		}
	}

	sPageDirectory = page;
}


uint32_t
arch_mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor* memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	addr_t memory_map_addr = (addr_t)memory_map;

	MemoryAttributeIndirection currentMair;

	arch_mmu_allocate_kernel_page_tables();

	build_physical_memory_list(memory_map_size, memory_map,
		descriptor_size, descriptor_version,
		PHYSICAL_MEMORY_LOW, PHYSICAL_MEMORY_HIGH);

	TRACE("Mapping EFI_MEMORY_RUNTIME\n");
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor*)(memory_map_addr + i * descriptor_size);
		if ((entry->Attribute & EFI_MEMORY_RUNTIME) != 0)
			map_range(entry->VirtualStart, entry->PhysicalStart,
				entry->NumberOfPages * B_PAGE_SIZE,
				ARMv8TranslationTableDescriptor::DefaultCodeAttribute | currentMair.MaskOf(MAIR_NORMAL_WB));
	}

	TRACE("Mapping \"next\" regions\n");
	void* cookie = NULL;
	addr_t vaddr;
	phys_addr_t paddr;
	size_t size;
	while (mmu_next_region(&cookie, &vaddr, &paddr, &size)) {
		map_range(vaddr, paddr, size,
			ARMv8TranslationTableDescriptor::DefaultCodeAttribute
			| currentMair.MaskOf(MAIR_NORMAL_WB));
	}

	// TODO: We actually can only map physical RAM, mapping everything
	// could cause unwanted MMIO or bus errors on real hardware.
	map_range(KERNEL_PMAP_BASE, 0, KERNEL_PMAP_SIZE - 1,
		ARMv8TranslationTableDescriptor::DefaultCodeAttribute
		| currentMair.MaskOf(MAIR_NORMAL_WB));

	if (gKernelArgs.arch_args.uart.kind[0] != 0) {
		// Map uart because we want to use it during early boot.
		uint64 regs_start = gKernelArgs.arch_args.uart.regs.start;
		uint64 regs_size = ROUNDUP(gKernelArgs.arch_args.uart.regs.size, B_PAGE_SIZE);
		uint64 base = get_next_virtual_address(regs_size);

		map_range(base, regs_start, regs_size,
			ARMv8TranslationTableDescriptor::DefaultPeripheralAttribute |
			currentMair.MaskOf(MAIR_DEVICE_nGnRnE));

		gKernelArgs.arch_args.uart.regs.start = base;
	}

	sort_address_ranges(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges);

	addr_t vir_pgdir;
	platform_bootloader_address_to_kernel_address((void*)sPageDirectory, &vir_pgdir);

	gKernelArgs.arch_args.phys_pgdir = (uint64)sPageDirectory;
	gKernelArgs.arch_args.vir_pgdir = (uint32)vir_pgdir;
	gKernelArgs.arch_args.next_pagetable = (uint64)(sNextPageTable) - (uint64)sPageDirectory;

	TRACE("gKernelArgs.arch_args.phys_pgdir     = 0x%08x\n",
		(uint32_t)gKernelArgs.arch_args.phys_pgdir);
	TRACE("gKernelArgs.arch_args.vir_pgdir      = 0x%08x\n",
		(uint32_t)gKernelArgs.arch_args.vir_pgdir);
	TRACE("gKernelArgs.arch_args.next_pagetable = 0x%08x\n",
		(uint32_t)gKernelArgs.arch_args.next_pagetable);

	if (kTracePageDirectory)
		arch_mmu_dump_present_tables();

	return (uint64_t)sPageDirectory;
}
