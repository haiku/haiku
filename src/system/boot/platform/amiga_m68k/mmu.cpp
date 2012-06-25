/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Based on code written by Travis Geiselbrecht for NewOS.
 *
 * Distributed under the terms of the MIT License.
 */


#include "amiga_memory_map.h"
#include "rom_calls.h"
#include "mmu.h"

#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <arch/cpu.h>
#include <arch_kernel.h>
#include <kernel.h>

#include <OS.h>

#include <string.h>


//XXX: x86
/** The (physical) memory layout of the boot loader is currently as follows:
 *	  0x0500 - 0x10000	protected mode stack
 *	  0x0500 - 0x09000	real mode stack
 *	 0x10000 - ?		code (up to ~500 kB)
 *	 0x90000			1st temporary page table (identity maps 0-4 MB)
 *	 0x91000			2nd (4-8 MB)
 *	 0x92000 - 0x92000	further page tables
 *	 0x9e000 - 0xa0000	SMP trampoline code
 *	[0xa0000 - 0x100000	BIOS/ROM/reserved area]
 *	0x100000			page directory
 *	     ...			boot loader heap (32 kB)
 *	     ...			free physical memory
 *
 *	The first 8 MB are identity mapped (0x0 - 0x0800000); paging is turned
 *	on. The kernel is mapped at 0x80000000, all other stuff mapped by the
 *	loader (kernel args, modules, driver settings, ...) comes after
 *	0x81000000 which means that there is currently only 1 MB reserved for
 *	the kernel itself (see kMaxKernelSize).
 */

// notes m68k:
/** The (physical) memory layout of the boot loader is currently as follows:
 *	  0x0800 - 0x10000	supervisor mode stack (1) XXX: more ? x86 starts at 500
 *	 0x10000 - ?		code (up to ~500 kB)
 *  0x100000 or FAST_RAM_BASE if any:
 *	     ...			page root directory
 *	     ...			interrupt vectors (VBR)
 *	     ...			page directory
 *	     ...			boot loader heap (32 kB)
 *	     ...			free physical memory
 *  0xdNNNNN			video buffer usually there, as per v_bas_ad
 *						(=Logbase() but Physbase() is better)
 *
 *	The first 32 MB (2) are identity mapped (0x0 - 0x1000000); paging
 *	is turned on. The kernel is mapped at 0x80000000, all other stuff
 *	mapped by the loader (kernel args, modules, driver settings, ...)
 *	comes after 0x81000000 which means that there is currently only
 *	1 MB reserved for the kernel itself (see kMaxKernelSize).
 *
 *	(1) no need for user stack, we are already in supervisor mode in the
 *	loader.
 *	(2) maps the whole regular ST space; transparent translation registers
 *	have larger granularity anyway.
 */
#warning M68K: check for Physbase() < ST_RAM_TOP

//#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// since the page root directory doesn't take a full page (1k)
// we stuff some other stuff after it, like the interrupt vectors (1k)
#define VBR_PAGE_OFFSET 1024

static const uint32 kDefaultPageTableFlags = 0x07;	// present, user, R/W
static const size_t kMaxKernelSize = 0x100000;		// 1 MB for the kernel

// working page directory and page table
addr_t gPageRoot = 0;

static addr_t sNextPhysicalAddress = 0x100000;
static addr_t sNextVirtualAddress = KERNEL_BASE + kMaxKernelSize;
static addr_t sMaxVirtualAddress = KERNEL_BASE /*+ 0x400000*/;

#if 0
static addr_t sNextPageTableAddress = 0x90000;
static const uint32 kPageTableRegionEnd = 0x9e000;
	// we need to reserve 2 pages for the SMP trampoline code XXX:no
#endif

static const struct boot_mmu_ops *gMMUOps;

static addr_t
get_next_virtual_address(size_t size)
{
	addr_t address = sNextVirtualAddress;
	sNextVirtualAddress += size;

	TRACE(("%s(%d): %08x\n", __FUNCTION__, size, address));
	return address;
}


static addr_t
get_next_physical_address(size_t size)
{
	addr_t address = sNextPhysicalAddress;
	sNextPhysicalAddress += size;

	TRACE(("%s(%d): %08x\n", __FUNCTION__, size, address));
	return address;
}


static addr_t
get_next_virtual_page()
{
	TRACE(("%s\n", __FUNCTION__));
	return get_next_virtual_address(B_PAGE_SIZE);
}


static addr_t
get_next_physical_page()
{
	TRACE(("%s\n", __FUNCTION__));
	return get_next_physical_address(B_PAGE_SIZE);
}


// allocate a page worth of page dir or tables
extern "C" addr_t
mmu_get_next_page_tables()
{
#if 0
	TRACE(("mmu_get_next_page_tables, sNextPageTableAddress %p, kPageTableRegionEnd %p\n",
		sNextPageTableAddress, kPageTableRegionEnd));

	addr_t address = sNextPageTableAddress;
	if (address >= kPageTableRegionEnd)
		return (uint32 *)get_next_physical_page();

	sNextPageTableAddress += B_PAGE_SIZE;
	return (uint32 *)address;
#endif
	addr_t tbl = get_next_physical_page();
	if (!tbl)
		return tbl;
	// shouldn't we fill this ?
	//gKernelArgs.arch_args.pgtables[gKernelArgs.arch_args.num_pgtables++] = (uint32)pageTable;

#if 0
	// clear them
	uint32 *p = (uint32 *)tbl;
	for (int32 i = 0; i < 1024; i++)
		p[i] = 0;
#endif
	return tbl;
}

#if 0
/**	Adds a new page table for the specified base address */

static void
add_page_table(addr_t base)
{
	TRACE(("add_page_table(base = %p)\n", (void *)base));
#if 0

	// Get new page table and clear it out
	uint32 *pageTable = mmu_get_next_page_tables();
	if (pageTable > (uint32 *)(8 * 1024 * 1024))
		panic("tried to add page table beyond the indentity mapped 8 MB region\n");

	gKernelArgs.arch_args.pgtables[gKernelArgs.arch_args.num_pgtables++] = (uint32)pageTable;

	for (int32 i = 0; i < 1024; i++)
		pageTable[i] = 0;

	// put the new page table into the page directory
	gPageRoot[base/(4*1024*1024)] = (uint32)pageTable | kDefaultPageTableFlags;
#endif
}
#endif


static void
unmap_page(addr_t virtualAddress)
{
	gMMUOps->unmap_page(virtualAddress);
}


/** Creates an entry to map the specified virtualAddress to the given
 *	physicalAddress.
 *	If the mapping goes beyond the current page table, it will allocate
 *	a new one. If it cannot map the requested page, it panics.
 */

static void
map_page(addr_t virtualAddress, addr_t physicalAddress, uint32 flags)
{
	TRACE(("map_page: vaddr 0x%lx, paddr 0x%lx\n", virtualAddress, physicalAddress));

	if (virtualAddress < KERNEL_BASE)
		panic("map_page: asked to map invalid page %p!\n", (void *)virtualAddress);

	// slow but I'm too lazy to fix the code below
	gMMUOps->add_page_table(virtualAddress);
#if 0
	if (virtualAddress >= sMaxVirtualAddress) {
		// we need to add a new page table

		gMMUOps->add_page_table(sMaxVirtualAddress);
		// 64 pages / page table
		sMaxVirtualAddress += B_PAGE_SIZE * 64;

		if (virtualAddress >= sMaxVirtualAddress)
			panic("map_page: asked to map a page to %p\n", (void *)virtualAddress);
	}
#endif

	physicalAddress &= ~(B_PAGE_SIZE - 1);

	// map the page to the correct page table
	gMMUOps->map_page(virtualAddress, physicalAddress, flags);
}


static void
init_page_directory(void)
{
	TRACE(("init_page_directory\n"));

	// allocate a new pg root dir
	gPageRoot = get_next_physical_page();
	gKernelArgs.arch_args.phys_pgroot = (uint32)gPageRoot;
	gKernelArgs.arch_args.phys_vbr = (uint32)gPageRoot + VBR_PAGE_OFFSET;

	// set the root pointers
	gMMUOps->load_rp(gPageRoot);
	// allocate second level tables for kernel space
	// this will simplify mmu code a lot, and only wastes 32KB
	gMMUOps->allocate_kernel_pgdirs();
	// enable mmu translation
	gMMUOps->enable_paging();
	//XXX: check for errors

	//gKernelArgs.arch_args.num_pgtables = 0;
	gMMUOps->add_page_table(KERNEL_BASE);

#if 0


	// clear out the pgdir
	for (int32 i = 0; i < 1024; i++) {
		gPageRoot[i] = 0;
	}

	// Identity map the first 8 MB of memory so that their
	// physical and virtual address are the same.
	// These page tables won't be taken over into the kernel.

	// make the first page table at the first free spot
	uint32 *pageTable = mmu_get_next_page_tables();

	for (int32 i = 0; i < 1024; i++) {
		pageTable[i] = (i * 0x1000) | kDefaultPageFlags;
	}

	gPageRoot[0] = (uint32)pageTable | kDefaultPageFlags;

	// make the second page table
	pageTable = mmu_get_next_page_tables();

	for (int32 i = 0; i < 1024; i++) {
		pageTable[i] = (i * 0x1000 + 0x400000) | kDefaultPageFlags;
	}

	gPageRoot[1] = (uint32)pageTable | kDefaultPageFlags;

	gKernelArgs.arch_args.num_pgtables = 0;
	add_page_table(KERNEL_BASE);

	// switch to the new pgdir and enable paging
	asm("movl %0, %%eax;"
		"movl %%eax, %%cr3;" : : "m" (gPageRoot) : "eax");
	// Important.  Make sure supervisor threads can fault on read only pages...
	asm("movl %%eax, %%cr0" : : "a" ((1 << 31) | (1 << 16) | (1 << 5) | 1));
#endif
}


//	#pragma mark -


extern "C" addr_t
mmu_map_physical_memory(addr_t physicalAddress, size_t size, uint32 flags)
{
	addr_t address = sNextVirtualAddress;
	addr_t pageOffset = physicalAddress & (B_PAGE_SIZE - 1);

	physicalAddress -= pageOffset;

	for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
		map_page(get_next_virtual_page(), physicalAddress + offset, flags);
	}

	return address + pageOffset;
}


extern "C" void *
mmu_allocate(void *virtualAddress, size_t size)
{
	TRACE(("mmu_allocate: requested vaddr: %p, next free vaddr: 0x%lx, size: %ld\n",
		virtualAddress, sNextVirtualAddress, size));

	size = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
		// get number of pages to map

	if (virtualAddress != NULL) {
		// This special path is almost only useful for loading the
		// kernel into memory; it will only allow you to map the
		// 1 MB following the kernel base address.
		// Also, it won't check for already mapped addresses, so
		// you better know why you are here :)
		addr_t address = (addr_t)virtualAddress;

		// is the address within the valid range?
		if (address < KERNEL_BASE || address + size >= KERNEL_BASE + kMaxKernelSize)
			return NULL;

		for (uint32 i = 0; i < size; i++) {
			map_page(address, get_next_physical_page(), kDefaultPageFlags);
			address += B_PAGE_SIZE;
		}

		TRACE(("mmu_allocate(KERNEL, %d): done\n", size));
		return virtualAddress;
	}

	void *address = (void *)sNextVirtualAddress;

	for (uint32 i = 0; i < size; i++) {
		map_page(get_next_virtual_page(), get_next_physical_page(), kDefaultPageFlags);
	}

	TRACE(("mmu_allocate(NULL, %d): %p\n", size, address));
	return address;
}


/**	This will unmap the allocated chunk of memory from the virtual
 *	address space. It might not actually free memory (as its implementation
 *	is very simple), but it might.
 */

extern "C" void
mmu_free(void *virtualAddress, size_t size)
{
	TRACE(("mmu_free(virtualAddress = %p, size: %ld)\n", virtualAddress, size));

	addr_t address = (addr_t)virtualAddress;
	size = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
		// get number of pages to map

	// is the address within the valid range?
	if (address < KERNEL_BASE
		|| address + size >= KERNEL_BASE + kMaxKernelSize) {
		panic("mmu_free: asked to unmap out of range region (%p, size %lx)\n",
			(void *)address, size);
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


/** Sets up the final and kernel accessible GDT and IDT tables.
 *	BIOS calls won't work any longer after this function has
 *	been called.
 */

extern "C" void
mmu_init_for_kernel(void)
{
	TRACE(("mmu_init_for_kernel\n"));




	// remove identity mapping of ST space
	// actually done by the kernel when it's done using query_early
	//gMMUOps->set_tt(0, NULL, 0, 0);

#if 0
	// set up a new idt
	{
		struct gdt_idt_descr idtDescriptor;
		uint32 *idt;

		// find a new idt
		idt = (uint32 *)get_next_physical_page();
		gKernelArgs.arch_args.phys_idt = (uint32)idt;

		TRACE(("idt at %p\n", idt));

		// map the idt into virtual space
		gKernelArgs.arch_args.vir_idt = (uint32)get_next_virtual_page();
		map_page(gKernelArgs.arch_args.vir_idt, (uint32)idt, kDefaultPageFlags);

		// clear it out
		uint32* virtualIDT = (uint32*)gKernelArgs.arch_args.vir_idt;
		for (int32 i = 0; i < IDT_LIMIT / 4; i++) {
			virtualIDT[i] = 0;
		}

		// load the idt
		idtDescriptor.limit = IDT_LIMIT - 1;
		idtDescriptor.base = (uint32 *)gKernelArgs.arch_args.vir_idt;

		asm("lidt	%0;"
			: : "m" (idtDescriptor));

		TRACE(("idt at virtual address 0x%lx\n", gKernelArgs.arch_args.vir_idt));
	}

	// set up a new gdt
	{
		struct gdt_idt_descr gdtDescriptor;
		segment_descriptor *gdt;

		// find a new gdt
		gdt = (segment_descriptor *)get_next_physical_page();
		gKernelArgs.arch_args.phys_gdt = (uint32)gdt;

		TRACE(("gdt at %p\n", gdt));

		// map the gdt into virtual space
		gKernelArgs.arch_args.vir_gdt = (uint32)get_next_virtual_page();
		map_page(gKernelArgs.arch_args.vir_gdt, (uint32)gdt, kDefaultPageFlags);

		// put standard segment descriptors in it
		segment_descriptor* virtualGDT
			= (segment_descriptor*)gKernelArgs.arch_args.vir_gdt;
		clear_segment_descriptor(&virtualGDT[0]);

		// seg 0x08 - kernel 4GB code
		set_segment_descriptor(&virtualGDT[1], 0, 0xffffffff, DT_CODE_READABLE,
			DPL_KERNEL);

		// seg 0x10 - kernel 4GB data
		set_segment_descriptor(&virtualGDT[2], 0, 0xffffffff, DT_DATA_WRITEABLE,
			DPL_KERNEL);

		// seg 0x1b - ring 3 user 4GB code
		set_segment_descriptor(&virtualGDT[3], 0, 0xffffffff, DT_CODE_READABLE,
			DPL_USER);

		// seg 0x23 - ring 3 user 4GB data
		set_segment_descriptor(&virtualGDT[4], 0, 0xffffffff, DT_DATA_WRITEABLE,
			DPL_USER);

		// virtualGDT[5] and above will be filled later by the kernel
		// to contain the TSS descriptors, and for TLS (one for every CPU)

		// load the GDT
		gdtDescriptor.limit = GDT_LIMIT - 1;
		gdtDescriptor.base = (uint32 *)gKernelArgs.arch_args.vir_gdt;

		asm("lgdt	%0;"
			: : "m" (gdtDescriptor));

		TRACE(("gdt at virtual address %p\n", (void *)gKernelArgs.arch_args.vir_gdt));
	}
#endif

	// save the memory we've physically allocated
	gKernelArgs.physical_allocated_range[0].size = sNextPhysicalAddress - gKernelArgs.physical_allocated_range[0].start;

	// save the memory we've virtually allocated (for the kernel and other stuff)
	gKernelArgs.virtual_allocated_range[0].start = KERNEL_BASE;
	gKernelArgs.virtual_allocated_range[0].size = sNextVirtualAddress - KERNEL_BASE;
	gKernelArgs.num_virtual_allocated_ranges = 1;

	// sort the address ranges
	sort_address_ranges(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges);
	sort_address_ranges(gKernelArgs.physical_allocated_range,
		gKernelArgs.num_physical_allocated_ranges);
	sort_address_ranges(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges);

#ifdef TRACE_MMU
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


extern "C" void
mmu_init(void)
{
	TRACE(("mmu_init\n"));
	switch (gKernelArgs.arch_args.mmu_type) {
#if 0
		case 68851:
			gMMUOps = &k851MMUOps;
			break;
#endif
		case 68030:
			gMMUOps = &k030MMUOps;
			break;
		case 68040:
			gMMUOps = &k040MMUOps;
			break;
#if 0
		case 68060:
			gMMUOps = &k060MMUOps;
			break;
#endif
		default:
			panic("unknown mmu type %d\n", gKernelArgs.arch_args.mmu_type);
	}

	gMMUOps->initialize();

#if 0
	addr_t fastram_top = 0;
	if (*TOSVARramvalid == TOSVARramvalid_MAGIC)
		fastram_top = *TOSVARramtop;
	if (fastram_top) {
		// we have some fastram, use it first
		sNextPhysicalAddress = AMIGA_FASTRAM_BASE;
	}

	gKernelArgs.physical_allocated_range[0].start = sNextPhysicalAddress;
	gKernelArgs.physical_allocated_range[0].size = 0;
	gKernelArgs.num_physical_allocated_ranges = 1;
		// remember the start of the allocated physical pages

	// enable transparent translation of the first 256 MB
	gMMUOps->set_tt(0, AMIGA_CHIPRAM_BASE, 0x10000000, 0);
	// enable transparent translation of the 16MB ST shadow range for I/O
	gMMUOps->set_tt(1, AMIGA_SHADOW_BASE, 0x01000000, 0);

	init_page_directory();
#if 0//XXX:HOLE

	// Map the page directory into kernel space at 0xffc00000-0xffffffff
	// this enables a mmu trick where the 4 MB region that this pgdir entry
	// represents now maps the 4MB of potential pagetables that the pgdir
	// points to. Thrown away later in VM bringup, but useful for now.
	gPageRoot[1023] = (uint32)gPageRoot | kDefaultPageFlags;
#endif

	// also map it on the next vpage
	gKernelArgs.arch_args.vir_pgroot = get_next_virtual_page();
	map_page(gKernelArgs.arch_args.vir_pgroot, (uint32)gPageRoot, kDefaultPageFlags);

	// set virtual addr for interrupt vector table
	gKernelArgs.arch_args.vir_vbr = gKernelArgs.arch_args.vir_pgroot
		+ VBR_PAGE_OFFSET;

	// map in a kernel stack
	gKernelArgs.cpu_kstack[0].start = (addr_t)mmu_allocate(NULL,
		KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE);
	gKernelArgs.cpu_kstack[0].size = KERNEL_STACK_SIZE
		+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;

	TRACE(("kernel stack at 0x%lx to 0x%lx\n", gKernelArgs.cpu_kstack[0].start,
		gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size));

	// st ram as 1st range
	gKernelArgs.physical_memory_range[0].start = AMIGA_CHIPRAM_BASE;
	gKernelArgs.physical_memory_range[0].size = *TOSVARphystop - AMIGA_CHIPRAM_BASE;
	gKernelArgs.num_physical_memory_ranges = 1;

	// fast ram as 2nd range
	if (fastram_top) {
		gKernelArgs.physical_memory_range[1].start =
			AMIGA_FASTRAM_BASE;
		gKernelArgs.physical_memory_range[1].size =
			fastram_top - AMIGA_FASTRAM_BASE;
		gKernelArgs.num_physical_memory_ranges++;

	}

	// mark the video area allocated
	addr_t video_base = *TOSVAR_memtop;
	video_base &= ~(B_PAGE_SIZE-1);
	gKernelArgs.physical_allocated_range[gKernelArgs.num_physical_allocated_ranges].start = video_base;
	gKernelArgs.physical_allocated_range[gKernelArgs.num_physical_allocated_ranges].size = *TOSVARphystop - video_base;
	gKernelArgs.num_physical_allocated_ranges++;

#endif
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


