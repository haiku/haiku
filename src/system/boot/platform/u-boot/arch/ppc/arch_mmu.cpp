/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
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
#include <platform/openfirmware/openfirmware.h>
#ifdef __ARM__
#include <arm_mmu.h>
#endif
#include <kernel.h>

#include <board_config.h>

#include <OS.h>

#include <string.h>

/*! This implements boot loader mmu support for Book-E PowerPC,
	which only support a limited number of TLB and no hardware page table walk,
	and does not standardize at how to use the mmu, requiring vendor-specific
	code.

	Like Linux, we pin one of the TLB entries to a fixed translation,
	however we use it differently.
	cf. http://kernel.org/doc/ols/2003/ols2003-pages-340-350.pdf

	This translation uses a single large page (16 or 256MB are possible) which
	directly maps the begining of the RAM.
	We use it as a linear space to allocate from at boot time,
	loading the kernel and modules into it, and other required data.
	Near the end we reserve a page table (it doesn't need to be aligned),
	but unlike Linux we use the same globally hashed page table that is
	implemented by Classic PPC, to allow reusing code if possible, and also
	to limit fragmentation which would occur by using a tree-based page table.
	However this means we might actually run out of page table entries in case
	of too many collisions.

	The kernel will then create areas to cover this already-mapped space.
	This also means proper permission bits (RWX) will not be applicable to
	separate areas which are enclosed by this mapping.

	We put the kernel stack at the end of the mapping so that the guard page is
	outsite and thus unmapped. (we don't support SMP)
*/

/*!	The (physical) memory layout of the boot loader is currently as follows:
	 0x00000000			kernel
	 0x00400000			...modules

	 (at least on the Sam460ex U-Boot; we'll need to accomodate other setups)
	 0x01000000			boot loader
	 0x01800000			Flattened Device Tree
	 0x01900000			boot.tgz (= ramdisk)
	 0x02000000			boot loader uimage


					boot loader heap (should be discarded later on)
	 ... 256M-Kstack		page hash table
	 ... 256M			kernel stack
					kernel stack guard page

	The kernel is mapped at KERNEL_BASE, all other stuff mapped by the
	loader (kernel args, modules, driver settings, ...) comes after
	0x80040000 which means that there is currently only 4 MB reserved for
	the kernel itself (see kMaxKernelSize). FIXME: downsize kernel_ppc
*/


int32 of_address_cells(int package);
int32 of_size_cells(int package);

extern bool gIs440;
// XXX:use a base class for Book-E support?
extern status_t arch_mmu_setup_pinned_tlb_amcc440(phys_addr_t totalRam,
	size_t &tableSize, size_t &tlbSize);

#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define TRACE_MEMORY_MAP
	// Define this to print the memory map to serial debug.

static const size_t kMaxKernelSize = 0x400000;		// 4 MB for the kernel

static addr_t sNextPhysicalAddress = kMaxKernelSize; //will be set by mmu_init
static addr_t sNextVirtualAddress = KERNEL_BASE + kMaxKernelSize;
//static addr_t sMaxVirtualAddress = KERNEL_BASE + kMaxKernelSize;

// working page directory and page table
static void *sPageTable = 0 ;


static addr_t
get_next_virtual_address(size_t size)
{
	addr_t address = sNextVirtualAddress;
	sNextPhysicalAddress += size;
	sNextVirtualAddress += size;

	return address;
}


#if 0
static addr_t
get_next_physical_address(size_t size)
{
	addr_t address = sNextPhysicalAddress;
	sNextPhysicalAddress += size;
	sNextVirtualAddress += size;

	return address;
}
#endif


//	#pragma mark -


extern "C" addr_t
mmu_map_physical_memory(addr_t physicalAddress, size_t size, uint32 flags)
{
	panic("WRITEME");
	return 0;
}


/*!	Sets up the final and kernel accessible GDT and IDT tables.
	BIOS calls won't work any longer after this function has
	been called.
*/
extern "C" void
mmu_init_for_kernel(void)
{
	TRACE(("mmu_init_for_kernel\n"));

	// TODO: remove all U-Boot TLB

#ifdef TRACE_MEMORY_MAP
	{
		uint32 i;

		dprintf("phys memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
			dprintf("    base 0x%" B_PRIxPHYSADDR
				", length 0x%" B_PRIxPHYSADDR "\n",
				gKernelArgs.physical_memory_range[i].start,
				gKernelArgs.physical_memory_range[i].size);
		}

		dprintf("allocated phys memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_physical_allocated_ranges; i++) {
			dprintf("    base 0x%" B_PRIxPHYSADDR
				", length 0x%" B_PRIxPHYSADDR "\n",
				gKernelArgs.physical_allocated_range[i].start,
				gKernelArgs.physical_allocated_range[i].size);
		}

		dprintf("allocated virt memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_virtual_allocated_ranges; i++) {
			dprintf("    base 0x%" B_PRIxPHYSADDR
				", length 0x%" B_PRIxPHYSADDR "\n",
				gKernelArgs.virtual_allocated_range[i].start,
				gKernelArgs.virtual_allocated_range[i].size);
		}
	}
#endif
}


//TODO:move this to generic/ ?
static status_t
find_physical_memory_ranges(phys_addr_t &total)
{
	int memory = -1;
	int package;
	dprintf("checking for memory...\n");
	if (of_getprop(gChosen, "memory", &memory, sizeof(int)) == OF_FAILED)
		package = of_finddevice("/memory");
	else
		package = of_instance_to_package(memory);
	if (package == OF_FAILED)
		return B_ERROR;

	total = 0;

	// Memory base addresses are provided in 32 or 64 bit flavors
	// #address-cells and #size-cells matches the number of 32-bit 'cells'
	// representing the length of the base address and size fields
	int root = of_finddevice("/");
	int32 regAddressCells = of_address_cells(root);
	int32 regSizeCells = of_size_cells(root);
	if (regAddressCells == OF_FAILED || regSizeCells == OF_FAILED) {
		dprintf("finding base/size length counts failed, assume 32-bit.\n");
		regAddressCells = 1;
		regSizeCells = 1;
	}

	// NOTE : Size Cells of 2 is possible in theory... but I haven't seen it yet.
	if (regAddressCells > 2 || regSizeCells > 1) {
		panic("%s: Unsupported OpenFirmware cell count detected.\n"
		"Address Cells: %" B_PRId32 "; Size Cells: %" B_PRId32
		" (CPU > 64bit?).\n", __func__, regAddressCells, regSizeCells);
		return B_ERROR;
	}

	// On 64-bit PowerPC systems (G5), our mem base range address is larger
	if (regAddressCells == 2) {
		struct of_region<uint64, uint32> regions[64];
		int count = of_getprop(package, "reg", regions, sizeof(regions));
		if (count == OF_FAILED)
			count = of_getprop(memory, "reg", regions, sizeof(regions));
		if (count == OF_FAILED)
			return B_ERROR;
		count /= sizeof(regions[0]);

		for (int32 i = 0; i < count; i++) {
			if (regions[i].size <= 0) {
				dprintf("%ld: empty region\n", i);
				continue;
			}
			dprintf("%" B_PRIu32 ": base = %" B_PRIu64 ","
				"size = %" B_PRIu32 "\n", i, regions[i].base, regions[i].size);

			total += regions[i].size;

			if (insert_physical_memory_range((addr_t)regions[i].base,
					regions[i].size) != B_OK) {
				dprintf("cannot map physical memory range "
					"(num ranges = %" B_PRIu32 ")!\n",
					gKernelArgs.num_physical_memory_ranges);
				return B_ERROR;
			}
		}
		return B_OK;
	}

	// Otherwise, normal 32-bit PowerPC G3 or G4 have a smaller 32-bit one
	struct of_region<uint32, uint32> regions[64];
	int count = of_getprop(package, "reg", regions, sizeof(regions));
	if (count == OF_FAILED)
		count = of_getprop(memory, "reg", regions, sizeof(regions));
	if (count == OF_FAILED)
		return B_ERROR;
	count /= sizeof(regions[0]);

	for (int32 i = 0; i < count; i++) {
		if (regions[i].size <= 0) {
			dprintf("%ld: empty region\n", i);
			continue;
		}
		dprintf("%" B_PRIu32 ": base = %" B_PRIu32 ","
			"size = %" B_PRIu32 "\n", i, regions[i].base, regions[i].size);

		total += regions[i].size;

		if (insert_physical_memory_range((addr_t)regions[i].base,
				regions[i].size) != B_OK) {
			dprintf("cannot map physical memory range "
				"(num ranges = %" B_PRIu32 ")!\n",
				gKernelArgs.num_physical_memory_ranges);
			return B_ERROR;
		}
	}

	return B_OK;
}


extern "C" void
mmu_init(void* fdt)
{
	size_t tableSize, tlbSize;
	status_t err;
	TRACE(("mmu_init\n"));

	// get map of physical memory (fill in kernel_args structure)

	phys_addr_t total;
	if (find_physical_memory_ranges(total) != B_OK) {
		dprintf("Error: could not find physical memory ranges!\n");
		return /*B_ERROR*/;
	}
	dprintf("total physical memory = %" B_PRId64 "MB\n", total / (1024 * 1024));

	// XXX: ugly, and wrong, there are several 440 mmu types... FIXME
	if (gIs440) {
		err = arch_mmu_setup_pinned_tlb_amcc440(total, tableSize, tlbSize);
		dprintf("setup_pinned_tlb: 0x%08lx table %zdMB tlb %zdMB\n",
			err, tableSize / (1024 * 1024), tlbSize / (1024 * 1024));
	} else {
		panic("Unknown MMU type!");
		return;
	}

	// remember the start of the allocated physical pages
	gKernelArgs.physical_allocated_range[0].start
		= gKernelArgs.physical_memory_range[0].start;
	gKernelArgs.physical_allocated_range[0].size = tlbSize;
	gKernelArgs.num_physical_allocated_ranges = 1;

	// Save the memory we've virtually allocated (for the kernel and other
	// stuff)
	gKernelArgs.virtual_allocated_range[0].start = KERNEL_BASE;
	gKernelArgs.virtual_allocated_range[0].size
		= tlbSize + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
	gKernelArgs.num_virtual_allocated_ranges = 1;


	sPageTable = (void *)(tlbSize - tableSize - KERNEL_STACK_SIZE);
		// we put the page table near the end of the pinned TLB
	TRACE(("page table at 0x%p to 0x%p\n", sPageTable,
		(uint8 *)sPageTable + tableSize));

	// map in a kernel stack
	gKernelArgs.cpu_kstack[0].start = (addr_t)(tlbSize - KERNEL_STACK_SIZE);
	gKernelArgs.cpu_kstack[0].size = KERNEL_STACK_SIZE
		+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;

	TRACE(("kernel stack at 0x%Lx to 0x%Lx\n", gKernelArgs.cpu_kstack[0].start,
		gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size));

#ifdef __ARM__
	init_page_directory();

	// map the page directory on the next vpage
	gKernelArgs.arch_args.vir_pgdir = mmu_map_physical_memory(
		(addr_t)sPageDirectory, MMU_L1_TABLE_SIZE, kDefaultPageFlags);
#endif
}


//	#pragma mark -


extern "C" status_t
platform_allocate_region(void **_address, size_t size, uint8 protection,
	bool /*exactAddress*/)
{
	TRACE(("platform_allocate_region(&%p, %zd)\n", *_address, size));

	//get_next_virtual_address
	size = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE * B_PAGE_SIZE;
		// roundup to page size for clarity

	if (*_address != NULL) {
		// This special path is almost only useful for loading the
		// kernel into memory; it will only allow you to map the
		// 'kMaxKernelSize' bytes following the kernel base address.
		// Also, it won't check for already mapped addresses, so
		// you better know why you are here :)
		addr_t address = (addr_t)*_address;

		// is the address within the valid range?
		if (address < KERNEL_BASE
			|| address + size >= KERNEL_BASE + kMaxKernelSize) {
			TRACE(("mmu_allocate in illegal range\n address: %" B_PRIx32
				"  KERNELBASE: %" B_PRIx32 " KERNEL_BASE + kMaxKernelSize:"
				" %" B_PRIx32 "  address + size : %" B_PRIx32 " \n",
				(uint32)address, (uint32)KERNEL_BASE,
				KERNEL_BASE + kMaxKernelSize, (uint32)(address + size)));
			return B_ERROR;
		}
		TRACE(("platform_allocate_region: allocated %zd bytes at %08lx\n", size,
			address));

		return B_OK;
	}

	void *address = (void *)get_next_virtual_address(size);
	if (address == NULL)
		return B_NO_MEMORY;

	TRACE(("platform_allocate_region: allocated %zd bytes at %p\n", size,
		address));
	*_address = address;
	return B_OK;
}


extern "C" status_t
platform_free_region(void *address, size_t size)
{
	TRACE(("platform_free_region(%p, %zd)\n", address, size));
#ifdef __ARM__
	mmu_free(address, size);
#endif
	return B_OK;
}


void
platform_release_heap(struct stage2_args *args, void *base)
{
	//XXX
	// It will be freed automatically, since it is in the
	// identity mapped region, and not stored in the kernel's
	// page tables.
}


status_t
platform_init_heap(struct stage2_args *args, void **_base, void **_top)
{
	// the heap is put right before the pagetable
	void *heap = (uint8 *)sPageTable - args->heap_size;
	//FIXME: use phys addresses to allow passing args to U-Boot?

	*_base = heap;
	*_top = (void *)((int8 *)heap + args->heap_size);
	TRACE(("boot heap at 0x%p to 0x%p\n", *_base, *_top));
	return B_OK;
}
