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

#include <OS.h>

#include <string.h>

#include "arch_040_mmu.h"


#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


static uint32 *sPageDirectory = 0;



static void
initialize(void)
{
	TRACE(("mmu_040:initialize\n"));
}


static status_t
set_tt(int which, addr_t pa, size_t len, uint32 perms)
{
	TRACE(("mmu_040:set_tt(%d, 0x%lx, %ld, 0x%08lx)\n", which, pa, len, perms));
	uint32 mask;
	uint32 ttr = 0;
	mask = 1;
	if (len) {
		while (len >>= 1)
			mask <<= 1;
		mask = (mask - 1);
		// enable, super only, upa=0,
		// cachable write-through, rw
		ttr = 0x0a000;
		ttr |= (pa & 0xff000000);
		ttr |= ((mask & 0xff000000) >> 8);
	}
	TRACE(("mmu_040:set_tt: 0x%08lx\n", ttr));
	

	switch (which) {
		case 0:
			asm volatile(  \
				"movec %0,%%dtt0\n"				\
				"movec %0,%%itt0\n"				\
				: : "d"(ttr));
			break;
		case 1:
			asm volatile(  \
				"movec %0,%%dtt1\n"				\
				"movec %0,%%itt1\n"				\
				: : "d"(ttr));
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}


static status_t
load_rp(addr_t pa)
{
	TRACE(("mmu_040:load_rp(0x%lx)\n", pa));
	// sanity check
	if (pa & ((1 << 9) - 1)) {
		panic("mmu root pointer missaligned!");
		return EINVAL;
	}
	/* mc68040 user's manual, 6-37 */
	/* pflush before... why not after ? */
	asm volatile(		   \
		"pflusha\n"		   \
		"movec %0,%%srp\n" \
		"movec %0,%%urp\n" \
		"pflusha\n"		   \
		: : "d"(pa));
	return B_OK;
}


static status_t
enable_paging(void)
{
	TRACE(("mmu_040:enable_paging\n"));
	uint16 tcr = 0x80; // Enable, 4K page size
	asm volatile( \
		"pflusha\n"		   \
		"movec %0,%%tcr\n" \
		"pflusha\n"		   \
		: : "d"(tcr));
	return B_OK;
}


const struct boot_mmu_ops k040MMUOps = {
	&initialize,
	&set_tt,
	&load_rp,
	&enable_paging
};
