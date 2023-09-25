/*
 * Copyright 2019-2022 Haiku, Inc. All rights reserved.
 * Released under the terms of the MIT License.
 */


#include <algorithm>

#include <kernel.h>
#include <arch_kernel.h>
#include <boot/platform.h>
#include <boot/stage2.h>
#include <efi/types.h>
#include <efi/boot-services.h>
#include <string.h>

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

// Ignore memory above 512GB
#define PHYSICAL_MEMORY_LOW		0x00000000
#define PHYSICAL_MEMORY_HIGH	0x8000000000ull

#define RESERVED_MEMORY_BASE	0x80000000

phys_addr_t sPageTable = 0;


static inline
void *VirtFromPhys(uint64_t physAdr)
{
	return (void*)physAdr;
}


static uint64_t
SignExtendVirtAdr(uint64_t virtAdr)
{
	if (((uint64_t)1 << 38) & virtAdr)
		return virtAdr | 0xFFFFFF8000000000;
	return virtAdr;
}


static void
WritePteFlags(uint32 flags)
{
	bool first = true;
	dprintf("{");
	for (uint32 i = 0; i < 32; i++) {
		if ((1 << i) & flags) {
			if (first) first = false; else dprintf(", ");
			switch (i) {
			case 0:  dprintf("valid"); break;
			case 1:  dprintf("read"); break;
			case 2:  dprintf("write"); break;
			case 3:  dprintf("exec"); break;
			case 4:  dprintf("user"); break;
			case 5:  dprintf("global"); break;
			case 6:  dprintf("accessed"); break;
			case 7:  dprintf("dirty"); break;
			default: dprintf("%" B_PRIu32, i);
			}
		}
	}
	dprintf("}");
}


static void
DumpPageWrite(uint64_t virtAdr, uint64_t physAdr, size_t size, uint64 flags, uint64& firstVirt,
	uint64& firstPhys, uint64& firstFlags, uint64& len)
{
	if (virtAdr == firstVirt + len && physAdr == firstPhys + len && flags == firstFlags) {
		len += size;
	} else {
		if (len != 0) {
			dprintf("  0x%08" B_PRIxADDR " - 0x%08" B_PRIxADDR,
				firstVirt, firstVirt + (len - 1));
			dprintf(": 0x%08" B_PRIxADDR " - 0x%08" B_PRIxADDR ", %#" B_PRIxADDR ", ",
				firstPhys, firstPhys + (len - 1), len);
			WritePteFlags(firstFlags); dprintf("\n");
		}
		firstVirt = virtAdr;
		firstPhys = physAdr;
		firstFlags = flags;
		len = size;
	}
}


static void
DumpPageTableInt(Pte* pte, uint64_t virtAdr, uint32_t level, uint64& firstVirt, uint64& firstPhys,
	uint64& firstFlags, uint64& len)
{
	for (uint32 i = 0; i < pteCount; i++) {
		if (pte[i].isValid) {
			if (!pte[i].isRead && !pte[i].isWrite && !pte[i].isExec) {
				if (level == 0)
					panic("internal page table on level 0");

				DumpPageTableInt((Pte*)VirtFromPhys(B_PAGE_SIZE*pte[i].ppn),
					virtAdr + ((uint64_t)i << (pageBits + pteIdxBits*level)),
					level - 1, firstVirt, firstPhys, firstFlags, len);
			} else {
				DumpPageWrite(
					SignExtendVirtAdr(virtAdr + ((uint64_t)i << (pageBits + pteIdxBits*level))),
					pte[i].ppn * B_PAGE_SIZE,
					1 << (pageBits + pteIdxBits*level),
					pte[i].val & 0xff,
					firstVirt, firstPhys, firstFlags, len);
			}
		}
	}
}


static int
DumpPageTable(uint64 satp)
{
	SatpReg satpReg{.val = satp};
	Pte* root = (Pte*)VirtFromPhys(satpReg.ppn * B_PAGE_SIZE);

	dprintf("PageTable:\n");
	uint64 firstVirt = 0;
	uint64 firstPhys = 0;
	uint64 firstFlags = 0;
	uint64 len = 0;
	DumpPageTableInt(root, 0, 2, firstVirt, firstPhys, firstFlags, len);
	DumpPageWrite(0, 0, 0, 0, firstVirt, firstPhys, firstFlags, len);

	return 0;
}


static Pte*
LookupPte(addr_t virtAdr, bool alloc)
{
	Pte *pte = (Pte*)VirtFromPhys(sPageTable);
	for (int level = 2; level > 0; level --) {
		pte += VirtAdrPte(virtAdr, level);
		if (!pte->isValid) {
			if (!alloc)
				return NULL;
			uint64 ppn = mmu_allocate_page() / B_PAGE_SIZE;
			if (ppn == 0)
				return NULL;
			memset((Pte*)VirtFromPhys(B_PAGE_SIZE * ppn), 0, B_PAGE_SIZE);
			Pte newPte {
				.isValid = true,
				.isGlobal = IS_KERNEL_ADDRESS(virtAdr),
				.ppn = ppn
			};
			pte->val = newPte.val;
		}
		pte = (Pte*)VirtFromPhys(B_PAGE_SIZE * pte->ppn);
	}
	pte += VirtAdrPte(virtAdr, 0);
	return pte;
}


static void
Map(addr_t virtAdr, phys_addr_t physAdr, uint64 flags)
{
	// TRACE("Map(%#" B_PRIxADDR ", %#" B_PRIxADDR ")\n", virtAdr, physAdr);
	Pte* pte = LookupPte(virtAdr, true);
	if (pte == NULL) panic("can't allocate page table");

	Pte newPte {
		.isValid = true,
		.isGlobal = IS_KERNEL_ADDRESS(virtAdr),
		.isAccessed = true,
		.isDirty = true,
	};
	newPte.val |= flags;

	pte->val = newPte.val;
}


static void
MapRange(addr_t virtAdr, phys_addr_t physAdr, size_t size, uint64 flags)
{
	TRACE("MapRange(%#" B_PRIxADDR " - %#" B_PRIxADDR ", %#" B_PRIxADDR " - %#" B_PRIxADDR ", %#"
		B_PRIxADDR ")\n", virtAdr, virtAdr + (size - 1), physAdr, physAdr + (size - 1), size);
	for (size_t i = 0; i < size; i += B_PAGE_SIZE)
		Map(virtAdr + i, physAdr + i, flags);

	ASSERT_ALWAYS(insert_virtual_allocated_range(virtAdr, size) >= B_OK);
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


static void
MapAddrRange(addr_range& range, uint64 flags)
{
	if (range.size == 0) {
		range.start = 0;
		return;
	}

	phys_addr_t physAdr = range.start;
	range.start = get_next_virtual_address(range.size);

	MapRange(range.start, physAdr, range.size, flags);
	insert_virtual_range_to_keep(range.start, range.size);
}


static void
PreallocKernelRange()
{
	Pte* root = (Pte*)VirtFromPhys(sPageTable);
	for (uint64 i = VirtAdrPte(KERNEL_BASE, 2); i <= VirtAdrPte(KERNEL_TOP, 2);
		i++) {
		Pte* pte = &root[i];
		uint64 ppn = mmu_allocate_page() / B_PAGE_SIZE;
		if (ppn == 0) panic("can't alloc early physical page");
		memset(VirtFromPhys(B_PAGE_SIZE * pte->ppn), 0, B_PAGE_SIZE);
		Pte newPte {
			.isValid = true,
			.isGlobal = true,
			.ppn = ppn
		};
		pte->val = newPte.val;
	}
}


static uint64
GetSatp()
{
	return SatpReg{
		.ppn = sPageTable / B_PAGE_SIZE,
		.asid = 0,
		.mode = satpModeSv39
	}.val;
}


static void
GetPhysMemRange(addr_range& range)
{
	phys_addr_t beg = (phys_addr_t)(-1), end = 0;
	if (gKernelArgs.num_physical_memory_ranges <= 0)
		beg = 0;
	else {
		for (size_t i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
			beg = std::min(beg, gKernelArgs.physical_memory_range[i].start);
			end = std::max(end, gKernelArgs.physical_memory_range[i].start + gKernelArgs.physical_memory_range[i].size);
		}
	}
	range.start = beg;
	range.size = end - beg;
}


//#pragma mark -


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
	kRuntimeServices->SetVirtualAddressMap(memory_map_size, descriptor_size,
		descriptor_version, memory_map);

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

	dprintf("virt memory ranges to keep:\n");
	for (uint32_t i = 0; i < gKernelArgs.arch_args.num_virtual_ranges_to_keep; i++) {
		uint64 start = gKernelArgs.arch_args.virtual_ranges_to_keep[i].start;
		uint64 size = gKernelArgs.arch_args.virtual_ranges_to_keep[i].size;
		dprintf("    0x%08" B_PRIx64 "-0x%08" B_PRIx64 ", length 0x%08" B_PRIx64 "\n",
			start, start + size, size);
	}
#endif
}


static void
fix_memory_map_for_m_mode(size_t memoryMapSize, efi_memory_descriptor* memoryMap,
	size_t descriptorSize, uint32_t descriptorVersion)
{
	addr_t addr = (addr_t)memoryMap;

	for (size_t i = 0; i < memoryMapSize / descriptorSize; ++i) {
		efi_memory_descriptor* entry = (efi_memory_descriptor *)(addr + i * descriptorSize);
		if (entry->PhysicalStart == RESERVED_MEMORY_BASE) {
			entry->Type = EfiReservedMemoryType;
		}
	}
}


uint64
arch_mmu_generate_post_efi_page_tables(size_t memoryMapSize, efi_memory_descriptor* memoryMap,
	size_t descriptorSize, uint32_t descriptorVersion)
{
	sPageTable = mmu_allocate_page();
	memset(VirtFromPhys(sPageTable), 0, B_PAGE_SIZE);
	TRACE("sPageTable: %#" B_PRIxADDR "\n", sPageTable);

	PreallocKernelRange();

	gKernelArgs.num_virtual_allocated_ranges = 0;
	gKernelArgs.arch_args.num_virtual_ranges_to_keep = 0;

	fix_memory_map_for_m_mode(memoryMapSize, memoryMap, descriptorSize, descriptorVersion);

	build_physical_memory_list(memoryMapSize, memoryMap, descriptorSize, descriptorVersion,
		PHYSICAL_MEMORY_LOW, PHYSICAL_MEMORY_HIGH);

	addr_range physMemRange;
	GetPhysMemRange(physMemRange);
	TRACE("physMemRange: %#" B_PRIxADDR ", %#" B_PRIxSIZE "\n",
		physMemRange.start, physMemRange.size);

	// Physical memory mapping
	gKernelArgs.arch_args.physMap.start = KERNEL_TOP + 1 - physMemRange.size;
	gKernelArgs.arch_args.physMap.size = physMemRange.size;
	MapRange(gKernelArgs.arch_args.physMap.start, physMemRange.start, physMemRange.size,
		Pte {.isRead = true, .isWrite = true}.val);

	// Boot loader
	TRACE("Boot loader:\n");
	for (size_t i = 0; i < memoryMapSize / descriptorSize; ++i) {
		efi_memory_descriptor* entry = &memoryMap[i];
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
			MapRange(entry->VirtualStart, entry->PhysicalStart, entry->NumberOfPages * B_PAGE_SIZE,
				Pte {.isRead = true, .isWrite = true, .isExec = true}.val);
			break;
		default:
			;
		}
	}
	TRACE("Boot loader stack\n");
	addr_t sp = Sp();
	addr_t stackTop = ROUNDDOWN(sp - 1024*64, B_PAGE_SIZE);
	TRACE("  SP: %#" B_PRIxADDR "\n", sp);

	// EFI runtime services
	TRACE("EFI runtime services:\n");
	for (size_t i = 0; i < memoryMapSize / descriptorSize; ++i) {
		efi_memory_descriptor* entry = &memoryMap[i];
		if ((entry->Attribute & EFI_MEMORY_RUNTIME) != 0)
			MapRange(entry->VirtualStart, entry->PhysicalStart, entry->NumberOfPages * B_PAGE_SIZE,
				Pte {.isRead = true, .isWrite = true, .isExec = true}.val);
	}

	// Memory regions
	TRACE("Regions:\n");
	void* cookie = NULL;
	addr_t virtAdr;
	phys_addr_t physAdr;
	size_t size;
	while (mmu_next_region(&cookie, &virtAdr, &physAdr, &size)) {
		MapRange(virtAdr, physAdr, size, Pte {.isRead = true, .isWrite = true, .isExec = true}.val);
	}

	// Devices
	TRACE("Devices:\n");
	MapAddrRange(gKernelArgs.arch_args.clint, Pte {.isRead = true, .isWrite = true}.val);
	MapAddrRange(gKernelArgs.arch_args.htif, Pte {.isRead = true, .isWrite = true}.val);
	MapAddrRange(gKernelArgs.arch_args.plic, Pte {.isRead = true, .isWrite = true}.val);

	if (strcmp(gKernelArgs.arch_args.uart.kind, "") != 0) {
		MapRange(gKernelArgs.arch_args.uart.regs.start,
			gKernelArgs.arch_args.uart.regs.start,
			gKernelArgs.arch_args.uart.regs.size,
			Pte {.isRead = true, .isWrite = true}.val);
		MapAddrRange(gKernelArgs.arch_args.uart.regs,
			Pte {.isRead = true, .isWrite = true}.val);
	}

	sort_address_ranges(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges);

	DumpPageTable(GetSatp());

	return GetSatp();
}
