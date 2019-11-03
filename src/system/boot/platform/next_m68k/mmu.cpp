/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Based on code written by Travis Geiselbrecht for NewOS.
 *
 * Distributed under the terms of the MIT License.
 */


#include "nextrom.h"
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

#warning TODO: M68K: NEXT: mmu

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

#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// since the page root directory doesn't take a full page (1k)
// we stuff some other stuff after it, like the interrupt vectors (1k)
#define VBR_PAGE_OFFSET 1024

static const uint32 kDefaultPageTableFlags = 0x07;	// present, user, R/W
static const size_t kMaxKernelSize = 0x200000;		// 2 MB for the kernel

// working page directory and page table
addr_t gPageRoot = 0;

static addr_t sNextPhysicalAddress = 0x100000;
static addr_t sNextVirtualAddress = KERNEL_LOAD_BASE + kMaxKernelSize;
static addr_t sMaxVirtualAddress = KERNEL_LOAD_BASE /*+ 0x400000*/;

#if 0
static addr_t sNextPageTableAddress = 0x90000;
static const uint32 kPageTableRegionEnd = 0x9e000;
	// we need to reserve 2 pages for the SMP trampoline code XXX:no
#endif



/**	This will unmap the allocated chunk of memory from the virtual
 *	address space. It might not actually free memory (as its implementation
 *	is very simple), but it might.
 */

extern "C" void
mmu_free(void *virtualAddress, size_t size)
{
	TRACE(("mmu_free(virtualAddress = %p, size: %ld)\n", virtualAddress, size));
}


/** Sets up the final and kernel accessible GDT and IDT tables.
 *	BIOS calls won't work any longer after this function has
 *	been called.
 */

extern "C" void
mmu_init_for_kernel(void)
{
	TRACE(("mmu_init_for_kernel\n"));
}


extern "C" void
mmu_init(void)
{
	TRACE(("mmu_init\n"));
}


//	#pragma mark -


extern "C" status_t
platform_allocate_region(void **_address, size_t size, uint8 protection,
	bool /*exactAddress*/)
{
	return B_UNSUPPORTED;
}


extern "C" status_t
platform_free_region(void *address, size_t size)
{
	return B_UNSUPPORTED;
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
	return B_UNSUPPORTED;
}


