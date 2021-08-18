/*
 * Copyright 2019-2020 Haiku, Inc. All rights reserved.
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

#include "mmu.h"
#include "efi_platform.h"


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
			case pteValid:    dprintf("valid"); break;
			case pteRead:     dprintf("read"); break;
			case pteWrite:    dprintf("write"); break;
			case pteExec:     dprintf("exec"); break;
			case pteUser:     dprintf("user"); break;
			case pteGlobal:   dprintf("global"); break;
			case pteAccessed: dprintf("accessed"); break;
			case pteDirty:    dprintf("dirty"); break;
			default:          dprintf("%" B_PRIu32, i);
			}
		}
	}
	dprintf("}");
}


static void
DumpPageWrite(uint64_t virtAdr, uint64_t physAdr, size_t size, uint64 flags, uint64& firstVirt, uint64& firstPhys, uint64& firstFlags, uint64& len)
{
	if (virtAdr == firstVirt + len && physAdr == firstPhys + len && flags == firstFlags) {
		len += size;
	} else {
		if (len != 0) {
			dprintf("  0x%08" B_PRIxADDR " - 0x%08" B_PRIxADDR,
				firstVirt, firstVirt + (len - 1));
			dprintf(": 0x%08" B_PRIxADDR " - 0x%08" B_PRIxADDR ", %#" B_PRIxADDR ", ", firstPhys, firstPhys + (len - 1), len);
			WritePteFlags(firstFlags); dprintf("\n");
		}
		firstVirt = virtAdr;
		firstPhys = physAdr;
		firstFlags = flags;
		len = size;
	}
}


static void
DumpPageTableInt(Pte* pte, uint64_t virtAdr, uint32_t level, uint64& firstVirt, uint64& firstPhys, uint64& firstFlags, uint64& len)
{
	for (uint32 i = 0; i < pteCount; i++) {
		if (((1 << pteValid) & pte[i].flags) != 0) {
			if ((((1 << pteRead) | (1 << pteWrite) | (1 << pteExec)) & pte[i].flags) == 0) {
				if (level == 0)
					panic("internal page table on level 0");

				DumpPageTableInt((Pte*)VirtFromPhys(pageSize*pte[i].ppn),
					virtAdr + ((uint64_t)i << (pageBits + pteIdxBits*level)),
					level - 1, firstVirt, firstPhys, firstFlags, len);
			} else {
				DumpPageWrite(
					SignExtendVirtAdr(virtAdr + ((uint64_t)i << (pageBits + pteIdxBits*level))),
					pte[i].ppn * B_PAGE_SIZE,
					1 << (pageBits + pteIdxBits*level),
					pte[i].flags,
					firstVirt, firstPhys, firstFlags, len);
			}
		}
	}
}


static int
DumpPageTable(uint64 satp)
{
	SatpReg satpReg(satp);
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
		if (((1 << pteValid) & pte->flags) == 0) {
			if (!alloc)
				return NULL;
			pte->ppn = mmu_allocate_page() / B_PAGE_SIZE;
			if (pte->ppn == 0)
				return NULL;
			memset((Pte*)VirtFromPhys(B_PAGE_SIZE * pte->ppn), 0, B_PAGE_SIZE);
			pte->flags |= (1 << pteValid);
		}
		pte = (Pte*)VirtFromPhys(B_PAGE_SIZE * pte->ppn);
	}
	pte += VirtAdrPte(virtAdr, 0);
	return pte;
}


static void
Map(addr_t virtAdr, phys_addr_t physAdr, uint64 flags)
{
	// dprintf("Map(%#" B_PRIxADDR ", %#" B_PRIxADDR ")\n", virtAdr, physAdr);
	Pte* pte = LookupPte(virtAdr, true);
	if (pte == NULL) panic("can't allocate page table");

	pte->ppn = physAdr / B_PAGE_SIZE;
	pte->flags = (1 << pteValid) | (1 << pteAccessed) | (1 << pteDirty) | flags;
}


static void
MapRange(addr_t virtAdr, phys_addr_t physAdr, size_t size, uint64 flags)
{
	dprintf("MapRange(%#" B_PRIxADDR " - %#" B_PRIxADDR ", %#" B_PRIxADDR " - %#" B_PRIxADDR ", %#"
		B_PRIxADDR ")\n", virtAdr, virtAdr + (size - 1), physAdr, physAdr + (size - 1), size);
	for (size_t i = 0; i < size; i += B_PAGE_SIZE)
		Map(virtAdr + i, physAdr + i, flags);

	ASSERT_ALWAYS(insert_virtual_allocated_range(virtAdr, size) >= B_OK);
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

	if (gKernelArgs.arch_args.num_virtual_ranges_to_keep
		>= MAX_VIRTUAL_RANGES_TO_KEEP)
		panic("too many virtual ranges to keep");

	gKernelArgs.arch_args.virtual_ranges_to_keep[
		gKernelArgs.arch_args.num_virtual_ranges_to_keep++] = range;
}


static void
PreallocKernelRange()
{
	Pte* root = (Pte*)VirtFromPhys(sPageTable);
	for (uint64 i = VirtAdrPte(KERNEL_BASE, 2); i <= VirtAdrPte(KERNEL_TOP, 2);
		i++) {
		Pte *pte = &root[i];
		pte->ppn = mmu_allocate_page() / B_PAGE_SIZE;
		if (pte->ppn == 0) panic("can't alloc early physical page");
		memset(VirtFromPhys(B_PAGE_SIZE * pte->ppn), 0, B_PAGE_SIZE);
		pte->flags |= (1 << pteValid);
	}
}


uint64
GetSatp()
{
	SatpReg satp;
	satp.ppn = sPageTable / B_PAGE_SIZE;
	satp.asid = 0;
	satp.mode = satpModeSv39;
	return satp.val;
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


static void
FillPhysicalMemoryMap(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	// Add physical memory to the kernel args and update virtual addresses for
	// EFI regions.
	gKernelArgs.num_physical_memory_ranges = 0;

	// First scan: Add all usable ranges
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = &memory_map[i];
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory: {
			// Usable memory.
			uint64_t base = entry->PhysicalStart;
			uint64_t end = entry->PhysicalStart + entry->NumberOfPages * 4096;
			uint64_t originalSize = end - base;

			// PMP protected memory, unusable
			if (base == 0x80000000)
				break;

			gKernelArgs.ignored_physical_memory
				+= originalSize - (std::max(end, base) - base);

			if (base >= end)
				break;
			uint64_t size = end - base;

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
		}
	}

	uint64_t initialPhysicalMemory = total_physical_memory();

	// Second scan: Remove everything reserved that may overlap
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = &memory_map[i];
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
		case EfiBootServicesCode:
		case EfiBootServicesData:
		case EfiConventionalMemory:
			break;
		default:
			uint64_t base = entry->PhysicalStart;
			uint64_t end = entry->PhysicalStart + entry->NumberOfPages * 4096;
			remove_physical_memory_range(base, end - base);
		}
	}

	gKernelArgs.ignored_physical_memory
		+= initialPhysicalMemory - total_physical_memory();

	sort_address_ranges(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges);
}


static void
FillPhysicalAllocatedMemoryMap(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = &memory_map[i];
		switch (entry->Type) {
		case EfiLoaderData:
			insert_physical_allocated_range(entry->PhysicalStart, entry->NumberOfPages * B_PAGE_SIZE);
			break;
		default:
			;
		}
	}
	sort_address_ranges(gKernelArgs.physical_allocated_range,
		gKernelArgs.num_physical_allocated_ranges);
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
	FillPhysicalAllocatedMemoryMap(memory_map_size, memory_map, descriptor_size, descriptor_version);

	// Switch EFI to virtual mode, using the kernel pmap.
	// Something involving ConvertPointer might need to be done after this?
	// http://wiki.phoenix.com/wiki/index.php/EFI_RUNTIME_SERVICES
	kRuntimeServices->SetVirtualAddressMap(memory_map_size, descriptor_size,
		descriptor_version, memory_map);
}


uint64
arch_mmu_generate_post_efi_page_tables(size_t memory_map_size,
	efi_memory_descriptor *memory_map, size_t descriptor_size,
	uint32_t descriptor_version)
{
	sPageTable = mmu_allocate_page();
	memset(VirtFromPhys(sPageTable), 0, B_PAGE_SIZE);
	dprintf("sPageTable: %#" B_PRIxADDR "\n", sPageTable);

	PreallocKernelRange();

	gKernelArgs.num_virtual_allocated_ranges = 0;
	gKernelArgs.arch_args.num_virtual_ranges_to_keep = 0;
	FillPhysicalMemoryMap(memory_map_size, memory_map, descriptor_size, descriptor_version);

	addr_range physMemRange;
	GetPhysMemRange(physMemRange);
	dprintf("physMemRange: %#" B_PRIxADDR ", %#" B_PRIxSIZE "\n", physMemRange.start, physMemRange.size);

	// Physical memory mapping
	gKernelArgs.arch_args.physMap.start = KERNEL_TOP + 1 - physMemRange.size;
	gKernelArgs.arch_args.physMap.size = physMemRange.size;
	MapRange(gKernelArgs.arch_args.physMap.start, physMemRange.start, physMemRange.size, (1 << pteRead) | (1 << pteWrite));

	// Boot loader
	dprintf("Boot loader:\n");
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = &memory_map[i];
		switch (entry->Type) {
		case EfiLoaderCode:
		case EfiLoaderData:
			MapRange(entry->VirtualStart, entry->PhysicalStart, entry->NumberOfPages * B_PAGE_SIZE, (1 << pteRead) | (1 << pteWrite) | (1 << pteExec));
			break;
		default:
			;
		}
	}
	dprintf("Boot loader stack\n");
	addr_t sp = Sp();
	addr_t stackTop = ROUNDDOWN(sp - 1024*64, B_PAGE_SIZE);
	dprintf("  SP: %#" B_PRIxADDR "\n", sp);

	// EFI runtime services
	dprintf("EFI runtime services:\n");
	for (size_t i = 0; i < memory_map_size / descriptor_size; ++i) {
		efi_memory_descriptor* entry = &memory_map[i];
		if ((entry->Attribute & EFI_MEMORY_RUNTIME) != 0)
			MapRange(entry->VirtualStart, entry->PhysicalStart, entry->NumberOfPages * B_PAGE_SIZE, (1 << pteRead) | (1 << pteWrite) | (1 << pteExec));
	}

	// Memory regions
	dprintf("Regions:\n");
	void* cookie = NULL;
	addr_t virtAdr;
	phys_addr_t physAdr;
	size_t size;
	while (mmu_next_region(&cookie, &virtAdr, &physAdr, &size)) {
		MapRange(virtAdr, physAdr, size, (1 << pteRead) | (1 << pteWrite) | (1 << pteExec));
	}

	// Devices
	dprintf("Devices:\n");
	MapAddrRange(gKernelArgs.arch_args.clint, (1 << pteRead) | (1 << pteWrite));
	MapAddrRange(gKernelArgs.arch_args.htif, (1 << pteRead) | (1 << pteWrite));
	MapAddrRange(gKernelArgs.arch_args.plic, (1 << pteRead) | (1 << pteWrite));

	if (strcmp(gKernelArgs.arch_args.uart.kind, "") != 0) {
		MapRange(gKernelArgs.arch_args.uart.regs.start,
			gKernelArgs.arch_args.uart.regs.start,
			gKernelArgs.arch_args.uart.regs.size,
			(1 << pteRead) | (1 << pteWrite));
		MapAddrRange(gKernelArgs.arch_args.uart.regs,
			(1 << pteRead) | (1 << pteWrite));
	}

	sort_address_ranges(gKernelArgs.virtual_allocated_range, gKernelArgs.num_virtual_allocated_ranges);

	DumpPageTable(GetSatp());

	return GetSatp();
}
