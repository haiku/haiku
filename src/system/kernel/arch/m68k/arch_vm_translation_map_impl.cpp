/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 *
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#ifndef ARCH_M68K_MMU_TYPE
#error This file is included from arch_*_mmu.cpp
#endif

/*  (mmu_man) Implementation details on 68030 and others:
	
	Unlike on x86 we can't just switch the context to another team by just
	setting a register to another page directory, since we only have one
	page table containing both kernel and user address mappings. 
	The 030 supports arbitrary layout of the page directory tree, including
	a 1-bit first level (2 entries top level table) that would map kernel
	and user land at a single place. But 040 and later only support a fixed
	splitting of 7/7/6 for 4K pages.

	Since 68k SMP hardware is rare enough we don't want to support them, we
	can take some shortcuts.
	
	As we don't want a separate user and kernel space, we'll use a single
	table. With the 7/7/6 split the 2nd level would require 32KB of tables,
	which is small enough to not want to use the list hack from x86.
	XXX: we use the hack for now, check later

	Since page directories/tables don't fit exactly a page, we stuff more
	than one per page, and allocate them all at once, and add them at the
	same time to the tree. So we guarantee all higher-level entries modulo
	the number of tables/page are either invalid or present.
 */

#include <KernelExport.h>
#include <kernel.h>
#include <vm.h>
#include <vm_address_space.h>
#include <vm_priv.h>
#include <int.h>
#include <boot/kernel_args.h>
#include <arch/vm_translation_map.h>
#include <arch/cpu.h>
#include <arch_mmu.h>
#include <stdlib.h>

#include "generic_vm_physical_page_mapper.h"



//#define TRACE_VM_TMAP
#ifdef TRACE_VM_TMAP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

//XXX: that's platform specific!
// 14 MB of iospace
#define IOSPACE_SIZE (14*1024*1024)
// 4 MB chunks, to optimize for 4 MB pages
// XXX: no such thing on 68k (060 ?)
// 256K
#define IOSPACE_CHUNK_SIZE (256*1024)

static page_table_entry *iospace_pgtables = NULL;

#define PAGE_INVALIDATE_CACHE_SIZE 64

// vm_translation object stuff
typedef struct vm_translation_map_arch_info {
	page_root_entry *rtdir_virt;
	page_root_entry *rtdir_phys;
	int num_invalidate_pages;
	addr_t pages_to_invalidate[PAGE_INVALIDATE_CACHE_SIZE];
} vm_translation_map_arch_info;

#if 0
static page_table_entry *page_hole = NULL;
static page_directory_entry *page_hole_pgdir = NULL;
#endif
static page_root_entry *sKernelPhysicalPageDirectory = NULL;
static page_root_entry *sKernelVirtualPageDirectory = NULL;
static addr_t sQueryPage = NULL;
//static page_table_entry *sQueryPageTable;
//static page_directory_entry *sQueryPageDir;
// MUST be aligned
static page_table_entry sQueryDesc __attribute__ (( aligned (4) ));

static vm_translation_map *tmap_list;
static spinlock tmap_list_lock;

static addr_t sIOSpaceBase;

#define CHATTY_TMAP 0

#if 0
// use P*E_TO_* and TA_TO_P*EA !
#define ADDR_SHIFT(x) ((x)>>12)
#define ADDR_REVERSE_SHIFT(x) ((x)<<12)
#endif

/* 7/7/6 split */
#define VADDR_TO_PRENT(va) (((va) / B_PAGE_SIZE) / (64*128))
#define VADDR_TO_PDENT(va) ((((va) / B_PAGE_SIZE) / 64) % 128)
#define VADDR_TO_PTENT(va) (((va) / B_PAGE_SIZE) % 64)

#define FIRST_USER_PGROOT_ENT    (VADDR_TO_PRENT(USER_BASE))
#define FIRST_USER_PGDIR_ENT    (VADDR_TO_PDENT(USER_BASE))
#define NUM_USER_PGROOT_ENTS     (VADDR_TO_PRENT(ROUNDUP(USER_SIZE, B_PAGE_SIZE * 64 * 128)))
#define NUM_USER_PGDIR_ENTS     (VADDR_TO_PDENT(ROUNDUP(USER_SIZE, B_PAGE_SIZE * 64)))
#define FIRST_KERNEL_PGROOT_ENT  (VADDR_TO_PRENT(KERNEL_BASE))
#define FIRST_KERNEL_PGDIR_ENT  (VADDR_TO_PDENT(KERNEL_BASE))
#define NUM_KERNEL_PGROOT_ENTS   (VADDR_TO_PRENT(KERNEL_SIZE))
#define NUM_KERNEL_PGDIR_ENTS   (VADDR_TO_PDENT(KERNEL_SIZE))
#define IS_KERNEL_MAP(map)		(map->arch_data->rtdir_phys == sKernelPhysicalPageRoot)

static status_t early_query(addr_t va, addr_t *out_physical);
static status_t get_physical_page_tmap(addr_t pa, addr_t *va, uint32 flags);
static status_t put_physical_page_tmap(addr_t va);

static void flush_tmap(vm_translation_map *map);


static void *
m68k_translation_map_get_pgdir(vm_translation_map *map)
{
	return map->arch_data->rtdir_phys;
}


static inline void
init_page_root_entry(page_root_entry *entry)
{
	// DT_INVALID is 0
	*(page_root_entry_scalar *)entry = DFL_ROOTENT_VAL;
}


static inline void
update_page_root_entry(page_root_entry *entry, page_root_entry *with)
{
	// update page directory entry atomically
	*(page_root_entry_scalar *)entry = *(page_root_entry_scalar *)with;
}


static inline void
init_page_directory_entry(page_directory_entry *entry)
{
	*(page_directory_entry_scalar *)entry = DFL_DIRENT_VAL;
}


static inline void
update_page_directory_entry(page_directory_entry *entry, page_directory_entry *with)
{
	// update page directory entry atomically
	*(page_directory_entry_scalar *)entry = *(page_directory_entry_scalar *)with;
}


static inline void
init_page_table_entry(page_table_entry *entry)
{
	*(page_table_entry_scalar *)entry = DFL_PAGEENT_VAL;
}


static inline void
update_page_table_entry(page_table_entry *entry, page_table_entry *with)
{
	// update page table entry atomically
	// XXX: is it ?? (long desc?)
	*(page_table_entry_scalar *)entry = *(page_table_entry_scalar *)with;
}


static void
_update_all_pgdirs(int index, page_root_entry e)
{
	vm_translation_map *entry;
	unsigned int state = disable_interrupts();

	acquire_spinlock(&tmap_list_lock);

	for(entry = tmap_list; entry != NULL; entry = entry->next)
		entry->arch_data->rtdir_virt[index] = e;

	release_spinlock(&tmap_list_lock);
	restore_interrupts(state);
}


/*!	Acquires the map's recursive lock, and resets the invalidate pages counter
	in case it's the first locking recursion.
*/
static status_t
lock_tmap(vm_translation_map *map)
{
	TRACE(("lock_tmap: map %p\n", map));

	recursive_lock_lock(&map->lock);
	if (recursive_lock_get_recursion(&map->lock) == 1) {
		// we were the first one to grab the lock
		TRACE(("clearing invalidated page count\n"));
		map->arch_data->num_invalidate_pages = 0;
	}

	return B_OK;
}


/*!	Unlocks the map, and, if we'll actually losing the recursive lock,
	flush all pending changes of this map (ie. flush TLB caches as
	needed).
*/
static status_t
unlock_tmap(vm_translation_map *map)
{
	TRACE(("unlock_tmap: map %p\n", map));

	if (recursive_lock_get_recursion(&map->lock) == 1) {
		// we're about to release it for the last time
		flush_tmap(map);
	}

	recursive_lock_unlock(&map->lock);
	return B_OK;
}


static void
destroy_tmap(vm_translation_map *map)
{
	int state;
	vm_translation_map *entry;
	vm_translation_map *last = NULL;
	unsigned int i, j;

	if (map == NULL)
		return;

	// remove it from the tmap list
	state = disable_interrupts();
	acquire_spinlock(&tmap_list_lock);

	entry = tmap_list;
	while (entry != NULL) {
		if (entry == map) {
			if (last != NULL)
				last->next = entry->next;
			else
				tmap_list = entry->next;

			break;
		}
		last = entry;
		entry = entry->next;
	}

	release_spinlock(&tmap_list_lock);
	restore_interrupts(state);

	if (map->arch_data->rtdir_virt != NULL) {
		// cycle through and free all of the user space pgtables
		// since the size of tables don't match B_PAEG_SIZE,
		// we alloc several at once, based on modulos,
		// we make sure they are either all in the tree or none.
		for (i = VADDR_TO_PRENT(USER_BASE); i <= VADDR_TO_PRENT(USER_BASE + (USER_SIZE - 1)); i++) {
			addr_t pgdir_pn;
			page_directory_entry *pgdir;
			vm_page *dirpage;

			if (map->arch_data->rtdir_virt[i].type == DT_INVALID)
				continue;
			if (map->arch_data->rtdir_virt[i].type != DT_ROOT) {
				panic("rtdir[%d]: buggy descriptor type", i);
				return;
			}
			// suboptimal (done 8 times)
			pgdir_pn = PRE_TO_PA(map->arch_data->rtdir_virt[i]);
			dirpage = vm_lookup_page(pgdir_pn);
			pgdir = &(((page_directory_entry *)dirpage)[i%NUM_DIRTBL_PER_PAGE]);

			for (j = 0; j <= NUM_DIRENT_PER_TBL; j+=NUM_PAGETBL_PER_PAGE) {
				addr_t pgtbl_pn;
				page_table_entry *pgtbl;
				vm_page *page;
				if (pgdir[j].type == DT_INVALID)
					continue;
				if (pgdir[j].type != DT_DIR) {
					panic("rtdir[%d][%d]: buggy descriptor type", i, j);
					return;
				}
				pgtbl_pn = PDE_TO_PN(pgdir[j]);
				page = vm_lookup_page(pgtbl_pn);
				pgtbl = (page_table_entry *)page;
				
				if (!page) {
					panic("destroy_tmap: didn't find pgtable page\n");
					return;
				}
				vm_page_set_state(page, PAGE_STATE_FREE);
			}
			if (((i+1)%NUM_DIRTBL_PER_PAGE) == 0)
				vm_page_set_state(dirpage, PAGE_STATE_FREE);
		}
		free(map->arch_data->rtdir_virt);
	}

	free(map->arch_data);
	recursive_lock_destroy(&map->lock);
}


static void
put_pgdir_in_pgroot(page_root_entry *entry,
	addr_t pgdir_phys, uint32 attributes)
{
	page_root_entry dir;
	// put it in the pgdir
	init_page_root_entry(&dir);
	dir.addr = TA_TO_PREA(pgdir_phys);

	// ToDo: we ignore the attributes of the page table - for compatibility
	//	with BeOS we allow having user accessible areas in the kernel address
	//	space. This is currently being used by some drivers, mainly for the
	//	frame buffer. Our current real time data implementation makes use of
	//	this fact, too.
	//	We might want to get rid of this possibility one day, especially if
	//	we intend to port it to a platform that does not support this.
	//dir.user = 1;
	//dir.rw = 1;
	dir.type = DT_ROOT;
	update_page_root_entry(entry, &dir);
}


static void
put_pgtable_in_pgdir(page_directory_entry *entry,
	addr_t pgtable_phys, uint32 attributes)
{
	page_directory_entry table;
	// put it in the pgdir
	init_page_directory_entry(&table);
	table.addr = TA_TO_PDEA(pgtable_phys);

	// ToDo: we ignore the attributes of the page table - for compatibility
	//	with BeOS we allow having user accessible areas in the kernel address
	//	space. This is currently being used by some drivers, mainly for the
	//	frame buffer. Our current real time data implementation makes use of
	//	this fact, too.
	//	We might want to get rid of this possibility one day, especially if
	//	we intend to port it to a platform that does not support this.
	//table.user = 1;
	//table.rw = 1;
	table.type = DT_DIR;
	update_page_directory_entry(entry, &table);
}


static void
put_page_table_entry_in_pgtable(page_table_entry *entry,
	addr_t physicalAddress, uint32 attributes, bool globalPage)
{
	page_table_entry page;
	init_page_table_entry(&page);

	page.addr = TA_TO_PTEA(physicalAddress);

	// if the page is user accessible, it's automatically
	// accessible in kernel space, too (but with the same
	// protection)
	page.supervisor = (attributes & B_USER_PROTECTION) == 0;
	if (page.supervisor)
		page.write_protect = (attributes & B_KERNEL_WRITE_AREA) == 0;
	else
		page.write_protect = (attributes & B_WRITE_AREA) == 0;
	page.type = DT_PAGE;

#ifdef PAGE_HAS_GLOBAL_BIT
	if (globalPage)
		page.global = 1;
#endif

	// put it in the page table
	update_page_table_entry(entry, &page);
}


static size_t
map_max_pages_need(vm_translation_map */*map*/, addr_t start, addr_t end)
{
	size_t need;
	size_t pgdirs = VADDR_TO_PRENT(end) + 1 - VADDR_TO_PRENT(start);
	// how much for page directories
	need = (pgdirs + NUM_DIRTBL_PER_PAGE - 1) / NUM_DIRTBL_PER_PAGE;
	// and page tables themselves
	need = ((pgdirs * NUM_DIRENT_PER_TBL) + NUM_PAGETBL_PER_PAGE - 1) / NUM_PAGETBL_PER_PAGE;

	// better rounding when only 1 pgdir
	// XXX: do better for other cases
	if (pgdirs == 1) {
		need = 1;
		need += (VADDR_TO_PDENT(end) + 1 - VADDR_TO_PDENT(start) + NUM_PAGETBL_PER_PAGE - 1) / NUM_PAGETBL_PER_PAGE;
	}
	
	return need;
}


static status_t
map_tmap(vm_translation_map *map, addr_t va, addr_t pa, uint32 attributes)
{
	page_root_entry *pr;
	page_directory_entry *pd;
	page_table_entry *pt;
	addr_t pd_pg, pt_pg;
	unsigned int rindex, dindex, pindex;
	int err;

	TRACE(("map_tmap: entry pa 0x%lx va 0x%lx\n", pa, va));

/*
	dprintf("pgdir at 0x%x\n", pgdir);
	dprintf("index is %d\n", va / B_PAGE_SIZE / 1024);
	dprintf("final at 0x%x\n", &pgdir[va / B_PAGE_SIZE / 1024]);
	dprintf("value is 0x%x\n", *(int *)&pgdir[va / B_PAGE_SIZE / 1024]);
	dprintf("present bit is %d\n", pgdir[va / B_PAGE_SIZE / 1024].present);
	dprintf("addr is %d\n", pgdir[va / B_PAGE_SIZE / 1024].addr);
*/
	pr = map->arch_data->rtdir_virt;

	// check to see if a page directory exists for this range
	rindex = VADDR_TO_PRENT(va);
	if (pr[rindex].type != DT_ROOT) {
		addr_t pgdir;
		vm_page *page;
		unsigned int i;

		// we need to allocate a pgtable
		page = vm_page_allocate_page(PAGE_STATE_CLEAR, true);

		// mark the page WIRED
		vm_page_set_state(page, PAGE_STATE_WIRED);

		pgdir = page->physical_page_number * B_PAGE_SIZE;

		TRACE(("map_tmap: asked for free page for pgdir. 0x%lx\n", pgdir));

		// for each pgdir on the allocated page:
		for (i = 0; i < NUM_DIRTBL_PER_PAGE; i++) {
			unsigned aindex = rindex & ~(NUM_DIRTBL_PER_PAGE-1); /* aligned */
			page_root_entry *apr = &pr[aindex + i];
			
			// put in the pgdir
			put_pgdir_in_pgroot(apr, pgdir, attributes
				| (attributes & B_USER_PROTECTION ? B_WRITE_AREA : B_KERNEL_WRITE_AREA));

			// update any other page directories, if it maps kernel space
			//XXX: suboptimal, should batch them
			if ((aindex+i) >= FIRST_KERNEL_PGDIR_ENT
				&& (aindex+i) < (FIRST_KERNEL_PGDIR_ENT + NUM_KERNEL_PGDIR_ENTS))
				_update_all_pgdirs((aindex+i), pr[aindex+i]);

			pgdir += SIZ_DIRTBL;
		}
#warning M68K: really mean map_count++ ??
		map->map_count++;
	}
	// now, fill in the pentry
	do {
		err = get_physical_page_tmap(PRE_TO_PA(pr[rindex]),
				&pd_pg, PHYSICAL_PAGE_NO_WAIT);
	} while (err < 0);
	pd = (page_directory_entry *)pd_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pd += rindex % NUM_DIRTBL_PER_PAGE;

	// check to see if a page table exists for this range
	dindex = VADDR_TO_PDENT(va);
	if (pd[dindex].type != DT_DIR) {
		addr_t pgtable;
		vm_page *page;
		unsigned int i;

		// we need to allocate a pgtable
		page = vm_page_allocate_page(PAGE_STATE_CLEAR, true);

		// mark the page WIRED
		vm_page_set_state(page, PAGE_STATE_WIRED);

		pgtable = page->physical_page_number * B_PAGE_SIZE;

		TRACE(("map_tmap: asked for free page for pgtable. 0x%lx\n", pgtable));

		// for each pgtable on the allocated page:
		for (i = 0; i < NUM_PAGETBL_PER_PAGE; i++) {
			unsigned aindex = dindex & ~(NUM_PAGETBL_PER_PAGE-1); /* aligned */
			page_directory_entry *apd = &pd[aindex + i];
			
			// put in the pgdir
			put_pgtable_in_pgdir(apd, pgtable, attributes
				| (attributes & B_USER_PROTECTION ? B_WRITE_AREA : B_KERNEL_WRITE_AREA));

			// no need to update other page directories for kernel space;
			// the root-level already point to us.

			pgtable += SIZ_PAGETBL;
		}

#warning M68K: really mean map_count++ ??
		map->map_count++;
	}
	// now, fill in the pentry
	do {
		err = get_physical_page_tmap(PDE_TO_PA(pd[dindex]),
				&pt_pg, PHYSICAL_PAGE_NO_WAIT);
	} while (err < 0);
	pt = (page_table_entry *)pt_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pt += dindex % NUM_PAGETBL_PER_PAGE;

	pindex = VADDR_TO_PTENT(va);

	put_page_table_entry_in_pgtable(&pt[pindex], pa, attributes,
		IS_KERNEL_MAP(map));

	put_physical_page_tmap(pt_pg);
	put_physical_page_tmap(pd_pg);

	if (map->arch_data->num_invalidate_pages < PAGE_INVALIDATE_CACHE_SIZE)
		map->arch_data->pages_to_invalidate[map->arch_data->num_invalidate_pages] = va;

	map->arch_data->num_invalidate_pages++;

	map->map_count++;

	return 0;
}


static status_t
unmap_tmap(vm_translation_map *map, addr_t start, addr_t end)
{
	page_table_entry *pt;
	page_directory_entry *pd;
	page_root_entry *pr = map->arch_data->rtdir_virt;
	addr_t pd_pg, pt_pg;
	status_t status;
	int index;

	start = ROUNDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

	TRACE(("unmap_tmap: asked to free pages 0x%lx to 0x%lx\n", start, end));

restart:
	if (start >= end)
		return B_OK;

	index = VADDR_TO_PRENT(start);
	if (pr[index].type != DT_ROOT) {
		// no pagedir here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE);
		goto restart;
	}

	do {
		status = get_physical_page_tmap(PRE_TO_PA(pr[index]),
			&pd_pg, PHYSICAL_PAGE_NO_WAIT);
	} while (status < B_OK);
	pd = (page_directory_entry *)pd_pg;
	pd += index % NUM_DIRTBL_PER_PAGE;

	index = VADDR_TO_PDENT(start);
	if (pd[index].type != DT_DIR) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE);
		put_physical_page_tmap(pd_pg);
		goto restart;
	}

	do {
		status = get_physical_page_tmap(PDE_TO_PA(pd[index]),
			&pt_pg, PHYSICAL_PAGE_NO_WAIT);
	} while (status < B_OK);
	pt = (page_table_entry *)pt_pg;
	pt += index % NUM_PAGETBL_PER_PAGE;

	for (index = VADDR_TO_PTENT(start); (index < NUM_PAGEENT_PER_TBL) && (start < end);
			index++, start += B_PAGE_SIZE) {
		if (pt[index].type != DT_PAGE && pt[index].type != DT_INDIRECT) {
			// page mapping not valid
			continue;
		}

		TRACE(("unmap_tmap: removing page 0x%lx\n", start));

		pt[index].type = DT_INVALID;
		map->map_count--;

		if (map->arch_data->num_invalidate_pages < PAGE_INVALIDATE_CACHE_SIZE)
			map->arch_data->pages_to_invalidate[map->arch_data->num_invalidate_pages] = start;

		map->arch_data->num_invalidate_pages++;
	}

	put_physical_page_tmap(pt_pg);
	put_physical_page_tmap(pd_pg);

	goto restart;
}

// XXX: 040 should be able to do that with PTEST (but not 030 or 060)
static status_t
query_tmap_interrupt(vm_translation_map *map, addr_t va, addr_t *_physical,
	uint32 *_flags)
{
	page_root_entry *pr = map->arch_data->rtdir_virt;
	page_directory_entry *pd;
	page_table_entry *pt;
	addr_t physicalPageTable;
	int32 cpu = smp_get_current_cpu();
	int32 index;
	int level

	*_physical = 0;

	for (level = 0; level < 4; level++) {
	
		index = VADDR_TO_PDENT(va);
		if (pd[index].type != 0) {
			// no pagetable here
			return B_ERROR;
		}

		// map page table entry using our per CPU mapping page

		physicalPageTable = ADDR_REVERSE_SHIFT(pd[index].addr);
		pt = (page_table_entry *)(sQueryPage/* + cpu * SIZ_DIRTBL*/);
		index = VADDR_TO_PDENT((addr_t)pt);
		if (pd[index].present == 0) {
			// no page table here
			return B_ERROR;
		}

		index = VADDR_TO_PTENT((addr_t)pt);
		put_page_table_entry_in_pgtable(&sQueryPageTable[index], physicalPageTable,
			B_KERNEL_READ_AREA, false);
		invalidate_TLB(pt);

		index = VADDR_TO_PTENT(va);

		switch (level) {
			case 0: // root table
			case 1: // directory table
			case 2: // page table
				if (.type == DT_INDIRECT) {
					continue;
				}
				// FALLTHROUGH
			case 3: // indirect desc
		}
	}
	*_physical = ADDR_REVERSE_SHIFT(pt[index].addr);

	*_flags |= ((pt[index].rw ? B_KERNEL_WRITE_AREA : 0) | B_KERNEL_READ_AREA)
		| (pt[index].dirty ? PAGE_MODIFIED : 0)
		| (pt[index].accessed ? PAGE_ACCESSED : 0)
		| (pt[index].present ? PAGE_PRESENT : 0);

	return B_OK;
}


static status_t
query_tmap(vm_translation_map *map, addr_t va, addr_t *_physical, uint32 *_flags)
{
	page_table_entry *pt;
	page_directory_entry *pd = map->arch_data->rtdir_virt;
	status_t status;
	int32 index;

	// default the flags to not present
	*_flags = 0;
	*_physical = 0;

	index = VADDR_TO_PDENT(va);
	if (pd[index].present == 0) {
		// no pagetable here
		return B_NO_ERROR;
	}

	do {
		status = get_physical_page_tmap(ADDR_REVERSE_SHIFT(pd[index].addr),
			(addr_t *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while (status < B_OK);
	index = VADDR_TO_PTENT(va);

	*_physical = ADDR_REVERSE_SHIFT(pt[index].addr);

	// read in the page state flags
	if (pt[index].user)
		*_flags |= (pt[index].rw ? B_WRITE_AREA : 0) | B_READ_AREA;

	*_flags |= ((pt[index].rw ? B_KERNEL_WRITE_AREA : 0) | B_KERNEL_READ_AREA)
		| (pt[index].dirty ? PAGE_MODIFIED : 0)
		| (pt[index].accessed ? PAGE_ACCESSED : 0)
		| (pt[index].present ? PAGE_PRESENT : 0);

	put_physical_page_tmap((addr_t)pt);

	TRACE(("query_tmap: returning pa 0x%lx for va 0x%lx\n", *_physical, va));

	return B_OK;
}


static addr_t
get_mapped_size_tmap(vm_translation_map *map)
{
	return map->map_count;
}


static status_t
protect_tmap(vm_translation_map *map, addr_t start, addr_t end, uint32 attributes)
{
	page_table_entry *pt;
	page_directory_entry *pd = map->arch_data->rtdir_virt;
	status_t status;
	int index;

	start = ROUNDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

	TRACE(("protect_tmap: pages 0x%lx to 0x%lx, attributes %lx\n", start, end, attributes));

restart:
	if (start >= end)
		return B_OK;

	index = VADDR_TO_PDENT(start);
	if (pd[index].present == 0) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE);
		goto restart;
	}

	do {
		status = get_physical_page_tmap(ADDR_REVERSE_SHIFT(pd[index].addr),
				(addr_t *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while (status < B_OK);

	for (index = VADDR_TO_PTENT(start); index < 1024 && start < end; index++, start += B_PAGE_SIZE) {
		if (pt[index].present == 0) {
			// page mapping not valid
			continue;
		}

		TRACE(("protect_tmap: protect page 0x%lx\n", start));

		pt[index].user = (attributes & B_USER_PROTECTION) != 0;
		if ((attributes & B_USER_PROTECTION) != 0)
			pt[index].rw = (attributes & B_WRITE_AREA) != 0;
		else
			pt[index].rw = (attributes & B_KERNEL_WRITE_AREA) != 0;

		if (map->arch_data->num_invalidate_pages < PAGE_INVALIDATE_CACHE_SIZE)
			map->arch_data->pages_to_invalidate[map->arch_data->num_invalidate_pages] = start;

		map->arch_data->num_invalidate_pages++;
	}

	put_physical_page_tmap((addr_t)pt);

	goto restart;
}


static status_t
clear_flags_tmap(vm_translation_map *map, addr_t va, uint32 flags)
{
	page_table_entry *pt;
	page_directory_entry *pd = map->arch_data->rtdir_virt;
	status_t status;
	int index;
	int tlb_flush = false;

	index = VADDR_TO_PDENT(va);
	if (pd[index].present == 0) {
		// no pagetable here
		return B_OK;
	}

	do {
		status = get_physical_page_tmap(ADDR_REVERSE_SHIFT(pd[index].addr),
			(addr_t *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while (status < B_OK);
	index = VADDR_TO_PTENT(va);

	// clear out the flags we've been requested to clear
	if (flags & PAGE_MODIFIED) {
		pt[index].dirty = 0;
		tlb_flush = true;
	}
	if (flags & PAGE_ACCESSED) {
		pt[index].accessed = 0;
		tlb_flush = true;
	}

	put_physical_page_tmap((addr_t)pt);

	if (tlb_flush) {
		if (map->arch_data->num_invalidate_pages < PAGE_INVALIDATE_CACHE_SIZE)
			map->arch_data->pages_to_invalidate[map->arch_data->num_invalidate_pages] = va;

		map->arch_data->num_invalidate_pages++;
	}

	return B_OK;
}


static void
flush_tmap(vm_translation_map *map)
{
	cpu_status state;

	if (map->arch_data->num_invalidate_pages <= 0)
		return;

	state = disable_interrupts();

	if (map->arch_data->num_invalidate_pages > PAGE_INVALIDATE_CACHE_SIZE) {
		// invalidate all pages
		TRACE(("flush_tmap: %d pages to invalidate, invalidate all\n",
			map->arch_data->num_invalidate_pages));

		if (IS_KERNEL_MAP(map)) {
			arch_cpu_global_TLB_invalidate();
			smp_send_broadcast_ici(SMP_MSG_GLOBAL_INVALIDATE_PAGES, 0, 0, 0, NULL,
				SMP_MSG_FLAG_SYNC);
		} else {
			arch_cpu_user_TLB_invalidate();
			smp_send_broadcast_ici(SMP_MSG_USER_INVALIDATE_PAGES, 0, 0, 0, NULL,
				SMP_MSG_FLAG_SYNC);
		}
	} else {
		TRACE(("flush_tmap: %d pages to invalidate, invalidate list\n",
			map->arch_data->num_invalidate_pages));

		arch_cpu_invalidate_TLB_list(map->arch_data->pages_to_invalidate,
			map->arch_data->num_invalidate_pages);
		smp_send_broadcast_ici(SMP_MSG_INVALIDATE_PAGE_LIST,
			(uint32)map->arch_data->pages_to_invalidate,
			map->arch_data->num_invalidate_pages, 0, NULL,
			SMP_MSG_FLAG_SYNC);
	}
	map->arch_data->num_invalidate_pages = 0;

	restore_interrupts(state);
}


static status_t
map_iospace_chunk(addr_t va, addr_t pa)
{
	int i;
	page_table_entry *pt;
	addr_t ppn;
	int state;

	pa &= ~(B_PAGE_SIZE - 1); // make sure it's page aligned
	va &= ~(B_PAGE_SIZE - 1); // make sure it's page aligned
	if (va < sIOSpaceBase || va >= (sIOSpaceBase + IOSPACE_SIZE))
		panic("map_iospace_chunk: passed invalid va 0x%lx\n", va);

	ppn = ADDR_SHIFT(pa);
	pt = &iospace_pgtables[(va - sIOSpaceBase) / B_PAGE_SIZE];
	for (i = 0; i < 1024; i++) {
		init_page_table_entry(&pt[i]);
		pt[i].addr = ppn + i;
		pt[i].user = 0;
		pt[i].rw = 1;
		pt[i].present = 1;
		pt[i].global = 1;
	}

	state = disable_interrupts();
	arch_cpu_invalidate_TLB_range(va, va + (IOSPACE_CHUNK_SIZE - B_PAGE_SIZE));
	smp_send_broadcast_ici(SMP_MSG_INVALIDATE_PAGE_RANGE,
		va, va + (IOSPACE_CHUNK_SIZE - B_PAGE_SIZE), 0,
		NULL, SMP_MSG_FLAG_SYNC);
	restore_interrupts(state);

	return B_OK;
}


static status_t
get_physical_page_tmap(addr_t pa, addr_t *va, uint32 flags)
{
	return generic_get_physical_page(pa, va, flags);
}


static status_t
put_physical_page_tmap(addr_t va)
{
	return generic_put_physical_page(va);
}


static vm_translation_map_ops tmap_ops = {
	destroy_tmap,
	lock_tmap,
	unlock_tmap,
	map_max_pages_need,
	map_tmap,
	unmap_tmap,
	query_tmap,
	query_tmap_interrupt,
	get_mapped_size_tmap,
	protect_tmap,
	clear_flags_tmap,
	flush_tmap,
	get_physical_page_tmap,
	put_physical_page_tmap
};


//	#pragma mark -
//	VM API


static status_t
arch_vm_translation_map_init_map(vm_translation_map *map, bool kernel)
{
	if (map == NULL)
		return B_BAD_VALUE;

	TRACE(("vm_translation_map_create\n"));

	// initialize the new object
	map->ops = &tmap_ops;
	map->map_count = 0;

	if (!kernel) {
		// During the boot process, there are no semaphores available at this
		// point, so we only try to create the translation map lock if we're
		// initialize a user translation map.
		// vm_translation_map_init_kernel_map_post_sem() is used to complete
		// the kernel translation map.
		if (recursive_lock_init(&map->lock, "translation map") < B_OK)
			return map->lock.sem;
	}

	map->arch_data = (vm_translation_map_arch_info *)malloc(sizeof(vm_translation_map_arch_info));
	if (map == NULL) {
		recursive_lock_destroy(&map->lock);
		return B_NO_MEMORY;
	}

	map->arch_data->num_invalidate_pages = 0;

	if (!kernel) {
		// user
		// allocate a pgdir
		map->arch_data->rtdir_virt = (page_directory_entry *)memalign(
			B_PAGE_SIZE, B_PAGE_SIZE);
		if (map->arch_data->rtdir_virt == NULL) {
			free(map->arch_data);
			recursive_lock_destroy(&map->lock);
			return B_NO_MEMORY;
		}
		vm_get_page_mapping(vm_kernel_address_space_id(),
			(addr_t)map->arch_data->rtdir_virt, (addr_t *)&map->arch_data->rtdir_phys);
	} else {
		// kernel
		// we already know the kernel pgdir mapping
		map->arch_data->rtdir_virt = sKernelVirtualPageDirectory;
		map->arch_data->rtdir_phys = sKernelPhysicalPageDirectory;
	}

	// zero out the bottom portion of the new pgdir
	memset(map->arch_data->rtdir_virt + FIRST_USER_PGDIR_ENT, 0,
		NUM_USER_PGDIR_ENTS * sizeof(page_directory_entry));

	// insert this new map into the map list
	{
		int state = disable_interrupts();
		acquire_spinlock(&tmap_list_lock);

		// copy the top portion of the pgdir from the current one
		memcpy(map->arch_data->rtdir_virt + FIRST_KERNEL_PGDIR_ENT,
			sKernelVirtualPageDirectory + FIRST_KERNEL_PGDIR_ENT,
			NUM_KERNEL_PGDIR_ENTS * sizeof(page_directory_entry));

		map->next = tmap_list;
		tmap_list = map;

		release_spinlock(&tmap_list_lock);
		restore_interrupts(state);
	}

	return B_OK;
}


static status_t
arch_vm_translation_map_init_kernel_map_post_sem(vm_translation_map *map)
{
	if (recursive_lock_init(&map->lock, "translation map") < B_OK)
		return map->lock.sem;

	return B_OK;
}


static status_t
arch_vm_translation_map_init(kernel_args *args)
{
	status_t error;

	TRACE(("vm_translation_map_init: entry\n"));
#if 0
	// page hole set up in stage2
	page_hole = (page_table_entry *)args->arch_args.page_hole;
	// calculate where the pgdir would be
	page_hole_pgdir = (page_directory_entry *)(((unsigned int)args->arch_args.page_hole) + (B_PAGE_SIZE * 1024 - B_PAGE_SIZE));
	// clear out the bottom 2 GB, unmap everything
	memset(page_hole_pgdir + FIRST_USER_PGDIR_ENT, 0, sizeof(page_directory_entry) * NUM_USER_PGDIR_ENTS);
#endif
	
	sKernelPhysicalPageDirectory = (page_directory_entry *)args->arch_args.phys_pgdir;
	sKernelVirtualPageDirectory = (page_directory_entry *)args->arch_args.vir_pgdir;

	sQueryDesc.type = DT_INVALID;

	tmap_list_lock = 0;
	tmap_list = NULL;

	// allocate some space to hold physical page mapping info
	iospace_pgtables = (page_table_entry *)vm_allocate_early(args,
		B_PAGE_SIZE * (IOSPACE_SIZE / (B_PAGE_SIZE * 1024)), ~0L,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	TRACE(("iospace_pgtables %p\n", iospace_pgtables));

	// init physical page mapper
	error = generic_vm_physical_page_mapper_init(args, map_iospace_chunk,
		&sIOSpaceBase, IOSPACE_SIZE, IOSPACE_CHUNK_SIZE);
	if (error != B_OK)
		return error;

	// initialize our data structures
	memset(iospace_pgtables, 0, B_PAGE_SIZE * (IOSPACE_SIZE / (B_PAGE_SIZE * 1024)));

	TRACE(("mapping iospace_pgtables\n"));

	// put the array of pgtables directly into the kernel pagedir
	// these will be wired and kept mapped into virtual space to be easy to get to
	{
		addr_t phys_pgtable;
		addr_t virt_pgtable;
		page_directory_entry *e;
		int i;

		virt_pgtable = (addr_t)iospace_pgtables;
		for (i = 0; i < (IOSPACE_SIZE / (B_PAGE_SIZE * 1024)); i++, virt_pgtable += B_PAGE_SIZE) {
			early_query(virt_pgtable, &phys_pgtable);
			e = &page_hole_pgdir[(sIOSpaceBase / (B_PAGE_SIZE * 1024)) + i];
			put_pgtable_in_pgdir(e, phys_pgtable, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		}
	}

	// enable global page feature if available
	if (x86_check_feature(IA32_FEATURE_PGE, FEATURE_COMMON)) {
		// this prevents kernel pages from being flushed from TLB on context-switch
		x86_write_cr4(x86_read_cr4() | IA32_CR4_GLOBAL_PAGES);
	}

	TRACE(("vm_translation_map_init: done\n"));

	return B_OK;
}


static status_t
arch_vm_translation_map_init_post_sem(kernel_args *args)
{
	return generic_vm_physical_page_mapper_init_post_sem(args);
}


static status_t
arch_vm_translation_map_init_post_area(kernel_args *args)
{
	// now that the vm is initialized, create a region that represents
	// the page hole
	void *temp;
	status_t error;
	area_id area;

	TRACE(("vm_translation_map_init_post_area: entry\n"));

	// unmap the page hole hack we were using before
	sKernelVirtualPageDirectory[1023].present = 0;
	page_hole_pgdir = NULL;
	page_hole = NULL;

	temp = (void *)sKernelVirtualPageDirectory;
	area = create_area("kernel_pgdir", &temp, B_EXACT_ADDRESS, B_PAGE_SIZE,
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	temp = (void *)iospace_pgtables;
	area = create_area("iospace_pgtables", &temp, B_EXACT_ADDRESS,
		B_PAGE_SIZE * (IOSPACE_SIZE / (B_PAGE_SIZE * 1024)),
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	error = generic_vm_physical_page_mapper_init_post_area(args);
	if (error != B_OK)
		return error;

	// this area is used for query_tmap_interrupt()
	// TODO: Note, this only works as long as all pages belong to the same
	//	page table, which is not yet enforced (or even tested)!
	// Note we don't support SMP which makes things simpler.

	area = vm_create_null_area(vm_kernel_address_space_id(),
		"interrupt query pages", (void **)&sQueryPage, B_ANY_ADDRESS,
		B_PAGE_SIZE);
	if (area < B_OK)
		return area;

	// insert the indirect descriptor in the tree so we can map the page we want from it.

	{
		page_directory_entry *pageDirEntry;
		page_indirect_entry *pageTableEntry;
		addr_t physicalPageTable;
		int32 index;

		sQueryPageTable = (page_indirect_entry *)(sQueryPage);

		index = VADDR_TO_PRENT((addr_t)sQueryPageTable);
		physicalPageTable = ADDR_REVERSE_SHIFT(sKernelVirtualPageDirectory[index].addr);

		get_physical_page_tmap(physicalPageTable,
			(addr_t *)&pageTableEntry, PHYSICAL_PAGE_NO_WAIT);

		sQueryPageTable = (page_table_entry *)(sQueryPages);

		index = VADDR_TO_PDENT((addr_t)sQueryPageTable);
		physicalPageTable = ADDR_REVERSE_SHIFT(sKernelVirtualPageDirectory[index].addr);

		get_physical_page_tmap(physicalPageTable,
			(addr_t *)&pageTableEntry, PHYSICAL_PAGE_NO_WAIT);

		index = VADDR_TO_PTENT((addr_t)sQueryPageTable);
		put_page_table_entry_in_pgtable(&pageTableEntry[index], physicalPageTable,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, false);

		put_physical_page_tmap((addr_t)pageTableEntry);
		put_physical_page_tmap((addr_t)pageDirEntry);
		//invalidate_TLB(sQueryPageTable);
	}

	TRACE(("vm_translation_map_init_post_area: done\n"));
	return B_OK;
}


// XXX horrible back door to map a page quickly regardless of translation map object, etc.
// used only during VM setup.
// uses a 'page hole' set up in the stage 2 bootloader. The page hole is created by pointing one of
// the pgdir entries back at itself, effectively mapping the contents of all of the 4MB of pagetables
// into a 4 MB region. It's only used here, and is later unmapped.

static status_t
arch_vm_translation_map_early_map(kernel_args *args, addr_t va, addr_t pa,
	uint8 attributes, addr_t (*get_free_page)(kernel_args *))
{
	int index;

	TRACE(("early_tmap: entry pa 0x%lx va 0x%lx\n", pa, va));

	// check to see if a page table exists for this range
	index = VADDR_TO_PDENT(va);
	if (page_hole_pgdir[index].present == 0) {
		addr_t pgtable;
		page_directory_entry *e;
		// we need to allocate a pgtable
		pgtable = get_free_page(args);
		// pgtable is in pages, convert to physical address
		pgtable *= B_PAGE_SIZE;

		TRACE(("early_map: asked for free page for pgtable. 0x%lx\n", pgtable));

		// put it in the pgdir
		e = &page_hole_pgdir[index];
		put_pgtable_in_pgdir(e, pgtable, attributes);

		// zero it out in it's new mapping
		memset((unsigned int *)((unsigned int)page_hole + (va / B_PAGE_SIZE / 1024) * B_PAGE_SIZE), 0, B_PAGE_SIZE);
	}

	// now, fill in the pentry
	put_page_table_entry_in_pgtable(page_hole + va / B_PAGE_SIZE, pa, attributes,
		IS_KERNEL_ADDRESS(va));

	arch_cpu_invalidate_TLB_range(va, va);

	return B_OK;
}

