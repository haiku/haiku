/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Based on code written by Travis Geiselbrecht for NewOS.
** Distributed under the terms of the OpenBeOS License.
*/


#include "mmu.h"
#include "bios.h"

#include <OS.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/kernel_args.h>
#include <arch/cpu.h>
#include <arch_kernel.h>

#include <string.h>


/** The (physical) memory layout of the boot loader is currently as follows:
 *	     0x0 - 0x10000	protected mode stack
 *	     0x0 - 0x09000	real mode stack
 *	 0x10000 - ?		code
 *	0x100000			kernel args
 *	0x101000			1st temporary page table (identity maps 0-4 MB)
 *	0x102000			2nd (4-8 MB)
 *	0x110000 -			free physical memory
 *
 *	The first 8 MB are identity mapped (0x0 - 0x0800000); paging is turned
 *	on. The kernel is mapped at 0x80000000, all other stuff mapped by the
 *	loader (kernel args, modules, driver settings, ...) comes after
 *	0x81000000 which means that there is currently only 1 MB reserved for
 *	the kernel itself.
 */

//#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x) printf x
#else
#	define TRACE(x) ;
#endif

struct gdt_idt_descr {
	uint16 limit;
	uint32 *base;
} _PACKED;

// memory structure returned by int 0x15, ax 0xe820
struct extended_memory {
	uint64 base_addr;
	uint64 length;
	uint32 type;
};

static const uint32 kDefaultPageFlags = 0x03;	// present, R/W

static kernel_args *ka = (kernel_args *)0x100000;
	// ToDo: this has to replace the gKernelArgs variable!

// working page directory and page table
static uint32 *sPageDirectory = 0;
static uint32 *sPageTable = 0;
	// this points to the most recently added kernel page table

static addr_t sNextPhysicalAddress = 0x110000;
static addr_t sNextVirtualAddress = KERNEL_BASE + 0x100000;


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
	addr_t address = sNextPhysicalAddress;
	sNextPhysicalAddress += size;

	return address;
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


/** Creates an entry to map the specified virtualAddress to the given
 *	physicalAddress.
 *	Note, it can only map the 4 meg region right after KERNEL_BASE; this
 *	could be easily fixed, though, by dynamically adding another page
 *	table, if the need arises.
 */

static void
map_page(addr_t virtualAddress, addr_t physicalAddress, uint32 flags)
{
	TRACE(("map_page: vaddr 0x%lx, paddr 0x%lx\n", virtualAddress, physicalAddress));

	if (virtualAddress < KERNEL_BASE || virtualAddress >= (KERNEL_BASE + 4096*1024))
		panic("map_page: asked to map invalid page!\n");

	physicalAddress &= ~(B_PAGE_SIZE - 1);

	TRACE(("paddr 0x%lx @ index %ld\n", physicalAddress,
		(virtualAddress % (B_PAGE_SIZE * 1024)) / B_PAGE_SIZE));
	sPageTable[(virtualAddress % (B_PAGE_SIZE * 1024)) / B_PAGE_SIZE] = physicalAddress | flags;
}


static void
sort_addr_range(addr_range *range, int count)
{
	addr_range tempRange;
	bool done;
	int i;

	do {
		done = true;
		for (i = 1; i < count; i++) {
			if (range[i].start < range[i - 1].start) {
				done = false;
				memcpy(&tempRange, &range[i], sizeof(addr_range));
				memcpy(&range[i], &range[i - 1], sizeof(addr_range));
				memcpy(&range[i - 1], &tempRange, sizeof(addr_range));
			}
		}
	} while (!done);
}


static uint32
get_memory_map(extended_memory **_extendedMemory)
{
	extended_memory *block = (extended_memory *)kExtraSegmentScratch;
	bios_regs regs = { 0, 0, sizeof(extended_memory), 0, 0, (uint32)block, 0, 0};
	uint32 count = 0;

	do {
		regs.eax = 0xe820;
		regs.edx = 'SMAP';

		call_bios(0x15, &regs);
		if (regs.flags & CARRY_FLAG)
			return 0;

		regs.edi += sizeof(extended_memory);
		count++;
	} while (regs.ebx != 0);

	*_extendedMemory = block;

	dprintf("extended memory info (from 0xe820):\n");
	for (uint32 i = 0; i < count; i++) {
		dprintf("    base 0x%Lx, len 0x%Lx, type %lu\n", 
			block[i].base_addr, block[i].length, block[i].type);
	}

	return count;
}


static void
init_page_directory()
{
	// allocate a new pgdir
	sPageDirectory = (uint32 *)get_next_physical_page();
	ka->arch_args.phys_pgdir = (uint32)sPageDirectory;

	// clear out the pgdir
	for (int32 i = 0; i < 1024; i++)
		sPageDirectory[i] = 0;

	// Identity map the first 8 MB of memory so that their
	// physical and virtual address are the same.
	// These page tables won't be taken over into the kernel.

	// make a pagetable at this random spot
	uint32 *pgtable = (uint32 *)0x101000;

	for (int32 i = 0; i < 1024; i++) {
		pgtable[i] = (i * 0x1000) | kDefaultPageFlags;
	}

	sPageDirectory[0] = (uint32)pgtable | kDefaultPageFlags;

	// make another pagetable at this random spot
	pgtable = (uint32 *)0x102000;

	for (int32 i = 0; i < 1024; i++) {
		pgtable[i] = (i * 0x1000 + 0x400000) | kDefaultPageFlags;
	}

	sPageDirectory[1] = (uint32)pgtable | kDefaultPageFlags;

	// Get new page table and clear it out
	sPageTable = (uint32 *)get_next_physical_page();
	ka->arch_args.pgtables[0] = (uint32)sPageTable;
	ka->arch_args.num_pgtables = 1;

	for (int32 i = 0; i < 1024; i++)
		sPageTable[i] = 0;

	// put the new page table into the page directory
	// this maps the kernel at KERNEL_BASE
	sPageDirectory[KERNEL_BASE/(4*1024*1024)] = (uint32)sPageTable | kDefaultPageFlags;

	// switch to the new pgdir and enable paging
	asm("movl %0, %%eax;"
		"movl %%eax, %%cr3;" : : "m" (sPageDirectory) : "eax");
	// Important.  Make sure supervisor threads can fault on read only pages...
	asm("movl %%eax, %%cr0" : : "a" ((1 << 31) | (1 << 16) | (1 << 5) | 1));
}


//	#pragma mark -


extern "C" void *
mmu_allocate(void */*virtualAddress*/, size_t size)
{
	size = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
		// get number of pages to map

	void *address = (void *)sNextVirtualAddress;

	for (uint32 i = 0; i < size; i++) {
		map_page(get_next_virtual_page(), get_next_physical_page(), kDefaultPageFlags);
	}

	return address;
}


extern "C" void
mmu_free(void *virtualAddress, size_t size)
{
	// ToDo: implement freeing a region (do we have to?)
}


/** Sets up the final and kernel accessible GDT and IDT tables.
 *	BIOS calls won't work any longer after this function has
 *	been called.
 */

extern "C" void
mmu_init_for_kernel(void)
{
	// set up a new idt
	{
		struct gdt_idt_descr idtDescriptor;
		uint32 *idt;

		// find a new idt
		idt = (uint32 *)get_next_physical_page();
		ka->arch_args.phys_idt = (uint32)idt;

		TRACE(("idt at %p\n", idt));

		// clear it out
		for (int32 i = 0; i < IDT_LIMIT / 4; i++) {
			idt[i] = 0;
		}

		// map the idt into virtual space
		ka->arch_args.vir_idt = (uint32)get_next_virtual_page();
		map_page(ka->arch_args.vir_idt, (uint32)idt, kDefaultPageFlags);

		// load the idt
		idtDescriptor.limit = IDT_LIMIT - 1;
		idtDescriptor.base = (uint32 *)ka->arch_args.vir_idt;

		asm("lidt	%0;"
			: : "m" (idtDescriptor));

		TRACE(("idt at virtual address 0x%lx\n", ka->arch_args.vir_idt));
	}

	// set up a new gdt
	{
		struct gdt_idt_descr gdtDescriptor;
		segment_descriptor *gdt;

		// find a new gdt
		gdt = (segment_descriptor *)get_next_physical_page();
		ka->arch_args.phys_gdt = (uint32)gdt;

		TRACE(("gdt at %p\n", gdt));

		// put standard segment descriptors in it
		clear_segment_descriptor(&gdt[0]);
		set_segment_descriptor(&gdt[1], 0, 0xfffff, DT_CODE_READABLE, DPL_KERNEL);
			// seg 0x08 - kernel 4GB code
		set_segment_descriptor(&gdt[2], 0, 0xfffff, DT_DATA_WRITEABLE, DPL_KERNEL);
			// seg 0x10 - kernel 4GB data

		set_segment_descriptor(&gdt[3], 0, 0xfffff, DT_CODE_READABLE, DPL_USER);
			// seg 0x1b - ring 3 user 4GB code
		set_segment_descriptor(&gdt[4], 0, 0xfffff, DT_DATA_WRITEABLE, DPL_USER);
			// seg 0x23 - ring 3 user 4GB data

		// gdt[5] and above will be filled later by the kernel
		// to contain the TSS descriptors, and for TLS (one for every CPU)

		// map the gdt into virtual space
		ka->arch_args.vir_gdt = (uint32)get_next_virtual_page();
		map_page(ka->arch_args.vir_gdt, (uint32)gdt, kDefaultPageFlags);

		// load the GDT
		gdtDescriptor.limit = GDT_LIMIT - 1;
		gdtDescriptor.base = (uint32 *)ka->arch_args.vir_gdt;

		asm("lgdt	%0;"
			: : "m" (gdtDescriptor));

		TRACE(("gdt at virtual address %p\n", (void *)ka->arch_args.vir_gdt));
	}
}


extern "C" void
mmu_init(void)
{
	init_page_directory();

	// Map the page directory into kernel space at 0xffc00000-0xffffffff
	// this enables a mmu trick where the 4 MB region that this pgdir entry
	// represents now maps the 4MB of potential pagetables that the pgdir
	// points to. Thrown away later in VM bringup, but useful for now.
	sPageDirectory[1023] = (uint32)sPageDirectory | kDefaultPageFlags;

	// also map it on the next vpage
	ka->arch_args.vir_pgdir = get_next_virtual_page();
	map_page(ka->arch_args.vir_pgdir, (uint32)sPageDirectory, kDefaultPageFlags);

	// mark memory that we know is used
	/*ka->physical_allocated_range[0].start = BOOTDIR_ADDR;
	ka->physical_allocated_range[0].size = sNextPhysicalAddress - BOOTDIR_ADDR;*/
	ka->num_physical_allocated_ranges = 0;	//1;

	extended_memory *extMemoryBlock;
	uint32 extMemoryCount = get_memory_map(&extMemoryBlock);

	// figure out the memory map
	if (extMemoryCount > 0) {
		uint32 i;

		ka->num_physical_memory_ranges = 0;

		for (i = 0; i < extMemoryCount; i++) {
			if (extMemoryBlock[i].type == 1) {
				// round everything up to page boundaries, exclusive of pages it partially occupies
				extMemoryBlock[i].length -= (extMemoryBlock[i].base_addr % B_PAGE_SIZE)
					? (B_PAGE_SIZE - (extMemoryBlock[i].base_addr % B_PAGE_SIZE)) : 0;
				extMemoryBlock[i].base_addr = ROUNDUP(extMemoryBlock[i].base_addr, B_PAGE_SIZE);
				extMemoryBlock[i].length = ROUNDOWN(extMemoryBlock[i].length, B_PAGE_SIZE);

				// this is mem we can use
				if (ka->num_physical_memory_ranges == 0) {
					ka->physical_memory_range[0].start = (addr_t)extMemoryBlock[i].base_addr;
					ka->physical_memory_range[0].size = (addr_t)extMemoryBlock[i].length;
					ka->num_physical_memory_ranges++;
				} else {
					// we might have to extend the previous hole
					addr_t previous_end = ka->physical_memory_range[ka->num_physical_memory_ranges - 1].start
						+ ka->physical_memory_range[ka->num_physical_memory_ranges - 1].size;

					if (previous_end <= extMemoryBlock[i].base_addr
						&& ((extMemoryBlock[i].base_addr - previous_end) < 0x100000)) {
						// extend the previous buffer
						ka->physical_memory_range[ka->num_physical_memory_ranges - 1].size +=
							(extMemoryBlock[i].base_addr - previous_end) +
							extMemoryBlock[i].length;

						// mark the gap between the two allocated ranges in use
						ka->physical_allocated_range[ka->num_physical_allocated_ranges].start = previous_end;
						ka->physical_allocated_range[ka->num_physical_allocated_ranges].size = extMemoryBlock[i].base_addr - previous_end;
						ka->num_physical_allocated_ranges++;
					}
				}
			}
		}
	} else {
		// ToDo: for now!
		uint32 memSize = 32 * 1024 * 1024;

		// we dont have an extended map, assume memory is contiguously mapped at 0x0
		ka->physical_memory_range[0].start = 0;
		ka->physical_memory_range[0].size = memSize;
		ka->num_physical_memory_ranges = 1;

		// mark the bios area allocated
		ka->physical_allocated_range[ka->num_physical_allocated_ranges].start = 0x9f000; // 640k - 1 page
		ka->physical_allocated_range[ka->num_physical_allocated_ranges].size = 0x61000;
		ka->num_physical_allocated_ranges++;
	}

	// save the memory we've virtually allocated (for the kernel and other stuff)
	ka->virtual_allocated_range[0].start = KERNEL_BASE;
	ka->virtual_allocated_range[0].size = sNextVirtualAddress - KERNEL_BASE;
	ka->num_virtual_allocated_ranges = 1;

	// sort the address ranges
	sort_addr_range(ka->physical_memory_range, ka->num_physical_memory_ranges);
	sort_addr_range(ka->physical_allocated_range, ka->num_physical_allocated_ranges);
	sort_addr_range(ka->virtual_allocated_range, ka->num_virtual_allocated_ranges);

#if 1
	{
		unsigned int i;

		dprintf("phys memory ranges:\n");
		for (i = 0; i < ka->num_physical_memory_ranges; i++) {
			dprintf("    base 0x%08lx, length 0x%08lx\n", ka->physical_memory_range[i].start, ka->physical_memory_range[i].size);
		}

		dprintf("allocated phys memory ranges:\n");
		for (i = 0; i < ka->num_physical_allocated_ranges; i++) {
			dprintf("    base 0x%08lx, length 0x%08lx\n", ka->physical_allocated_range[i].start, ka->physical_allocated_range[i].size);
		}

		dprintf("allocated virt memory ranges:\n");
		for (i = 0; i < ka->num_virtual_allocated_ranges; i++) {
			dprintf("    base 0x%08lx, length 0x%08lx\n", ka->virtual_allocated_range[i].start, ka->virtual_allocated_range[i].size);
		}
	}
#endif

	ka->arch_args.page_hole = 0xffc00000;
}

