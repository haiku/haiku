/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Based on code written by Travis Geiselbrecht for NewOS.
 *
 * Distributed under the terms of the MIT License.
 */


#include "mmu.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <kernel.h>
#include <AutoDeleter.h>

#include <OS.h>

#include <string.h>


struct MemoryRegion
{
	MemoryRegion* next;
	addr_t virtAdr;
	phys_addr_t physAdr;
	size_t size;
	uint32 protection;
};


extern uint8 gStackEnd;

uint8* gMemBase = NULL;
size_t gTotalMem = 0;
uint8* gFreeMem = &gStackEnd;
addr_t gFreeVirtMem = KERNEL_LOAD_BASE;

MemoryRegion* sRegions = NULL;

ssize_t gVirtFromPhysOffset = 0;
phys_addr_t sPageTable = 0;


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


static phys_addr_t
AllocPhysPages(size_t size)
{
	size = ROUNDUP(size, B_PAGE_SIZE);
	phys_addr_t adr = ROUNDUP((addr_t)gFreeMem, B_PAGE_SIZE);

	if (adr + size - (addr_t)gMemBase > gTotalMem)
		return 0;

	gFreeMem = (uint8*)(adr + size);

	return adr;
}


static phys_addr_t
AllocPhysPage()
{
	return AllocPhysPages(B_PAGE_SIZE);
}


static void
FreePhysPages(phys_addr_t physAdr, size_t size)
{
	if (physAdr + size == (phys_addr_t)gFreeMem)
		gFreeMem -= size;
}


static phys_addr_t
AllocVirtPages(size_t size)
{
	size = ROUNDUP(size, B_PAGE_SIZE);
	phys_addr_t adr = ROUNDUP(gFreeVirtMem, B_PAGE_SIZE);
	gFreeVirtMem = adr + size;

	return adr;
}


static void
FreeVirtPages(addr_t virtAdr, size_t size)
{
	if (virtAdr + size == gFreeVirtMem)
		gFreeVirtMem -= size;
}


static inline void*
VirtFromPhys(phys_addr_t physAdr)
{
	return (void*)physAdr;
}


static inline phys_addr_t
PhysFromVirt(void* virtAdr)
{
	return (phys_addr_t)virtAdr;
}


static Pte*
LookupPte(addr_t virtAdr, bool alloc)
{
	Pte *pte = (Pte*)VirtFromPhys(sPageTable);
	for (int level = 2; level > 0; level--) {
		pte += VirtAdrPte(virtAdr, level);
		if (!pte->isValid) {
			if (!alloc)
				return NULL;
			uint64 ppn = AllocPhysPage() / B_PAGE_SIZE;
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
	// dprintf("Map(0x%" B_PRIxADDR ", 0x%" B_PRIxADDR ")\n", virtAdr, physAdr);
	Pte* pte = LookupPte(virtAdr, true);
	if (pte == NULL)
		panic("can't allocate page table");

	Pte newPte {
		.isValid = true,
		.isGlobal = IS_KERNEL_ADDRESS(virtAdr),
		.isAccessed = true,
		.isDirty = true,
		.ppn = physAdr / B_PAGE_SIZE
	};
	newPte.val |= flags;

	pte->val = newPte.val;
}


static void
MapRange(addr_t virtAdr, phys_addr_t physAdr, size_t size, uint64 flags)
{
	dprintf("MapRange(0x%" B_PRIxADDR ", 0x%" B_PRIxADDR ", 0x%"
		B_PRIxADDR ", ", virtAdr, physAdr, size);
	WritePteFlags(flags);
	dprintf(")\n");
	for (size_t i = 0; i < size; i += B_PAGE_SIZE)
		Map(virtAdr + i, physAdr + i, flags);

	ASSERT_ALWAYS(insert_virtual_allocated_range(virtAdr, size) >= B_OK);
}


static void
MapRangeIdentity(addr_t adr, size_t size, uint64 flags)
{
	MapRange(adr, adr, size, flags);
}


static void
MapAddrRange(addr_range& range, uint64 flags)
{
	phys_addr_t physAdr = range.start;
	range.start = AllocVirtPages(range.size);

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
	Pte *root = (Pte*)VirtFromPhys(sPageTable);
	for (uint64 i = VirtAdrPte(KERNEL_BASE, 2); i <= VirtAdrPte(KERNEL_TOP, 2);
		i++) {
		Pte* pte = &root[i];
		uint64 ppn = AllocPhysPage() / B_PAGE_SIZE;
		if (ppn == 0) panic("can't alloc early physical page");
		memset(VirtFromPhys(B_PAGE_SIZE * ppn), 0, B_PAGE_SIZE);
		Pte newPte {
			.isValid = true,
			.isGlobal = true,
			.ppn = ppn
		};
		pte->val = newPte.val;
	}
}


static void
SetupPageTable()
{
	sPageTable = AllocPhysPage();
	memset(VirtFromPhys(sPageTable), 0, B_PAGE_SIZE);

	PreallocKernelRange();

	// Physical memory mapping
	gKernelArgs.arch_args.physMap.size
		= gKernelArgs.physical_memory_range[0].size;
	gKernelArgs.arch_args.physMap.start = KERNEL_TOP + 1
		- gKernelArgs.arch_args.physMap.size;
	MapRange(gKernelArgs.arch_args.physMap.start,
		gKernelArgs.physical_memory_range[0].start,
		gKernelArgs.arch_args.physMap.size,
		Pte {.isRead = true, .isWrite = true}.val);

	// Boot loader
	MapRangeIdentity((addr_t)gMemBase, &gStackEnd - gMemBase,
		Pte {.isRead = true, .isWrite = true, .isExec = true}.val);

	// Memory regions
	MemoryRegion* region;
	for (region = sRegions; region != NULL; region = region->next) {
		Pte flags {
			.isRead  = (region->protection & B_READ_AREA)    != 0,
			.isWrite = (region->protection & B_WRITE_AREA)   != 0,
			.isExec  = (region->protection & B_EXECUTE_AREA) != 0
		};
		MapRange(region->virtAdr, region->physAdr, region->size, flags.val);
	}

	// Devices
	MapAddrRange(gKernelArgs.arch_args.clint, Pte {.isRead = true, .isWrite = true}.val);
	MapAddrRange(gKernelArgs.arch_args.htif, Pte {.isRead = true, .isWrite = true}.val);
	MapAddrRange(gKernelArgs.arch_args.plic, Pte {.isRead = true, .isWrite = true}.val);
	if (strcmp(gKernelArgs.arch_args.uart.kind, "") != 0) {
		MapAddrRange(gKernelArgs.arch_args.uart.regs,
			Pte {.isRead = true, .isWrite = true}.val);
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


//	#pragma mark -

extern "C" status_t
platform_allocate_region(void** address, size_t size, uint8 protection)
{
	size = ROUNDUP(size, B_PAGE_SIZE);

	ObjectDeleter<MemoryRegion> region(new(std::nothrow) MemoryRegion());
	if (!region.IsSet())
		return B_NO_MEMORY;

	region->physAdr = AllocPhysPages(size);
	if (region->physAdr == 0)
		return B_NO_MEMORY;

	region->virtAdr = AllocVirtPages(size);
	region->size = size;
	region->protection = protection;

	*address = (void*)region->physAdr;

	region->next = sRegions;
	sRegions = region.Detach();

	return B_OK;
}


extern "C" status_t
platform_free_region(void* address, size_t size)
{
	MemoryRegion* prev = NULL;
	MemoryRegion* region = sRegions;
	while (region != NULL && !(region->physAdr == (phys_addr_t)address)) {
		prev = region;
		region = region->next;
	}
	if (region == NULL) {
		panic("platform_free_region: address %p is not allocated\n", address);
		return B_ERROR;
	}
	FreePhysPages(region->physAdr, region->size);
	FreeVirtPages(region->virtAdr, region->size);
	if (prev == NULL)
		sRegions = region->next;
	else
		prev->next = region->next;

	delete region;

	return B_OK;
}


ssize_t
platform_allocate_heap_region(size_t size, void **_base)
{
	addr_t heap = AllocPhysPages(size);
	if (heap == 0)
		return B_NO_MEMORY;

	*_base = (void*)heap;
	return size;
}


void
platform_free_heap_region(void *_base, size_t size)
{
}


status_t
platform_bootloader_address_to_kernel_address(void* address, addr_t* result)
{
	MemoryRegion* region = sRegions;
	while (region != NULL && !((phys_addr_t)address >= region->physAdr
		&& (phys_addr_t)address < region->physAdr + region->size))
		region = region->next;

	if (region == NULL)
		return B_ERROR;

	*result = (addr_t)address - region->physAdr + region->virtAdr;
	return B_OK;
}


status_t
platform_kernel_address_to_bootloader_address(addr_t address, void** result)
{
	MemoryRegion* region = sRegions;
	while (region != NULL && !((phys_addr_t)address >= region->virtAdr
		&& (phys_addr_t)address < region->virtAdr + region->size))
		region = region->next;

	if (region == NULL)
		return B_ERROR;

	*result = (void*)(address - region->virtAdr + region->physAdr);
	return B_OK;
}


//	#pragma mark -

void
mmu_init(void)
{
}


void
mmu_init_for_kernel(addr_t& satp)
{
	// map in a kernel stack
	void* stack_address = NULL;
	if (platform_allocate_region(&stack_address,
		KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE,
		B_READ_AREA | B_WRITE_AREA) != B_OK) {
		panic("Unabled to allocate a stack");
	}
	gKernelArgs.cpu_kstack[0].start = fix_address((addr_t)stack_address);
	gKernelArgs.cpu_kstack[0].size = KERNEL_STACK_SIZE
		+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
	dprintf("Kernel stack at %#lx\n", gKernelArgs.cpu_kstack[0].start);

	gKernelArgs.num_physical_memory_ranges = 0;
	insert_physical_memory_range((addr_t)gMemBase, gTotalMem);

	gKernelArgs.num_virtual_allocated_ranges = 0;
	gKernelArgs.arch_args.num_virtual_ranges_to_keep = 0;

	SetupPageTable();
	satp = GetSatp();
	dprintf("satp: %#" B_PRIx64 "\n", satp);

	gKernelArgs.num_physical_allocated_ranges = 0;
	insert_physical_allocated_range((addr_t)gMemBase, gFreeMem - gMemBase);

	sort_address_ranges(gKernelArgs.virtual_allocated_range, gKernelArgs.num_virtual_allocated_ranges);
}
