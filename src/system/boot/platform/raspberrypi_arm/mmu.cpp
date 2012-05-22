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
#include <arm_mmu.h>
#include <kernel.h>

#include <board_config.h>

#include <OS.h>

#include <string.h>


#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x...) dprintf("mmu: " x)
#	define CALLED() dprintf("%s()\n", __func__)
#else
#	define TRACE(x) ;
#	define CALLED() ;
#endif


#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define TRACE_MEMORY_MAP
	// Define this to print the memory map to serial debug,
	// You also need to define ENABLE_SERIAL in serial.cpp
	// for output to work.

extern uint8 __stack_start;
extern uint8 __stack_end;


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


static struct memblock LOADER_MEMORYMAP[] = {
	{
		"devices",
		PERIPHERAL_BASE,
		PERIPHERAL_BASE + PERIPHERAL_SIZE,
		MMU_L2_FLAG_B,
	},
	// Device memory between 0x0 and 0x10000000
	// (0x0) Ram / Video ram (0x10000000) (256MB)
	// We don't detect the split yet, se we have to be
	// careful not to run into video ram.
	{
		"RAM_loader", // 1MB for the loader
		SDRAM_BASE + 0,
		SDRAM_BASE + 0x0fffff,
		MMU_L2_FLAG_C,
	},
	{
		"RAM_pt", // Page Table 1MB
		SDRAM_BASE + 0x100000,
		SDRAM_BASE + 0x1FFFFF,
		MMU_L2_FLAG_C,
	},
	{
		"RAM_free", // 16MB free RAM (more but we don't map it automaticaly)
		SDRAM_BASE + 0x0200000,
		SDRAM_BASE + 0x11FFFFF,
		MMU_L2_FLAG_C,
	},
	{
		"RAM_stack", // stack
		SDRAM_BASE + 0x1200000,
		SDRAM_BASE + 0x2000000,
		MMU_L2_FLAG_C,
	},
	{
		"RAM_initrd", // stack
		SDRAM_BASE + 0x2000000,
		SDRAM_BASE + 0x2500000,
		MMU_L2_FLAG_C,
	},
	// (0x2500000 ~37MB)
#ifdef FB_BASE
	{
		"framebuffer", // 2MB framebuffer ram
		FB_BASE,
		FB_BASE + FB_SIZE - 1,
		MMU_L2_FLAG_AP_RW|MMU_L2_FLAG_C,
	},
#endif
};


//static const uint32 kDefaultPageTableFlags = MMU_FLAG_READWRITE;
	// not cached not buffered, R/W
static const size_t kMaxKernelSize = 0x200000;		// 2 MB for the kernel

static addr_t sNextPhysicalAddress = 0; //will be set by mmu_init
static addr_t sNextVirtualAddress = KERNEL_BASE + kMaxKernelSize;
static addr_t sMaxVirtualAddress = KERNEL_BASE + kMaxKernelSize;

static addr_t sNextPageTableAddress = 0;
//the page directory is in front of the pagetable
static uint32 kPageTableRegionEnd = 0;

// working page directory and page table
static uint32 *sPageDirectory = 0 ;
//page directory has to be on a multiple of 16MB for
//some arm processors


static addr_t
get_next_virtual_address(size_t size)
{
	addr_t address = sNextVirtualAddress;
	sNextVirtualAddress += size;

	return address;
}


static addr_t
get_next_virtual_address_alligned (size_t size, uint32 mask)
{
	addr_t address = (sNextVirtualAddress) & mask;
	sNextVirtualAddress = address + size;

	return address;
}


static addr_t
get_next_physical_address(size_t size)
{
	addr_t address = sNextPhysicalAddress;
	sNextPhysicalAddress += size;

	return address;
}


static addr_t
get_next_physical_address_alligned(size_t size, uint32 mask)
{
	addr_t address = sNextPhysicalAddress & mask;
	sNextPhysicalAddress = address + size;

	return address;
}


static addr_t
get_next_virtual_page(size_t pagesize)
{
	return get_next_virtual_address_alligned(pagesize, 0xffffffc0);
}


static addr_t
get_next_physical_page(size_t pagesize)
{
	return get_next_physical_address_alligned(pagesize, 0xffffffc0);
}


/*
 * Set translation table base
 */
void
mmu_set_TTBR(uint32 ttb)
{
	TRACE("%s: Set Translation Table Base to 0x%lx\n", __func__);
	ttb &= 0xffffc000;
	asm volatile("MRC p15, 0, %[adr], c2, c0, 0"::[adr] "r" (ttb));
}


/*
 * Flush the TLB
 */
void
mmu_flush_TLB()
{
	TRACE("%s: TLB Flush\n", __func__);
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


void
mmu_write_DACR(uint32 value)
{
	TRACE("%s: Set Domain Access Register to 0x%lx\n", __func__);
	asm volatile("MCR p15, 0, %[c1in], c3, c0, 0"::[c1in] "r" (value));
}


static uint32 *
get_next_page_table(uint32 type)
{
	TRACE("%s: sNextPageTableAddress %p, kPageTableRegionEnd %p, "
		"type 0x%" B_PRIX32 "\n", __func__, sNextPageTableAddress,
		kPageTableRegionEnd, type);

	size_t size = 0;
	switch(type) {
		case MMU_L1_TYPE_COARSE:
		default:
			size = 1024;
			break;
		case MMU_L1_TYPE_FINE:
			size = 4096;
			break;
		case MMU_L1_TYPE_SECTION:
			size = 16384;
			break;
	}

	addr_t address = sNextPageTableAddress;
	if (address >= kPageTableRegionEnd) {
		TRACE("%s: outside of pagetableregion!\n", __func__);
		return (uint32 *)get_next_physical_address_alligned(size, 0xffffffc0);
	}

	sNextPageTableAddress += size;
	return (uint32 *)address;
}


void
init_page_directory()
{
	CALLED();
	uint32 smalltype;

	// see if subpages disabled
	if (mmu_read_C1() & (1<<23))
		smalltype = MMU_L2_TYPE_SMALLNEW;
	else
		smalltype = MMU_L2_TYPE_SMALLEXT;

	gKernelArgs.arch_args.phys_pgdir = (uint32)sPageDirectory;

	// clear out the pgdir
	for (uint32 i = 0; i < 4096; i++)
		sPageDirectory[i] = 0;

	uint32 *pageTable = NULL;
	for (uint32 i = 0; i < ARRAY_SIZE(LOADER_MEMORYMAP);i++) {

		pageTable = get_next_page_table(MMU_L1_TYPE_COARSE);
		TRACE("BLOCK: %s START: %lx END %lx\n", LOADER_MEMORYMAP[i].name,
			LOADER_MEMORYMAP[i].start, LOADER_MEMORYMAP[i].end);
		addr_t pos = LOADER_MEMORYMAP[i].start;

		int c = 0;
		while (pos < LOADER_MEMORYMAP[i].end) {
			pageTable[c] = pos |  LOADER_MEMORYMAP[i].flags | smalltype;

			c++;
			if (c > 255) { // we filled a pagetable => we need a new one
				// there is 1MB per pagetable so:
				sPageDirectory[VADDR_TO_PDENT(pos)]
					= (uint32)pageTable | MMU_L1_TYPE_COARSE;
				pageTable = get_next_page_table(MMU_L1_TYPE_COARSE);
				c = 0;
			}

			pos += B_PAGE_SIZE;
		}

		if (c > 0) {
			sPageDirectory[VADDR_TO_PDENT(pos)]
				= (uint32)pageTable | MMU_L1_TYPE_COARSE;
		}
	}
	TRACE("%s: Page table setup complete\n", __func__);

	// TLB Flush
	mmu_flush_TLB();

	// Set Translation Table Base
	mmu_set_TTBR((uint32)sPageDirectory);

	// TLB Flush
	mmu_flush_TLB();

	// Set domain access register
	mmu_write_DACR(0xFFFFFFFF);

	TRACE("%s: Enable MMU...\n", __func__);
	mmu_write_C1(mmu_read_C1() | 0x1);

	TRACE("%s: Complete\n", __func__);
}


/*!     Adds a new page table for the specified base address */
static void
add_page_table(addr_t base)
{
	TRACE("%s: base = %p\n", __func__, (void *)base);

	// Get new page table and clear it out
	uint32 *pageTable = get_next_page_table(MMU_L1_TYPE_COARSE);
/*
	if (pageTable > (uint32 *)(8 * 1024 * 1024)) {
		panic("tried to add page table beyond the indentity mapped 8 MB "
			"region\n");
	}
*/
	for (int32 i = 0; i < 256; i++)
		pageTable[i] = 0;

	// put the new page table into the page directory
	sPageDirectory[VADDR_TO_PDENT(base)]
		= (uint32)pageTable | MMU_L1_TYPE_COARSE;
}


/*!	Creates an entry to map the specified virtualAddress to the given
	physicalAddress.
	If the mapping goes beyond the current page table, it will allocate
	a new one. If it cannot map the requested page, it panics.
*/
static void
map_page(addr_t virtualAddress, addr_t physicalAddress, uint32 flags)
{
	TRACE("%s: vaddr 0x%lx, paddr 0x%lx\n", __func__, virtualAddress,
		physicalAddress);

	if (virtualAddress < KERNEL_BASE) {
		panic("%s: asked to map invalid page %p!\n", __func__,
			(void *)virtualAddress);
	}

	if (virtualAddress >= sMaxVirtualAddress) {
		// we need to add a new page table
		add_page_table(sMaxVirtualAddress);
		sMaxVirtualAddress += B_PAGE_SIZE * 256;

		if (virtualAddress >= sMaxVirtualAddress) {
			panic("%s: asked to map a page to %p\n", __func__,
				(void *)virtualAddress);
		}
	}

	physicalAddress &= ~(B_PAGE_SIZE - 1);

	// map the page to the correct page table
	uint32 *pageTable
		= (uint32 *)(sPageDirectory[VADDR_TO_PDENT(virtualAddress)]
			& ARM_PDE_ADDRESS_MASK);

	TRACE("%s: pageTable 0x%lx\n", __func__,
		sPageDirectory[VADDR_TO_PDENT(virtualAddress)] & ARM_PDE_ADDRESS_MASK);

	if (pageTable == NULL) {
		add_page_table(virtualAddress);
		pageTable = (uint32 *)(sPageDirectory[VADDR_TO_PDENT(virtualAddress)]
			& ARM_PDE_ADDRESS_MASK);
	}

	uint32 tableEntry = VADDR_TO_PTENT(virtualAddress);

	TRACE("%s: inserting pageTable %p, tableEntry %ld, physicalAddress %p\n",
		__func__, pageTable, tableEntry, physicalAddress);

	pageTable[tableEntry] = physicalAddress | flags;

	mmu_flush_TLB();

	TRACE("%s: done\n", __func__);
}


//	#pragma mark -


extern "C" addr_t
mmu_map_physical_memory(addr_t physicalAddress, size_t size, uint32 flags)
{
	addr_t address = sNextVirtualAddress;
	addr_t pageOffset = physicalAddress & (B_PAGE_SIZE - 1);

	physicalAddress -= pageOffset;

	for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
		map_page(get_next_virtual_page(B_PAGE_SIZE), physicalAddress + offset,
			flags);
	}

	return address + pageOffset;
}


static void
unmap_page(addr_t virtualAddress)
{
	TRACE("%s: virtualAddress = %p\n", __func__, (void *)virtualAddress);

	if (virtualAddress < KERNEL_BASE) {
		panic("%s: asked to unmap invalid page %p!\n", __func__,
			(void *)virtualAddress);
	}

	// unmap the page from the correct page table
	uint32 *pageTable
		= (uint32 *)(sPageDirectory[VADDR_TO_PDENT(virtualAddress)]
			& ARM_PDE_ADDRESS_MASK);

	pageTable[VADDR_TO_PTENT(virtualAddress)] = 0;

	mmu_flush_TLB();
}


extern "C" void *
mmu_allocate(void *virtualAddress, size_t size)
{
	TRACE("%s: requested vaddr: %p, next free vaddr: 0x%lx, size: %ld\n",
		__func__, virtualAddress, sNextVirtualAddress, size);

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
		if (address < KERNEL_BASE
			|| address + size >= KERNEL_BASE + kMaxKernelSize) {
			TRACE("mmu_allocate in illegal range\n address: %lx"
				"  KERNELBASE: %lx KERNEL_BASE + kMaxKernelSize: %lx"
				"  address + size : %lx \n", (uint32)address, KERNEL_BASE,
				KERNEL_BASE + kMaxKernelSize, (uint32)(address + size));
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
	TRACE("%s: virtualAddress = %p, size: %ld\n", __func__,
		virtualAddress, size);

	addr_t address = (addr_t)virtualAddress;
	size = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
		// get number of pages to map

	// is the address within the valid range?
	if (address < KERNEL_BASE
		|| address + size >= KERNEL_BASE + kMaxKernelSize) {
		panic("%s: asked to unmap out of range region (%p, size %lx)\n",
			__func__, (void *)address, size);
	}

	// unmap all pages within the range
	for (uint32 i = 0; i < size; i++) {
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
	CALLED();

	// save the memory we've physically allocated
	gKernelArgs.physical_allocated_range[0].size
		= sNextPhysicalAddress - gKernelArgs.physical_allocated_range[0].start;

	// Save the memory we've virtually allocated (for the kernel and other
	// stuff)
	gKernelArgs.virtual_allocated_range[0].start = KERNEL_BASE;
	gKernelArgs.virtual_allocated_range[0].size
		= sNextVirtualAddress - KERNEL_BASE;
	gKernelArgs.num_virtual_allocated_ranges = 1;

#ifdef TRACE_MEMORY_MAP
	{
		uint32 i;

		dprintf("phys memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
			dprintf("    base 0x%08lx, length 0x%08lx\n",
				gKernelArgs.physical_memory_range[i].start,
				gKernelArgs.physical_memory_range[i].size);
		}

		dprintf("allocated phys memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_physical_allocated_ranges; i++) {
			dprintf("    base 0x%08lx, length 0x%08lx\n",
				gKernelArgs.physical_allocated_range[i].start,
				gKernelArgs.physical_allocated_range[i].size);
		}

		dprintf("allocated virt memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_virtual_allocated_ranges; i++) {
			dprintf("    base 0x%08lx, length 0x%08lx\n",
				gKernelArgs.virtual_allocated_range[i].start,
				gKernelArgs.virtual_allocated_range[i].size);
		}
	}
#endif
}


extern "C" void
mmu_init(void)
{
	CALLED();

	mmu_write_C1(mmu_read_C1() & ~((1 << 29) | (1 << 28) | (1 << 0)));
		// access flag disabled, TEX remap disabled, mmu disabled

	uint32 highestRAMAddress = SDRAM_BASE;

	// calculate lowest RAM adress from MEMORYMAP
	for (uint32 i = 0; i < ARRAY_SIZE(LOADER_MEMORYMAP); i++) {
		if (strcmp("RAM_free", LOADER_MEMORYMAP[i].name) == 0)
			sNextPhysicalAddress = LOADER_MEMORYMAP[i].start;

		if (strcmp("RAM_pt", LOADER_MEMORYMAP[i].name) == 0) {
			sNextPageTableAddress = LOADER_MEMORYMAP[i].start
				+ MMU_L1_TABLE_SIZE;
			kPageTableRegionEnd = LOADER_MEMORYMAP[i].end;
			sPageDirectory = (uint32 *) LOADER_MEMORYMAP[i].start;
		}

		if (strncmp("RAM_", LOADER_MEMORYMAP[i].name, 4) == 0) {
			if (LOADER_MEMORYMAP[i].end > highestRAMAddress)
				highestRAMAddress = LOADER_MEMORYMAP[i].end;
		}
	}

	gKernelArgs.physical_memory_range[0].start = SDRAM_BASE;
	gKernelArgs.physical_memory_range[0].size = highestRAMAddress - SDRAM_BASE;
	gKernelArgs.num_physical_memory_ranges = 1;

	gKernelArgs.physical_allocated_range[0].start = SDRAM_BASE;
	gKernelArgs.physical_allocated_range[0].size = 0;
	gKernelArgs.num_physical_allocated_ranges = 1;
		// remember the start of the allocated physical pages

	init_page_directory();

	// map the page directory on the next vpage
	gKernelArgs.arch_args.vir_pgdir = mmu_map_physical_memory(
		(addr_t)sPageDirectory, MMU_L1_TABLE_SIZE, kDefaultPageFlags);

	// map in a kernel stack
	gKernelArgs.cpu_kstack[0].start = (addr_t)mmu_allocate(NULL,
		KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE);
	gKernelArgs.cpu_kstack[0].size = KERNEL_STACK_SIZE
		+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;

	TRACE("kernel stack at 0x%lx to 0x%lx\n", gKernelArgs.cpu_kstack[0].start,
		gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size);
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
	void *heap = (void *)get_next_physical_address(args->heap_size);
	if (heap == NULL)
		return B_NO_MEMORY;

	*_base = heap;
	*_top = (void *)((int8 *)heap + args->heap_size);
	return B_OK;
}
