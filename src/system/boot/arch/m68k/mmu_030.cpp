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

#include "arch_030_mmu.h"


#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


extern page_root_entry *gPageRoot;


static void
initialize(void)
{
	TRACE(("mmu_030:initialize\n"));
}


static status_t
set_tt(int which, addr_t pa, size_t len, uint32 perms)
{
	TRACE(("mmu_030:set_tt(%d, 0x%lx, 0x%lx, 0x%08lx)\n", which, pa, len, perms));
	uint32 mask;
	uint32 ttr = 0;
	mask = 0x0000ffff;
	if (len) {
		len = (len >> 24) & 0x00ff;
		while (len >>= 1)
			mask <<= 1;
		// enable, cachable(?), r/w
		// super only
		// mc68030 user's manual, page 9-57
		ttr = 0x08043;
		ttr |= (pa & 0xff000000);
		ttr |= (mask & 0x00ff0000);
	}
	TRACE(("mmu_030:set_tt: 0x%08lx\n", ttr));
	
	/* as seen in linux and BSD code,
	 * we need to use .chip pseudo op here as -m68030 doesn't seem to help gas grok it. 
	 */
	switch (which) {
		case 0:
			asm volatile(  \
				".chip 68030\n\t"				\
				"pmove %%tt0,(%0)\n\t"				\
				".chip 68k\n\t"					\
				: : "a"(&ttr));
			break;
		case 1:
			asm volatile(  \
				".chip 68030\n\t"				\
				"pmove (%0),%%tt1\n"				\
				".chip 68k\n\t"					\
				: : "a"(&ttr));
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}


static status_t
load_rp(addr_t pa)
{
	TRACE(("mmu_030:load_rp(0x%lx)\n", pa));
	long_page_directory_entry entry;
	*(uint64 *)&entry = DFL_PAGEENT_VAL;
	entry.type = DT_ROOT;
	entry.addr = TA_TO_PREA(((addr_t)pa));

	asm volatile( \
		"pmove (%0),%%srp\n" \
		"pmove (%0),%%crp\n" \
		: : "a"((uint64 *)&entry));
	return B_OK;
}


static status_t
allocate_kernel_pgdirs(void)
{
	page_root_entry *pr = gPageRoot;
	page_directory_entry *pd;
	addr_t tbl;
	int i;

	// we'll fill in the 2nd half with ready made page dirs
	for (i = NUM_ROOTENT_PER_TBL/2; i < NUM_ROOTENT_PER_TBL; i++) {
		if (i % NUM_DIRTBL_PER_PAGE)
			tbl += SIZ_DIRTBL;
		else
			tbl = mmu_get_next_page_tables();
		pr[i].addr = TA_TO_PREA(tbl);
		pr[i].type = DT_ROOT;
		pd = (page_directory_entry *)tbl;
		for (int32 j = 0; j < NUM_DIRENT_PER_TBL; j++)
			*(page_directory_entry_scalar *)(&pd[j]) = DFL_DIRENT_VAL;
	}
	return B_OK;
}


static status_t
enable_paging(void)
{
	TRACE(("mmu_030:enable_paging\n"));
	return B_NO_INIT;
}


const struct boot_mmu_ops k030MMUOps = {
	&initialize,
	&set_tt,
	&load_rp,
	&allocate_kernel_pgdirs,
	&enable_paging
};
