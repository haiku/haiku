/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Based on code written by Travis Geiselbrecht for NewOS.
 *
 * Distributed under the terms of the MIT License.
 */


#include "arch_mmu.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <arm_mmu.h>
#include <kernel.h>

#include <OS.h>

#include <string.h>

#include "fdt_support.h"

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};


#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define TRACE_MEMORY_MAP
	// Define this to print the memory map to serial debug,
	// You also need to define ENABLE_SERIAL in serial.cpp
	// for output to work.

extern void *gFDT;


/*
TODO:
	-recycle bit!
*/

/*!	The (physical) memory layout of the boot loader is currently as follows:
	 0x00000000			u-boot (run from NOR flash)
	 0xa0000000			u-boot stuff like kernel arguments afaik
	 0xa0100000 - 0xa0ffffff	boot.tgz (up to 15MB probably never needed so big...)
	 0xa1000000 - 0xa1ffffff	pagetables
	 0xa2000000 - ?			code (up to 1MB)
	 0xa2100000			boot loader heap / free physical memory

	The kernel is mapped at KERNEL_LOAD_BASE, all other stuff mapped by the
	loader (kernel args, modules, driver settings, ...) comes after
	0x80020000 which means that there is currently only 2 MB reserved for
	the kernel itself (see kMaxKernelSize).
*/


// 8 MB for the kernel, kernel args, modules, driver settings, ...
static const size_t kMaxKernelSize = 0x800000;

// Start and end of ourselfs (from ld script)
extern int _start, _end;

/*
*defines a block in memory
*/
struct memblock {
	const char name[16];
		// the name will be used for debugging etc later perhaps...
	addr_t	start;
		// start of the block
	addr_t	end;
		// end of the block
	uint32	flags;
		// which flags should be applied (device/normal etc..)
};


#warning TODO: Plot pref. base from fdt!
static struct memblock LOADER_MEMORYMAP[] = {
/*
	{
		"devices",
		DEVICE_BASE,
		DEVICE_BASE + DEVICE_SIZE - 1,
		ARM_MMU_L2_FLAG_B,
	},
*/
	{
		"RAM_kernel", // 8MB space for kernel, drivers etc
		KERNEL_LOAD_BASE,
		KERNEL_LOAD_BASE + kMaxKernelSize - 1,
		ARM_MMU_L2_FLAG_C,
	},

#ifdef FB_BASE
	{
		"framebuffer", // 2MB framebuffer ram
		FB_BASE,
		FB_BASE + FB_SIZE - 1,
		ARM_MMU_L2_FLAG_AP_RW | ARM_MMU_L2_FLAG_C,
	},
#endif
};

//static const uint32 kDefaultPageTableFlags = MMU_FLAG_READWRITE;
	// not cached not buffered, R/W

static addr_t sNextPhysicalAddress = 0; //will be set by mmu_init
static addr_t sNextVirtualAddress = 0;

static addr_t sNextPageTableAddress = 0;
//the page directory is in front of the pagetable
static uint32 sPageTableRegionEnd = 0;

static uint32 sSmallPageType = ARM_MMU_L2_TYPE_SMALLEXT;

// working page directory and page table
static uint32 *sPageDirectory = 0 ;
//page directory has to be on a multiple of 16MB for
//some arm processors


static addr_t
get_next_virtual_address_aligned(size_t size, uint32 mask)
{
	addr_t address = (sNextVirtualAddress) & mask;
	sNextVirtualAddress = address + size;

	return address;
}


static addr_t
get_next_physical_address_aligned(size_t size, uint32 mask)
{
	addr_t address = sNextPhysicalAddress & mask;
	sNextPhysicalAddress = address + size;

	return address;
}


static addr_t
get_next_virtual_page(size_t pagesize)
{
	return get_next_virtual_address_aligned(pagesize, 0xffffffc0);
}


static addr_t
get_next_physical_page(size_t pagesize)
{
	return get_next_physical_address_aligned(pagesize, 0xffffffc0);
}


/*
 * Set translation table base
 */
void
mmu_set_TTBR(uint32 ttb)
{
	ttb &= 0xffffc000;
	asm volatile("MCR p15, 0, %[adr], c2, c0, 0"::[adr] "r" (ttb));
}


/*
 * Flush the TLB
 */
void
mmu_flush_TLB()
{
	uint32 value = 0;
	asm volatile("MCR p15, 0, %[c8format], c8, c7, 0"::[c8format] "r" (value));
}


/*
 * Read MMU Control Register
 */
uint32
mmu_read_C1()
{
	uint32 controlReg = 0;
	asm volatile("MRC p15, 0, %[c1out], c1, c0, 0":[c1out] "=r" (controlReg));
	return controlReg;
}


/*
 * Write MMU Control Register
 */
void
mmu_write_C1(uint32 value)
{
	asm volatile("MCR p15, 0, %[c1in], c1, c0, 0"::[c1in] "r" (value));
}


/*
 * Dump current MMU Control Register state
 * For debugging, can be added to loader temporarly post-serial-init
 */
void
mmu_dump_C1()
{
	uint32 cpValue = mmu_read_C1();

	dprintf("MMU CP15:c1 State:\n");

	if ((cpValue & (1 << 0)) != 0)
		dprintf(" - MMU Enabled\n");
	else
		dprintf(" - MMU Disabled\n");

	if ((cpValue & (1 << 2)) != 0)
		dprintf(" - Data Cache Enabled\n");
	else
		dprintf(" - Data Cache Disabled\n");

	if ((cpValue & (1 << 3)) != 0)
		dprintf(" - Write Buffer Enabled\n");
	else
		dprintf(" - Write Buffer Disabled\n");

	if ((cpValue & (1 << 12)) != 0)
		dprintf(" - Instruction Cache Enabled\n");
	else
		dprintf(" - Instruction Cache Disabled\n");

	if ((cpValue & (1 << 13)) != 0)
		dprintf(" - Vector Table @ 0xFFFF0000\n");
	else
		dprintf(" - Vector Table @ 0x00000000\n");
}


void
mmu_write_DACR(uint32 value)
{
	asm volatile("MCR p15, 0, %[c1in], c3, c0, 0"::[c1in] "r" (value));
}


static uint32 *
get_next_page_table(uint32 type)
{
	TRACE(("get_next_page_table, sNextPageTableAddress 0x%" B_PRIxADDR
		", sPageTableRegionEnd 0x%" B_PRIxADDR ", type 0x%" B_PRIx32 "\n",
		sNextPageTableAddress, sPageTableRegionEnd, type));

	size_t size = 0;
	size_t entryCount = 0;
	switch (type) {
		case ARM_MMU_L1_TYPE_COARSE:
			size = ARM_MMU_L2_COARSE_TABLE_SIZE;
			entryCount = ARM_MMU_L2_COARSE_ENTRY_COUNT;
			break;
		case ARM_MMU_L1_TYPE_FINE:
			size = ARM_MMU_L2_FINE_TABLE_SIZE;
			entryCount = ARM_MMU_L2_FINE_ENTRY_COUNT;
			break;
		default:
			panic("asked for unknown page table type: %#" B_PRIx32 "\n", type);
			return NULL;
	}

	addr_t address = sNextPageTableAddress;
	if (address < sPageTableRegionEnd)
		sNextPageTableAddress += size;
	else {
		TRACE(("page table allocation outside of pagetable region!\n"));
		address = get_next_physical_address_aligned(size, 0xffffffc0);
	}

	uint32 *pageTable = (uint32 *)address;
	for (size_t i = 0; i < entryCount; i++)
		pageTable[i] = 0;

	return pageTable;
}


static uint32 *
get_or_create_page_table(addr_t address, uint32 type)
{
	uint32 *pageTable = NULL;
	uint32 pageDirectoryIndex = VADDR_TO_PDENT(address);
	uint32 pageDirectoryEntry = sPageDirectory[pageDirectoryIndex];

	uint32 entryType = pageDirectoryEntry & ARM_PDE_TYPE_MASK;
	if (entryType == ARM_MMU_L1_TYPE_FAULT) {
		// This page directory entry has not been set yet, allocate it.
		pageTable = get_next_page_table(type);
		sPageDirectory[pageDirectoryIndex] = (uint32)pageTable | type;
		return pageTable;
	}

	if (entryType != type) {
		// This entry has been allocated with a different type!
		panic("tried to reuse page directory entry %" B_PRIu32
			" with different type (entry: %#" B_PRIx32 ", new type: %#" B_PRIx32
			")\n", pageDirectoryIndex, pageDirectoryEntry, type);
		return NULL;
	}

	return (uint32 *)(pageDirectoryEntry & ARM_PDE_ADDRESS_MASK);
}


static void
mmu_map_identity(addr_t start, size_t end, int flags)
{
	uint32 *pageTable = NULL;
	uint32 pageTableIndex = 0;

	start = ROUNDDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

	TRACE(("mmu_map_identity: [ %" B_PRIxADDR " - %" B_PRIxADDR "]\n", start, end));

	for (addr_t address = start; address < end; address += B_PAGE_SIZE) {
		if (pageTable == NULL
			|| pageTableIndex >= ARM_MMU_L2_COARSE_ENTRY_COUNT) {
			pageTable = get_or_create_page_table(address,
				ARM_MMU_L1_TYPE_COARSE);
			pageTableIndex = VADDR_TO_PTENT(address);
		}

		pageTable[pageTableIndex++]
			= address | flags | sSmallPageType;
	}
}

void
init_page_directory()
{
	TRACE(("init_page_directory\n"));

	gKernelArgs.arch_args.phys_pgdir =
	gKernelArgs.arch_args.vir_pgdir = (uint32)sPageDirectory;

	// clear out the page directory
	for (uint32 i = 0; i < ARM_MMU_L1_TABLE_ENTRY_COUNT; i++)
		sPageDirectory[i] = 0;

	// map ourselfs first... just to make sure
	mmu_map_identity((addr_t)&_start, (addr_t)&_end, ARM_MMU_L2_FLAG_C);

	// map our page directory region (TODO should not be identity mapped)
	mmu_map_identity((addr_t)sPageDirectory, sPageTableRegionEnd, ARM_MMU_L2_FLAG_C);

	for (uint32 i = 0; i < B_COUNT_OF(LOADER_MEMORYMAP); i++) {

		TRACE(("BLOCK: %s START: %lx END %lx\n", LOADER_MEMORYMAP[i].name,
			LOADER_MEMORYMAP[i].start, LOADER_MEMORYMAP[i].end));

		addr_t address = LOADER_MEMORYMAP[i].start;
		ASSERT((address & ~ARM_PTE_ADDRESS_MASK) == 0);

		mmu_map_identity(LOADER_MEMORYMAP[i].start, LOADER_MEMORYMAP[i].end,
			LOADER_MEMORYMAP[i].flags);
	}

	mmu_flush_TLB();

	/* set up the translation table base */
	mmu_set_TTBR((uint32)sPageDirectory);

	mmu_flush_TLB();

	/* set up the domain access register */
	mmu_write_DACR(0xFFFFFFFF);

	/* turn on the mmu */
	mmu_write_C1(mmu_read_C1() | 0x1);
}


/*!	Creates an entry to map the specified virtualAddress to the given
	physicalAddress.
	If the mapping goes beyond the current page table, it will allocate
	a new one. If it cannot map the requested page, it panics.
*/
static void
map_page(addr_t virtualAddress, addr_t physicalAddress, uint32 flags)
{
	TRACE(("map_page: vaddr 0x%lx, paddr 0x%lx\n", virtualAddress,
		physicalAddress));

	if (virtualAddress < KERNEL_LOAD_BASE) {
		panic("map_page: asked to map invalid page %p!\n",
			(void *)virtualAddress);
	}

	physicalAddress &= ~(B_PAGE_SIZE - 1);

	// map the page to the correct page table
	uint32 *pageTable = get_or_create_page_table(virtualAddress,
		ARM_MMU_L1_TYPE_COARSE);

	uint32 pageTableIndex = VADDR_TO_PTENT(virtualAddress);
	TRACE(("map_page: inserting pageTable %p, tableEntry 0x%" B_PRIx32
		", physicalAddress 0x%" B_PRIxADDR "\n", pageTable, pageTableIndex,
		physicalAddress));

	pageTable[pageTableIndex] = physicalAddress | flags;

	mmu_flush_TLB();

	TRACE(("map_page: done\n"));
}


//	#pragma mark -


extern "C" addr_t
mmu_map_physical_memory(addr_t physicalAddress, size_t size, uint32 flags)
{
	TRACE(("mmu_map_physical_memory(phAddr=%lx, %lx, %lu)\n", physicalAddress, size, flags));
	addr_t address = sNextVirtualAddress;
	addr_t pageOffset = physicalAddress & (B_PAGE_SIZE - 1);

	physicalAddress -= pageOffset;
	if (pageOffset)
		size += B_PAGE_SIZE;

	for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
		map_page(get_next_virtual_page(B_PAGE_SIZE), physicalAddress + offset,
			flags);
	}

	return address + pageOffset;
}


static void
unmap_page(addr_t virtualAddress)
{
	TRACE(("unmap_page(virtualAddress = %p)\n", (void *)virtualAddress));

	if (virtualAddress < KERNEL_LOAD_BASE) {
		panic("unmap_page: asked to unmap invalid page %p!\n",
			(void *)virtualAddress);
	}

	// unmap the page from the correct page table
	uint32 *pageTable
		= (uint32 *)(sPageDirectory[VADDR_TO_PDENT(virtualAddress)]
			& ARM_PDE_ADDRESS_MASK);

	pageTable[VADDR_TO_PTENT(virtualAddress)] = 0;

	mmu_flush_TLB();
}


// XXX: use phys_addr_t ?
extern "C" bool
mmu_get_virtual_mapping(addr_t virtualAddress, /*phys_*/addr_t *_physicalAddress)
{
	if (virtualAddress < KERNEL_LOAD_BASE) {
		panic("mmu_get_virtual_mapping: asked to lookup invalid page %p!\n",
			(void *)virtualAddress);
		return false;
	}

	// map the page to the correct page table
	uint32 *pageTable = get_or_create_page_table(virtualAddress,
		ARM_MMU_L1_TYPE_COARSE);

	uint32 pageTableIndex = VADDR_TO_PTENT(virtualAddress);

	*_physicalAddress = pageTable[pageTableIndex] & ~(B_PAGE_SIZE - 1);

	return true;
}


extern "C" void *
mmu_allocate(void *virtualAddress, size_t size)
{
	TRACE(("mmu_allocate: requested vaddr: %p, next free vaddr: 0x%lx, size: "
		"%ld\n", virtualAddress, sNextVirtualAddress, size));

	size = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
		// get number of pages to map

	if (virtualAddress != NULL) {
		// This special path is almost only useful for loading the
		// kernel into memory; it will only allow you to map the
		// 'kMaxKernelSize' bytes following the kernel base address.
		// Also, it won't check for already mapped addresses, so
		// you better know why you are here :)
		addr_t address = (addr_t)virtualAddress;

		// is the address within the valid range?
		if (address < KERNEL_LOAD_BASE || address + size * B_PAGE_SIZE
			>= KERNEL_LOAD_BASE + kMaxKernelSize) {
			TRACE(("mmu_allocate in illegal range\n address: %" B_PRIx32
				"  KERNELBASE: %" B_PRIx32 " KERNEL_LOAD_BASE + kMaxKernelSize:"
				" %" B_PRIx32 "  address + size : %" B_PRIx32 "\n",
				(uint32)address, (uint32)KERNEL_LOAD_BASE,
				(uint32)KERNEL_LOAD_BASE + kMaxKernelSize,
				(uint32)(address + size)));
			return NULL;
		}
		for (uint32 i = 0; i < size; i++) {
			map_page(address, get_next_physical_page(B_PAGE_SIZE),
				kDefaultPageFlags);
			address += B_PAGE_SIZE;
		}

		return virtualAddress;
	}

	void *address = (void *)sNextVirtualAddress;

	for (uint32 i = 0; i < size; i++) {
		map_page(get_next_virtual_page(B_PAGE_SIZE),
			get_next_physical_page(B_PAGE_SIZE), kDefaultPageFlags);
	}

	return address;
}


/*!	This will unmap the allocated chunk of memory from the virtual
	address space. It might not actually free memory (as its implementation
	is very simple), but it might.
*/
extern "C" void
mmu_free(void *virtualAddress, size_t size)
{
	TRACE(("mmu_free(virtualAddress = %p, size: %ld)\n", virtualAddress, size));

	addr_t address = (addr_t)virtualAddress;
	addr_t pageOffset = address % B_PAGE_SIZE;
	address -= pageOffset;
	size = (size + pageOffset + B_PAGE_SIZE - 1) / B_PAGE_SIZE * B_PAGE_SIZE;

	// is the address within the valid range?
	if (address < KERNEL_LOAD_BASE || address + size > sNextVirtualAddress) {
		panic("mmu_free: asked to unmap out of range region (%p, size %lx)\n",
			(void *)address, size);
	}

	// unmap all pages within the range
	for (size_t i = 0; i < size; i += B_PAGE_SIZE) {
		unmap_page(address);
		address += B_PAGE_SIZE;
	}

	if (address == sNextVirtualAddress) {
		// we can actually reuse the virtual address space
		sNextVirtualAddress -= size;
	}
}


/*!	Sets up the final and kernel accessible GDT and IDT tables.
	BIOS calls won't work any longer after this function has
	been called.
*/
extern "C" void
mmu_init_for_kernel(void)
{
	TRACE(("mmu_init_for_kernel\n"));

	// store next available pagetable in our pagedir mapping, for
	// the kernel to use in early vm setup
	gKernelArgs.arch_args.next_pagetable = sNextPageTableAddress - (addr_t)sPageDirectory;

	// save the memory we've physically allocated
	insert_physical_allocated_range((addr_t)sPageDirectory, sNextPhysicalAddress - (addr_t)sPageDirectory);

	// Save the memory we've virtually allocated (for the kernel and other
	// stuff)
	insert_virtual_allocated_range(KERNEL_LOAD_BASE, sNextVirtualAddress - KERNEL_LOAD_BASE);

#ifdef TRACE_MEMORY_MAP
	{
		uint32 i;

		dprintf("phys memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
			dprintf("    base 0x%08" B_PRIx64 ", length 0x%08" B_PRIx64 "\n",
				gKernelArgs.physical_memory_range[i].start,
				gKernelArgs.physical_memory_range[i].size);
		}

		dprintf("allocated phys memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_physical_allocated_ranges; i++) {
			dprintf("    base 0x%08" B_PRIx64 ", length 0x%08" B_PRIx64 "\n",
				gKernelArgs.physical_allocated_range[i].start,
				gKernelArgs.physical_allocated_range[i].size);
		}

		dprintf("allocated virt memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_virtual_allocated_ranges; i++) {
			dprintf("    base 0x%08" B_PRIx64 ", length 0x%08" B_PRIx64 "\n",
				gKernelArgs.virtual_allocated_range[i].start,
				gKernelArgs.virtual_allocated_range[i].size);
		}
	}
#endif
}


//TODO:move this to generic/ ?
static status_t
find_physical_memory_ranges(uint64 &total)
{
	int node;
	const void *prop;
	int len;

	dprintf("checking for memory...\n");
	// let's just skip the OF way (prop memory on /chosen)
	//node = fdt_path_offset(gFDT, "/chosen");
	node = fdt_path_offset(gFDT, "/memory");
	// TODO: check devicetype=="memory" ?

	total = 0;

	int32 regAddressCells = 1;
	int32 regSizeCells = 1;
	fdt_get_cell_count(gFDT, node, regAddressCells, regSizeCells);

	prop = fdt_getprop(gFDT, node, "reg", &len);
	if (prop == NULL) {
		panic("FDT /memory reg property not set");
		return B_ERROR;
	}

	const uint32 *p = (const uint32 *)prop;
	for (int32 i = 0; len; i++) {
		uint64 base;
		uint64 size;
		if (regAddressCells == 2)
			base = fdt64_to_cpu(*(uint64_t *)p);
		else
			base = fdt32_to_cpu(*(uint32_t *)p);
		p += regAddressCells;
		if (regSizeCells == 2)
			size = fdt64_to_cpu(*(uint64_t *)p);
		else
			size = fdt32_to_cpu(*(uint32_t *)p);
		p += regSizeCells;
		len -= sizeof(uint32) * (regAddressCells + regSizeCells);

		if (size <= 0) {
			dprintf("%ld: empty region\n", i);
			continue;
		}
		dprintf("%" B_PRIu32 ": base = %" B_PRIu64 ","
			"size = %" B_PRIu64 "\n", i, base, size);

		total += size;

		if (insert_physical_memory_range(base, size) != B_OK) {
			dprintf("cannot map physical memory range "
				"(num ranges = %" B_PRIu32 ")!\n",
				gKernelArgs.num_physical_memory_ranges);
			return B_ERROR;
		}
	}

	return B_OK;
}


extern "C" void
mmu_init(void)
{
	TRACE(("mmu_init\n"));

	// skip RAM check if already done (rPi)
	if (gKernelArgs.num_physical_memory_ranges == 0) {
		// get map of physical memory (fill in kernel_args structure)

		uint64 total;
		if (find_physical_memory_ranges(total) != B_OK) {
			dprintf("Error: could not find physical memory ranges!\n");

#ifdef SDRAM_BASE
			dprintf("Defaulting to 32MB at %" B_PRIx64 "\n", (uint64)SDRAM_BASE);
			// specify available physical memory, using 32MB for now, since our
			// ARMv5 targets have very little by default.
			total = 32 * 1024 * 1024;
			insert_physical_memory_range(SDRAM_BASE, total);
#else
			return /*B_ERROR*/;
#endif
		}
		dprintf("total physical memory = %" B_PRId64 "MB\n",
			total / (1024 * 1024));
	}

	// see if subpages are disabled
	if (mmu_read_C1() & (1 << 23))
		sSmallPageType = ARM_MMU_L2_TYPE_SMALLNEW;

	mmu_write_C1(mmu_read_C1() & ~((1 << 29) | (1 << 28) | (1 << 0)));
		// access flag disabled, TEX remap disabled, mmu disabled

	// allocate page directory in memory after loader
	sPageDirectory = (uint32 *)ROUNDUP((addr_t)&_end, 0x100000);
	sNextPageTableAddress = (addr_t)sPageDirectory + ARM_MMU_L1_TABLE_SIZE;
	sPageTableRegionEnd = (addr_t)sPageDirectory + 0x200000;

	// Mark start for dynamic allocation
	sNextPhysicalAddress = sPageTableRegionEnd;
	// We are going to allocate early kernel stuff there, so the virtual address
	// has to be in kernel space. Either put it right after the identity mapped
	// RAM (if it happens to be in the 0x80000000 range), or at the start of
	// the kernel address space otherwise.
	sNextVirtualAddress = KERNEL_LOAD_BASE + kMaxKernelSize;
	if (sPageTableRegionEnd > sNextVirtualAddress)
		sNextVirtualAddress = sPageTableRegionEnd;

	// mark allocated ranges, so they don't get overwritten
	insert_physical_allocated_range((addr_t)&_start,
		(addr_t)&_end - (addr_t)&_start);
	insert_physical_allocated_range((addr_t)sPageDirectory, 0x200000);

	init_page_directory();

	// map in a kernel stack
	gKernelArgs.cpu_kstack[0].size = KERNEL_STACK_SIZE
		+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;
	gKernelArgs.cpu_kstack[0].start = (addr_t)mmu_allocate(NULL,
		gKernelArgs.cpu_kstack[0].size);

	TRACE(("kernel stack at 0x%" B_PRIx64 " to 0x%" B_PRIx64 "\n",
		gKernelArgs.cpu_kstack[0].start,
		gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size));
}


//	#pragma mark -


extern "C" status_t
platform_allocate_region(void **_address, size_t size, uint8 protection,
	bool /*exactAddress*/)
{
	void *address = mmu_allocate(*_address, size);
	if (address == NULL)
		return B_NO_MEMORY;

	*_address = address;
	return B_OK;
}


extern "C" status_t
platform_free_region(void *address, size_t size)
{
	mmu_free(address, size);
	return B_OK;
}


void
platform_release_heap(struct stage2_args *args, void *base)
{
	// It will be freed automatically, since it is in the
	// identity mapped region, and not stored in the kernel's
	// page tables.
}


status_t
platform_init_heap(struct stage2_args *args, void **_base, void **_top)
{
	void *heap = mmu_allocate(NULL, args->heap_size);
	if (heap == NULL)
		return B_NO_MEMORY;

	*_base = heap;
	*_top = (void *)((int8 *)heap + args->heap_size);
	return B_OK;
}
