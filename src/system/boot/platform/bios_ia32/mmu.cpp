/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Based on code written by Travis Geiselbrecht for NewOS.
 *
 * Distributed under the terms of the MIT License.
 */


#include "mmu.h"

#include <string.h>

#include <OS.h>

#include <arch/cpu.h>
#include <arch_kernel.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <kernel.h>

#include "bios.h"
#include "interrupts.h"


/*!	The (physical) memory layout of the boot loader is currently as follows:
	  0x0500 - 0x10000	protected mode stack
	  0x0500 - 0x09000	real mode stack
	 0x10000 - ?		code (up to ~500 kB)
	 0x90000			1st temporary page table (identity maps 0-4 MB)
	 0x91000			2nd (4-8 MB)
	 0x92000 - 0x92000	further page tables
	 0x9e000 - 0xa0000	SMP trampoline code
	[0xa0000 - 0x100000	BIOS/ROM/reserved area]
	0x100000			page directory
	     ...			boot loader heap (32 kB)
	     ...			free physical memory

	The first 8 MB are identity mapped (0x0 - 0x0800000); paging is turned
	on. The kernel is mapped at 0x80000000, all other stuff mapped by the
	loader (kernel args, modules, driver settings, ...) comes after
	0x80020000 which means that there is currently only 2 MB reserved for
	the kernel itself (see kMaxKernelSize).

	The layout in PXE mode differs a bit from this, see definitions below.
*/

//#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


//#define TRACE_MEMORY_MAP
	// Define this to print the memory map to serial debug,
	// You also need to define ENABLE_SERIAL in serial.cpp
	// for output to work.


// memory structure returned by int 0x15, ax 0xe820
struct extended_memory {
	uint64 base_addr;
	uint64 length;
	uint32 type;
};


static const uint32 kDefaultPageTableFlags = 0x07;	// present, user, R/W
static const size_t kMaxKernelSize = 0x1000000;		// 16 MB for the kernel

// working page directory and page table
static uint32 *sPageDirectory = 0;

#ifdef _PXE_ENV

static addr_t sNextPhysicalAddress = 0x112000;
static addr_t sNextVirtualAddress = KERNEL_BASE + kMaxKernelSize;

static addr_t sNextPageTableAddress = 0x7d000;
static const uint32 kPageTableRegionEnd = 0x8b000;
	// we need to reserve 2 pages for the SMP trampoline code

#else

static addr_t sNextPhysicalAddress = 0x100000;
static addr_t sNextVirtualAddress = KERNEL_BASE + kMaxKernelSize;

static addr_t sNextPageTableAddress = 0x90000;
static const uint32 kPageTableRegionEnd = 0x9e000;
	// we need to reserve 2 pages for the SMP trampoline code

#endif


static addr_t
get_next_virtual_address(size_t size)
{
	addr_t address = sNextVirtualAddress;
	sNextVirtualAddress += size;

	return address;
}


static addr_t
get_next_physical_address(size_t size)
{
	uint64 base;
	if (!get_free_address_range(gKernelArgs.physical_allocated_range,
			gKernelArgs.num_physical_allocated_ranges, sNextPhysicalAddress,
			size, &base)) {
		panic("Out of physical memory!");
		return 0;
	}

	insert_physical_allocated_range(base, size);
	sNextPhysicalAddress = base + size;
		// TODO: Can overflow theoretically.

	return base;
}


static addr_t
get_next_virtual_page()
{
	return get_next_virtual_address(B_PAGE_SIZE);
}


static addr_t
get_next_physical_page()
{
	return get_next_physical_address(B_PAGE_SIZE);
}


static uint32 *
get_next_page_table()
{
	TRACE("get_next_page_table, sNextPageTableAddress %#" B_PRIxADDR
		", kPageTableRegionEnd %#" B_PRIxADDR "\n", sNextPageTableAddress,
		kPageTableRegionEnd);

	addr_t address = sNextPageTableAddress;
	if (address >= kPageTableRegionEnd)
		return (uint32 *)get_next_physical_page();

	sNextPageTableAddress += B_PAGE_SIZE;
	return (uint32 *)address;
}


/*!	Adds a new page table for the specified base address */
static uint32*
add_page_table(addr_t base)
{
	base = ROUNDDOWN(base, B_PAGE_SIZE * 1024);

	// Get new page table and clear it out
	uint32 *pageTable = get_next_page_table();
	if (pageTable > (uint32 *)(8 * 1024 * 1024)) {
		panic("tried to add page table beyond the identity mapped 8 MB "
			"region\n");
		return NULL;
	}

	TRACE("add_page_table(base = %p), got page: %p\n", (void*)base, pageTable);

	gKernelArgs.arch_args.pgtables[gKernelArgs.arch_args.num_pgtables++]
		= (uint32)pageTable;

	for (int32 i = 0; i < 1024; i++)
		pageTable[i] = 0;

	// put the new page table into the page directory
	sPageDirectory[base / (4 * 1024 * 1024)]
		= (uint32)pageTable | kDefaultPageTableFlags;

	// update the virtual end address in the kernel args
	base += B_PAGE_SIZE * 1024;
	if (base > gKernelArgs.arch_args.virtual_end)
		gKernelArgs.arch_args.virtual_end = base;

	return pageTable;
}


static void
unmap_page(addr_t virtualAddress)
{
	TRACE("unmap_page(virtualAddress = %p)\n", (void *)virtualAddress);

	if (virtualAddress < KERNEL_BASE) {
		panic("unmap_page: asked to unmap invalid page %p!\n",
			(void *)virtualAddress);
	}

	// unmap the page from the correct page table
	uint32 *pageTable = (uint32 *)(sPageDirectory[virtualAddress
		/ (B_PAGE_SIZE * 1024)] & 0xfffff000);
	pageTable[(virtualAddress % (B_PAGE_SIZE * 1024)) / B_PAGE_SIZE] = 0;

	asm volatile("invlpg (%0)" : : "r" (virtualAddress));
}


/*!	Creates an entry to map the specified virtualAddress to the given
	physicalAddress.
	If the mapping goes beyond the current page table, it will allocate
	a new one. If it cannot map the requested page, it panics.
*/
static void
map_page(addr_t virtualAddress, addr_t physicalAddress, uint32 flags)
{
	TRACE("map_page: vaddr 0x%lx, paddr 0x%lx\n", virtualAddress,
		physicalAddress);

	if (virtualAddress < KERNEL_BASE) {
		panic("map_page: asked to map invalid page %p!\n",
			(void *)virtualAddress);
	}

	uint32 *pageTable = (uint32 *)(sPageDirectory[virtualAddress
		/ (B_PAGE_SIZE * 1024)] & 0xfffff000);

	if (pageTable == NULL) {
		// we need to add a new page table
		pageTable = add_page_table(virtualAddress);

		if (pageTable == NULL) {
			panic("map_page: failed to allocate a page table for virtual "
				"address %p\n", (void*)virtualAddress);
			return;
		}
	}

	physicalAddress &= ~(B_PAGE_SIZE - 1);

	// map the page to the correct page table
	uint32 tableEntry = (virtualAddress % (B_PAGE_SIZE * 1024)) / B_PAGE_SIZE;

	TRACE("map_page: inserting pageTable %p, tableEntry %" B_PRIu32
		", physicalAddress %#" B_PRIxADDR "\n", pageTable, tableEntry,
		physicalAddress);

	pageTable[tableEntry] = physicalAddress | flags;

	asm volatile("invlpg (%0)" : : "r" (virtualAddress));

	TRACE("map_page: done\n");
}


#ifdef TRACE_MEMORY_MAP
static const char *
e820_memory_type(uint32 type)
{
	switch (type) {
		case 1: return "memory";
		case 2: return "reserved";
		case 3: return "ACPI reclaim";
		case 4: return "ACPI NVS";
		default: return "unknown/reserved";
	}
}
#endif


static uint32
get_memory_map(extended_memory **_extendedMemory)
{
	extended_memory *block = (extended_memory *)kExtraSegmentScratch;
	bios_regs regs = {0, 0, sizeof(extended_memory), 0, 0, (uint32)block, 0, 0};
	uint32 count = 0;

	TRACE("get_memory_map()\n");

	do {
		regs.eax = 0xe820;
		regs.edx = 'SMAP';

		call_bios(0x15, &regs);
		if ((regs.flags & CARRY_FLAG) != 0)
			return 0;

		regs.edi += sizeof(extended_memory);
		count++;
	} while (regs.ebx != 0);

	*_extendedMemory = block;

#ifdef TRACE_MEMORY_MAP
	dprintf("extended memory info (from 0xe820):\n");
	for (uint32 i = 0; i < count; i++) {
		dprintf("    base 0x%08Lx, len 0x%08Lx, type %lu (%s)\n",
			block[i].base_addr, block[i].length,
			block[i].type, e820_memory_type(block[i].type));
	}
#endif

	return count;
}


static void
init_page_directory(void)
{
	TRACE("init_page_directory\n");

	// allocate a new pgdir
	sPageDirectory = (uint32 *)get_next_physical_page();
	gKernelArgs.arch_args.phys_pgdir = (uint32)sPageDirectory;

	// clear out the pgdir
	for (int32 i = 0; i < 1024; i++) {
		sPageDirectory[i] = 0;
	}

	// Identity map the first 8 MB of memory so that their
	// physical and virtual address are the same.
	// These page tables won't be taken over into the kernel.

	// make the first page table at the first free spot
	uint32 *pageTable = get_next_page_table();

	for (int32 i = 0; i < 1024; i++) {
		pageTable[i] = (i * 0x1000) | kDefaultPageFlags;
	}

	sPageDirectory[0] = (uint32)pageTable | kDefaultPageFlags;

	// make the second page table
	pageTable = get_next_page_table();

	for (int32 i = 0; i < 1024; i++) {
		pageTable[i] = (i * 0x1000 + 0x400000) | kDefaultPageFlags;
	}

	sPageDirectory[1] = (uint32)pageTable | kDefaultPageFlags;

	gKernelArgs.arch_args.num_pgtables = 0;

	// switch to the new pgdir and enable paging
	asm("movl %0, %%eax;"
		"movl %%eax, %%cr3;" : : "m" (sPageDirectory) : "eax");
	// Important.  Make sure supervisor threads can fault on read only pages...
	asm("movl %%eax, %%cr0" : : "a" ((1 << 31) | (1 << 16) | (1 << 5) | 1));
}


//	#pragma mark -


/*!
	Neither \a virtualAddress nor \a size need to be aligned, but the function
	will map all pages the range intersects with.
	If physicalAddress is not page-aligned, the returned virtual address will
	have the same "misalignment".
*/
extern "C" addr_t
mmu_map_physical_memory(addr_t physicalAddress, size_t size, uint32 flags)
{
	addr_t address = sNextVirtualAddress;
	addr_t pageOffset = physicalAddress & (B_PAGE_SIZE - 1);

	physicalAddress -= pageOffset;
	size += pageOffset;

	for (addr_t offset = 0; offset < size; offset += B_PAGE_SIZE) {
		map_page(get_next_virtual_page(), physicalAddress + offset, flags);
	}

	return address + pageOffset;
}


extern "C" void *
mmu_allocate(void *virtualAddress, size_t size)
{
	TRACE("mmu_allocate: requested vaddr: %p, next free vaddr: 0x%lx, size: "
		"%ld\n", virtualAddress, sNextVirtualAddress, size);

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
			|| address + size >= KERNEL_BASE + kMaxKernelSize)
			return NULL;

		for (uint32 i = 0; i < size; i++) {
			map_page(address, get_next_physical_page(), kDefaultPageFlags);
			address += B_PAGE_SIZE;
		}

		return virtualAddress;
	}

	void *address = (void *)sNextVirtualAddress;

	for (uint32 i = 0; i < size; i++) {
		map_page(get_next_virtual_page(), get_next_physical_page(),
			kDefaultPageFlags);
	}

	return address;
}


/*!	Allocates a single page and returns both its virtual and physical
	addresses.
*/
void *
mmu_allocate_page(addr_t *_physicalAddress)
{
	addr_t virt = get_next_virtual_page();
	addr_t phys = get_next_physical_page();

	map_page(virt, phys, kDefaultPageFlags);

	if (_physicalAddress)
		*_physicalAddress = phys;

	return (void *)virt;
}


/*!	Allocates the given physical range.
	\return \c true, if the range could be allocated, \c false otherwise.
*/
bool
mmu_allocate_physical(addr_t base, size_t size)
{
	// check whether the physical memory range exists at all
	if (!is_address_range_covered(gKernelArgs.physical_memory_range,
			gKernelArgs.num_physical_memory_ranges, base, size)) {
		return false;
	}

	// check whether the physical range is still free
	uint64 foundBase;
	if (!get_free_address_range(gKernelArgs.physical_allocated_range,
			gKernelArgs.num_physical_allocated_ranges, base, size, &foundBase)
		|| foundBase != base) {
		return false;
	}

	return insert_physical_allocated_range(base, size) == B_OK;
}


/*!	This will unmap the allocated chunk of memory from the virtual
	address space. It might not actually free memory (as its implementation
	is very simple), but it might.
	Neither \a virtualAddress nor \a size need to be aligned, but the function
	will unmap all pages the range intersects with.
*/
extern "C" void
mmu_free(void *virtualAddress, size_t size)
{
	TRACE("mmu_free(virtualAddress = %p, size: %ld)\n", virtualAddress, size);

	addr_t address = (addr_t)virtualAddress;
	addr_t pageOffset = address % B_PAGE_SIZE;
	address -= pageOffset;
	size = (size + pageOffset + B_PAGE_SIZE - 1) / B_PAGE_SIZE * B_PAGE_SIZE;

	// is the address within the valid range?
	if (address < KERNEL_BASE || address + size > sNextVirtualAddress) {
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


size_t
mmu_get_virtual_usage()
{
	return sNextVirtualAddress - KERNEL_BASE;
}


bool
mmu_get_virtual_mapping(addr_t virtualAddress, addr_t *_physicalAddress)
{
	if (virtualAddress < KERNEL_BASE) {
		panic("mmu_get_virtual_mapping: asked to lookup invalid page %p!\n",
			(void *)virtualAddress);
	}

	uint32 *pageTable = (uint32 *)(sPageDirectory[virtualAddress
		/ (B_PAGE_SIZE * 1024)] & 0xfffff000);
	uint32 tableEntry = pageTable[(virtualAddress % (B_PAGE_SIZE * 1024))
		/ B_PAGE_SIZE];

	if ((tableEntry & (1<<0)) != 0) {
		*_physicalAddress = tableEntry & 0xFFFFF000;
		return true;
	} else {
		return false;
	}
}


/*!	Sets up the final and kernel accessible GDT and IDT tables.
	BIOS calls won't work any longer after this function has
	been called.
*/
extern "C" void
mmu_init_for_kernel(void)
{
	TRACE("mmu_init_for_kernel\n");
	// set up a new idt
	{
		uint32 *idt;

		// find a new idt
		idt = (uint32 *)get_next_physical_page();
		gKernelArgs.arch_args.phys_idt = (uint32)idt;

		TRACE("idt at %p\n", idt);

		// map the idt into virtual space
		gKernelArgs.arch_args.vir_idt = (uint32)get_next_virtual_page();
		map_page(gKernelArgs.arch_args.vir_idt, (uint32)idt, kDefaultPageFlags);

		// initialize it
		interrupts_init_kernel_idt((void*)(addr_t)gKernelArgs.arch_args.vir_idt,
			IDT_LIMIT);

		TRACE("idt at virtual address 0x%llx\n", gKernelArgs.arch_args.vir_idt);
	}

	// set up a new gdt
	{
		struct gdt_idt_descr gdtDescriptor;
		segment_descriptor *gdt;

		// find a new gdt
		gdt = (segment_descriptor *)get_next_physical_page();
		gKernelArgs.arch_args.phys_gdt = (uint32)gdt;

		TRACE("gdt at %p\n", gdt);

		// map the gdt into virtual space
		gKernelArgs.arch_args.vir_gdt = (uint32)get_next_virtual_page();
		map_page(gKernelArgs.arch_args.vir_gdt, (uint32)gdt, kDefaultPageFlags);

		// put standard segment descriptors in it
		segment_descriptor* virtualGDT
			= (segment_descriptor*)(addr_t)gKernelArgs.arch_args.vir_gdt;
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
		gdtDescriptor.base = (void*)(addr_t)gKernelArgs.arch_args.vir_gdt;

		asm("lgdt	%0;"
			: : "m" (gdtDescriptor));

		TRACE("gdt at virtual address %p\n",
			(void*)gKernelArgs.arch_args.vir_gdt);
	}

	// Save the memory we've virtually allocated (for the kernel and other
	// stuff)
	gKernelArgs.virtual_allocated_range[0].start = KERNEL_BASE;
	gKernelArgs.virtual_allocated_range[0].size
		= sNextVirtualAddress - KERNEL_BASE;
	gKernelArgs.num_virtual_allocated_ranges = 1;

	// sort the address ranges
	sort_address_ranges(gKernelArgs.physical_memory_range,
		gKernelArgs.num_physical_memory_ranges);
	sort_address_ranges(gKernelArgs.physical_allocated_range,
		gKernelArgs.num_physical_allocated_ranges);
	sort_address_ranges(gKernelArgs.virtual_allocated_range,
		gKernelArgs.num_virtual_allocated_ranges);

#ifdef TRACE_MEMORY_MAP
	{
		uint32 i;

		dprintf("phys memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_physical_memory_ranges; i++) {
			dprintf("    base %#018" B_PRIx64 ", length %#018" B_PRIx64 "\n",
				gKernelArgs.physical_memory_range[i].start,
				gKernelArgs.physical_memory_range[i].size);
		}

		dprintf("allocated phys memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_physical_allocated_ranges; i++) {
			dprintf("    base %#018" B_PRIx64 ", length %#018" B_PRIx64 "\n",
				gKernelArgs.physical_allocated_range[i].start,
				gKernelArgs.physical_allocated_range[i].size);
		}

		dprintf("allocated virt memory ranges:\n");
		for (i = 0; i < gKernelArgs.num_virtual_allocated_ranges; i++) {
			dprintf("    base %#018" B_PRIx64 ", length %#018" B_PRIx64 "\n",
				gKernelArgs.virtual_allocated_range[i].start,
				gKernelArgs.virtual_allocated_range[i].size);
		}
	}
#endif
}


extern "C" void
mmu_init(void)
{
	TRACE("mmu_init\n");

	gKernelArgs.arch_args.virtual_end = KERNEL_BASE;

	gKernelArgs.physical_allocated_range[0].start = sNextPhysicalAddress;
	gKernelArgs.physical_allocated_range[0].size = 0;
	gKernelArgs.num_physical_allocated_ranges = 1;
		// remember the start of the allocated physical pages

	init_page_directory();

	// Map the page directory into kernel space at 0xffc00000-0xffffffff
	// this enables a mmu trick where the 4 MB region that this pgdir entry
	// represents now maps the 4MB of potential pagetables that the pgdir
	// points to. Thrown away later in VM bringup, but useful for now.
	sPageDirectory[1023] = (uint32)sPageDirectory | kDefaultPageFlags;

	// also map it on the next vpage
	gKernelArgs.arch_args.vir_pgdir = get_next_virtual_page();
	map_page(gKernelArgs.arch_args.vir_pgdir, (uint32)sPageDirectory,
		kDefaultPageFlags);

	// map in a kernel stack
	gKernelArgs.cpu_kstack[0].start = (addr_t)mmu_allocate(NULL,
		KERNEL_STACK_SIZE + KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE);
	gKernelArgs.cpu_kstack[0].size = KERNEL_STACK_SIZE
		+ KERNEL_STACK_GUARD_PAGES * B_PAGE_SIZE;

	TRACE("kernel stack at 0x%lx to 0x%lx\n", gKernelArgs.cpu_kstack[0].start,
		gKernelArgs.cpu_kstack[0].start + gKernelArgs.cpu_kstack[0].size);

	extended_memory *extMemoryBlock;
	uint32 extMemoryCount = get_memory_map(&extMemoryBlock);

	// figure out the memory map
	if (extMemoryCount > 0) {
		gKernelArgs.num_physical_memory_ranges = 0;

		for (uint32 i = 0; i < extMemoryCount; i++) {
			// Type 1 is available memory
			if (extMemoryBlock[i].type == 1) {
				uint64 base = extMemoryBlock[i].base_addr;
				uint64 length = extMemoryBlock[i].length;
				uint64 end = base + length;

				// round everything up to page boundaries, exclusive of pages
				// it partially occupies
				base = ROUNDUP(base, B_PAGE_SIZE);
				end = ROUNDDOWN(end, B_PAGE_SIZE);

				// We ignore all memory beyond 4 GB, if phys_addr_t is only
				// 32 bit wide.
				#if B_HAIKU_PHYSICAL_BITS == 32
					if (end > 0x100000000ULL)
						end = 0x100000000ULL;
				#endif

				// Also ignore memory below 1 MB. Apparently some BIOSes fail to
				// provide the correct range type for some ranges (cf. #1925).
				// Later in the kernel we will reserve the range 0x0 - 0xa0000
				// and apparently 0xa0000 - 0x100000 never contain usable
				// memory, so we don't lose anything by doing that.
				if (base < 0x100000)
					base = 0x100000;

				gKernelArgs.ignored_physical_memory
					+= length - (max_c(end, base) - base);

				if (end <= base)
					continue;

				status_t status = insert_physical_memory_range(base, end - base);
				if (status == B_ENTRY_NOT_FOUND) {
					panic("mmu_init(): Failed to add physical memory range "
						"%#" B_PRIx64 " - %#" B_PRIx64 " : all %d entries are "
						"used already!\n", base, end, MAX_PHYSICAL_MEMORY_RANGE);
				} else if (status != B_OK) {
					panic("mmu_init(): Failed to add physical memory range "
						"%#" B_PRIx64 " - %#" B_PRIx64 "\n", base, end);
				}
			} else if (extMemoryBlock[i].type == 3) {
				// ACPI reclaim -- physical memory we could actually use later
				gKernelArgs.ignored_physical_memory += extMemoryBlock[i].length;
			}
		}

		// sort the ranges
		sort_address_ranges(gKernelArgs.physical_memory_range,
			gKernelArgs.num_physical_memory_ranges);

		// On some machines we get several ranges that contain only a few pages
		// (or even only one) each, which causes us to run out of MTRRs later.
		// So we remove all ranges smaller than 64 KB, hoping that this will
		// leave us only with a few larger contiguous ranges (ideally one).
		for (int32 i = gKernelArgs.num_physical_memory_ranges - 1; i >= 0;
				i--) {
			uint64 size = gKernelArgs.physical_memory_range[i].size;
			if (size < 64 * 1024) {
				uint64 start = gKernelArgs.physical_memory_range[i].start;
				remove_address_range(gKernelArgs.physical_memory_range,
					&gKernelArgs.num_physical_memory_ranges,
					MAX_PHYSICAL_MEMORY_RANGE, start, size);
			}
		}
	} else {
		bios_regs regs;

		// We dont have an extended map, assume memory is contiguously mapped
		// at 0x0, but leave out the BIOS range ((640k - 1 page) to 1 MB).
		gKernelArgs.physical_memory_range[0].start = 0;
		gKernelArgs.physical_memory_range[0].size = 0x9f000;
		gKernelArgs.physical_memory_range[1].start = 0x100000;

		regs.eax = 0xe801; // AX
		call_bios(0x15, &regs);
		if ((regs.flags & CARRY_FLAG) != 0) {
			regs.eax = 0x8800; // AH 88h
			call_bios(0x15, &regs);
			if ((regs.flags & CARRY_FLAG) != 0) {
				// TODO: for now!
				dprintf("No memory size - using 64 MB (fix me!)\n");
				uint32 memSize = 64 * 1024 * 1024;
				gKernelArgs.physical_memory_range[1].size = memSize - 0x100000;
			} else {
				dprintf("Get Extended Memory Size succeeded.\n");
				gKernelArgs.physical_memory_range[1].size = regs.eax * 1024;
			}
			gKernelArgs.num_physical_memory_ranges = 2;
		} else {
			dprintf("Get Memory Size for Large Configurations succeeded.\n");
			gKernelArgs.physical_memory_range[1].size = regs.ecx * 1024;
			gKernelArgs.physical_memory_range[2].start = 0x1000000;
			gKernelArgs.physical_memory_range[2].size = regs.edx * 64 * 1024;
			gKernelArgs.num_physical_memory_ranges = 3;
		}
	}

	gKernelArgs.arch_args.page_hole = 0xffc00000;
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


