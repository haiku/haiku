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


//#define TRACE_MMU
#ifdef TRACE_MMU
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


extern page_root_entry *gPageRoot;

//XXX: the milan BIOS uses the mmu for itself,
// likely to virtualize missing Atari I/O ports...
// tcr: c000 (enabled, 8k pages :()
// dtt0: 803fe140	0x80000000 & ~0x3f... en ignFC2 U=1 CI,S  RW
// dtt1: 403fe060	0x40000000 & ~0x3f... en ignFC2 U=0 CI,NS RW
// itt0: 803fe040	0x80000000 & ~0x3f... en ignFC2 U=0 CI,S  RW
// itt1: 403fe000	0x40000000 & ~0x3f... en ignFC2 U=0 C,WT  RW
// srp:  00dfde00
// urp:  00dfde00

static void
dump_mmu(void)
{
	uint32 dttr0, dttr1;
	uint32 ittr0, ittr1;
	uint32 srp, urp;
	uint32 tcr;

	TRACE(("mmu_040:dump:\n"));

	asm volatile("movec %%tcr,%0\n" : "=d"(tcr) :);
	TRACE(("tcr:\t%08lx\n", tcr));

	asm volatile("movec %%dtt0,%0\n" : "=d"(dttr0) :);
	TRACE(("dtt0:\t%08lx\n", dttr0));
	asm volatile("movec %%dtt1,%0\n" : "=d"(dttr1) :);
	TRACE(("dtt1:\t%08lx\n", dttr1));

	asm volatile("movec %%itt0,%0\n" : "=d"(ittr0) :);
	TRACE(("itt0:\t%08lx\n", ittr0));
	asm volatile("movec %%itt1,%0\n" : "=d"(ittr1) :);
	TRACE(("itt1:\t%08lx\n", ittr1));

	asm volatile("movec %%srp,%0\n" : "=d"(srp) :);
	TRACE(("srp:\t%08lx\n", srp));
	asm volatile("movec %%urp,%0\n" : "=d"(urp) :);
	TRACE(("urp:\t%08lx\n", urp));

	TRACE(("mmu_040:dump:\n"));
}


static void
initialize(void)
{
	dump_mmu();
	TRACE(("mmu_040:initialize\n"));
}


static status_t
set_tt(int which, addr_t pa, size_t len, uint32 perms /* NOTUSED */)
{
	TRACE(("mmu_040:set_tt(%d, 0x%lx, 0x%lx, 0x%08lx)\n", which, pa, len, perms));
	uint32 mask;
	uint32 ttr = 0;
	mask = 0x0000ffff;
	if (len) {
		len = (len >> 24) & 0x00ff;
		while (len >>= 1)
			mask <<= 1;
		// enable, super only, upa=0,
		// cachable write-through, rw
		ttr = 0x0a000;
		ttr |= (pa & 0xff000000);
		ttr |= (mask & 0x00ff0000);
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
	// make sure it's empty
	page_directory_entry *pr = (page_directory_entry *)pa;
	for (int32 j = 0; j < NUM_ROOTENT_PER_TBL; j++)
		pr[j] = DFL_ROOTENT_VAL;
	
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
		pr[i] = DT_ROOT | TA_TO_PREA(tbl);
		pd = (page_directory_entry *)tbl;
		for (int32 j = 0; j < NUM_DIRENT_PER_TBL; j++)
			pd[j] = DFL_DIRENT_VAL;
	}
	return B_OK;
}


static status_t
enable_paging(void)
{
	TRACE(("mmu_040:enable_paging\n"));
	uint16 tcr = 0x8000; // Enable, 4K page size
	asm volatile( \
		"pflusha\n"		   \
		"movec %0,%%tcr\n" \
		"pflusha\n"		   \
		: : "d"(tcr));
	return B_OK;
}


static status_t
add_page_table(addr_t virtualAddress)
{
	page_root_entry *pr = gPageRoot;
	page_directory_entry *pd;
	page_table_entry *pt;
	addr_t tbl;
	uint32 index;
	uint32 i;
	
	TRACE(("mmu->add_page_table(base = %p)\n", (void *)virtualAddress));

	// everything much simpler here because pa = va
	// thanks to transparent translation

	index = VADDR_TO_PRENT(virtualAddress);
	if (PRE_TYPE(pr[index]) != DT_ROOT)
		panic("invalid page root entry %d\n", index);
#if 0
	// not needed anymore
	if (PRE_TYPE(pr[index]) != DT_ROOT) {
		unsigned aindex = index & ~(NUM_DIRTBL_PER_PAGE-1); /* aligned */
		//TRACE(("missing page root entry %d ai %d\n", index, aindex));
		tbl = mmu_get_next_page_tables();
		if (!tbl)
			return ENOMEM;
		// for each pgdir on the allocated page:
		for (i = 0; i < NUM_DIRTBL_PER_PAGE; i++) {
			page_root_entry *apr = &pr[aindex + i];
			apr->addr = TA_TO_PREA(tbl);
			apr->type = DT_ROOT;
			//TRACE(("inserting tbl @ %p as %08x pr[%d] %08x\n", tbl, TA_TO_PREA(tbl), aindex + i, *(uint32 *)apr));
			// clear the table
			//TRACE(("clearing table[%d]\n", i));
			pd = (page_directory_entry *)tbl;
			for (int32 j = 0; j < NUM_DIRENT_PER_TBL; j++)
				*(page_directory_entry_scalar *)(&pd[j]) = DFL_DIRENT_VAL;
			tbl += SIZ_DIRTBL;
		}
	}
#endif
	pd = (page_directory_entry *)PRE_TO_TA(pr[index]);

	index = VADDR_TO_PDENT(virtualAddress);
	if (PDE_TYPE(pd[index]) != DT_DIR) {
		unsigned aindex = index & ~(NUM_PAGETBL_PER_PAGE-1); /* aligned */
		//TRACE(("missing page dir entry %d ai %d\n", index, aindex));
		tbl = mmu_get_next_page_tables();
		if (!tbl)
			return ENOMEM;
		// for each pgdir on the allocated page:
		for (i = 0; i < NUM_PAGETBL_PER_PAGE; i++) {
			page_directory_entry *apd = &pd[aindex + i];
			pd[aindex + i] = DT_DIR | TA_TO_PDEA(tbl);
			// clear the table
			//TRACE(("clearing table[%d]\n", i));
			pt = (page_table_entry *)tbl;
			for (int32 j = 0; j < NUM_PAGEENT_PER_TBL; j++)
				pt[j] = DFL_PAGEENT_VAL;
			tbl += SIZ_PAGETBL;
		}
	}
#if 0
	pt = PDE_TO_TA(pd[index]);

	index = VADDR_TO_PTENT(virtualAddress);
	pt[index].addr = TA_TO_PTEA(0xdeadb00b);
	pt[index].supervisor = 1;
	pt[index].type = DT_PAGE;
#endif
	return B_OK;
}


static page_table_entry *
lookup_pte(addr_t virtualAddress)
{
	page_root_entry *pr = gPageRoot;
	page_directory_entry *pd;
	page_table_entry *pt;
	uint32 rindex, dindex, pindex;

	rindex = VADDR_TO_PRENT(virtualAddress);
	if (PRE_TYPE(pr[rindex]) != DT_ROOT)
		panic("lookup_pte: invalid entry pgrt[%d]", rindex);
	pd = (page_directory_entry *)PRE_TO_TA(pr[rindex]);

	dindex = VADDR_TO_PDENT(virtualAddress);
	if (PDE_TYPE(pd[dindex]) != DT_DIR)
		panic("lookup_pte: invalid entry pgrt[%d] prdir[%d]", rindex, dindex);
	pt = (page_table_entry *)PDE_TO_TA(pd[dindex]);
	
	pindex = VADDR_TO_PTENT(virtualAddress);
#if 0 // of course, it's used in map_page!
	if (PTE_TYPE(pt[pindex]) != DT_PAGE)
		panic("lookup_pte: invalid entry pgrt[%d] prdir[%d] pgtbl[%d]",
			rindex, dindex, pindex);
#endif

	return (&pt[pindex]);
}


static void
unmap_page(addr_t virtualAddress)
{
	page_table_entry *pt;

	TRACE(("mmu->unmap_page(virtualAddress = %p)\n", (void *)virtualAddress));

	if (virtualAddress < KERNEL_BASE)
		panic("unmap_page: asked to unmap invalid page %p!\n",
			(void *)virtualAddress);

	// unmap the page from the correct page table
	pt = lookup_pte(virtualAddress);

	if (PTE_TYPE(*pt) != DT_PAGE)
		panic("unmap_page: asked to map non-existing page for %08x\n",
			virtualAddress);

	*pt = DT_INVALID | TA_TO_PTEA(0xdeadb00b);

	// flush ATC
	asm volatile("pflush (%0)" : : "a" (virtualAddress));
}


/** insert the physical address into existing page table */
static void
map_page(addr_t virtualAddress, addr_t physicalAddress, uint32 flags)
{
	page_table_entry *pt;

	TRACE(("mmu->map_page: vaddr 0x%lx, paddr 0x%lx\n", virtualAddress, physicalAddress));


	physicalAddress &= ~(B_PAGE_SIZE - 1);

	// map the page to the correct page table

	pt = lookup_pte(virtualAddress);

	if (PTE_TYPE(*pt) != DT_INVALID)
		panic("map_page: asked to map existing page for %08x\n",
			virtualAddress);

	TRACE(("map_page: inserting pageTableEntry %p, physicalAddress %p\n", 
		pt, physicalAddress));


	*pt = DT_PAGE
		| TA_TO_PTEA(physicalAddress)
#ifdef MMU_HAS_GLOBAL_PAGES
		| M68K_PTE_GLOBAL
#endif
		| M68K_PTE_SUPERVISOR;
	// XXX: are flags needed ? ro ? global ?

	// flush ATC
	asm volatile("pflush (%0)" : : "a" (virtualAddress));

	TRACE(("mmu->map_page: done\n"));
}




const struct boot_mmu_ops k040MMUOps = {
	&initialize,
	&set_tt,
	&load_rp,
	&allocate_kernel_pgdirs,
	&enable_paging,
	&add_page_table,
	&unmap_page,
	&map_page
};
