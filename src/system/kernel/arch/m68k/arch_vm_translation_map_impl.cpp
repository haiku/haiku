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
#include <heap.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <int.h>
#include <boot/kernel_args.h>
#include <arch/vm_translation_map.h>
#include <arch/cpu.h>
#include <arch_mmu.h>
#include <stdlib.h>

#include "generic_vm_physical_page_mapper.h"
#include "generic_vm_physical_page_ops.h"


#define TRACE_VM_TMAP
#ifdef TRACE_VM_TMAP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// 4 MB of iospace
//#define IOSPACE_SIZE (4*1024*1024)
#define IOSPACE_SIZE (16*1024*1024)
// 256K = 2^6*4K
#define IOSPACE_CHUNK_SIZE (NUM_PAGEENT_PER_TBL*B_PAGE_SIZE)

static page_table_entry *iospace_pgtables = NULL;

#define PAGE_INVALIDATE_CACHE_SIZE 64

// vm_translation object stuff
typedef struct vm_translation_map_arch_info {
	page_root_entry *rtdir_virt;
	page_root_entry *rtdir_phys;
	int num_invalidate_pages;
	addr_t pages_to_invalidate[PAGE_INVALIDATE_CACHE_SIZE];
} vm_translation_map_arch_info;

#if 1//XXX:HOLE
static page_table_entry *page_hole = NULL;
static page_directory_entry *page_hole_pgdir = NULL;
#endif
static page_root_entry *sKernelPhysicalPageRoot = NULL;
static page_root_entry *sKernelVirtualPageRoot = NULL;
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
static status_t get_physical_page_tmap_internal(addr_t pa, addr_t *va, uint32 flags);
static status_t put_physical_page_tmap_internal(addr_t va);

static void flush_tmap(vm_translation_map *map);


#warning M68K: RENAME
static void *
_m68k_translation_map_get_pgdir(vm_translation_map *map)
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


static inline void
init_page_indirect_entry(page_indirect_entry *entry)
{
#warning M68K: is it correct ?
	*(page_indirect_entry_scalar *)entry = DFL_PAGEENT_VAL;
}


static inline void
update_page_indirect_entry(page_indirect_entry *entry, page_indirect_entry *with)
{
	// update page table entry atomically
	// XXX: is it ?? (long desc?)
	*(page_indirect_entry_scalar *)entry = *(page_indirect_entry_scalar *)with;
}


#warning M68K: allocate all kernel pgdirs at boot and remove this (also dont remove them anymore from unmap)
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


// this is used before the vm is fully up, it uses the
// transparent translation of the first 256MB
// a set up by the bootloader.
static status_t
early_query(addr_t va, addr_t *_physicalAddress)
{
	page_root_entry *pr = sKernelVirtualPageRoot;
	page_directory_entry *pd;
	page_indirect_entry *pi;
	page_table_entry *pt;
	addr_t pa;
	int32 index;
	status_t err = B_ERROR;	// no pagetable here
	TRACE(("%s(%p,)\n", __FUNCTION__, va));

	index = VADDR_TO_PRENT(va);
	TRACE(("%s: pr[%d].type %d\n", __FUNCTION__, index, pr[index].type));
	if (pr && pr[index].type == DT_ROOT) {
		pa = PRE_TO_TA(pr[index]);
		// pa == va when in TT
		// and no need to fiddle with cache
		pd = (page_directory_entry *)pa;

		index = VADDR_TO_PDENT(va);
		TRACE(("%s: pd[%d].type %d\n", __FUNCTION__, index,
				pd?(pd[index].type):-1));
		if (pd && pd[index].type == DT_DIR) {
			pa = PDE_TO_TA(pd[index]);
			pt = (page_table_entry *)pa;

			index = VADDR_TO_PTENT(va);
			TRACE(("%s: pt[%d].type %d\n", __FUNCTION__, index,
					pt?(pt[index].type):-1));
			if (pt && pt[index].type == DT_INDIRECT) {
				pi = (page_indirect_entry *)pt;
				pa = PIE_TO_TA(pi[index]);
				pt = (page_table_entry *)pa;
				index = 0; // single descriptor
			}

			if (pt && pt[index].type == DT_PAGE) {
				*_physicalAddress = PTE_TO_PA(pt[index]);
				// we should only be passed page va, but just in case.
				*_physicalAddress += va % B_PAGE_SIZE;
				err = B_OK;
			}
		}
	}

	return err;
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
		// since the size of tables don't match B_PAGE_SIZE,
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
			pgdir_pn = PRE_TO_PN(map->arch_data->rtdir_virt[i]);
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
				DEBUG_PAGE_ACCESS_START(page);
				vm_page_set_state(page, PAGE_STATE_FREE);
			}
			if (((i + 1) % NUM_DIRTBL_PER_PAGE) == 0) {
				DEBUG_PAGE_ACCESS_END(dirpage);
				vm_page_set_state(dirpage, PAGE_STATE_FREE);
			}
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


static void
put_page_indirect_entry_in_pgtable(page_indirect_entry *entry,
	addr_t physicalAddress, uint32 attributes, bool globalPage)
{
	page_indirect_entry page;
	init_page_indirect_entry(&page);

	page.addr = TA_TO_PIEA(physicalAddress);
	page.type = DT_INDIRECT;

	// there are no protection bits in indirect descriptor usually.

	// put it in the page table
	update_page_indirect_entry(entry, &page);
}


static size_t
map_max_pages_need(vm_translation_map */*map*/, addr_t start, addr_t end)
{
	size_t need;
	size_t pgdirs;
	// If start == 0, the actual base address is not yet known to the caller
	// and we shall assume the worst case.
	if (start == 0) {
#warning M68K: FIXME?
		start = (1023) * B_PAGE_SIZE;
		end += start;
	}
	pgdirs = VADDR_TO_PRENT(end) + 1 - VADDR_TO_PRENT(start);
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
		page = vm_page_allocate_page(PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

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
		err = get_physical_page_tmap_internal(PRE_TO_PA(pr[rindex]),
				&pd_pg, PHYSICAL_PAGE_DONT_WAIT);
	} while (err < 0);
	pd = (page_directory_entry *)pd_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pd += (rindex % NUM_DIRTBL_PER_PAGE) * NUM_DIRENT_PER_TBL;

	// check to see if a page table exists for this range
	dindex = VADDR_TO_PDENT(va);
	if (pd[dindex].type != DT_DIR) {
		addr_t pgtable;
		vm_page *page;
		unsigned int i;

		// we need to allocate a pgtable
		page = vm_page_allocate_page(PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

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
		err = get_physical_page_tmap_internal(PDE_TO_PA(pd[dindex]),
				&pt_pg, PHYSICAL_PAGE_DONT_WAIT);
	} while (err < 0);
	pt = (page_table_entry *)pt_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pt += (dindex % NUM_PAGETBL_PER_PAGE) * NUM_PAGEENT_PER_TBL;

	pindex = VADDR_TO_PTENT(va);

	put_page_table_entry_in_pgtable(&pt[pindex], pa, attributes,
		IS_KERNEL_MAP(map));

	put_physical_page_tmap_internal(pt_pg);
	put_physical_page_tmap_internal(pd_pg);

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

	start = ROUNDDOWN(start, B_PAGE_SIZE);
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
		status = get_physical_page_tmap_internal(PRE_TO_PA(pr[index]),
			&pd_pg, PHYSICAL_PAGE_DONT_WAIT);
	} while (status < B_OK);
	pd = (page_directory_entry *)pd_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pd += (index % NUM_DIRTBL_PER_PAGE) * NUM_DIRENT_PER_TBL;

	index = VADDR_TO_PDENT(start);
	if (pd[index].type != DT_DIR) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE);
		put_physical_page_tmap_internal(pd_pg);
		goto restart;
	}

	do {
		status = get_physical_page_tmap_internal(PDE_TO_PA(pd[index]),
			&pt_pg, PHYSICAL_PAGE_DONT_WAIT);
	} while (status < B_OK);
	pt = (page_table_entry *)pt_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pt += (index % NUM_PAGETBL_PER_PAGE) * NUM_PAGEENT_PER_TBL;

	for (index = VADDR_TO_PTENT(start);
			(index < NUM_PAGEENT_PER_TBL) && (start < end);
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

	put_physical_page_tmap_internal(pt_pg);
	put_physical_page_tmap_internal(pd_pg);

	goto restart;
}

// XXX: 040 should be able to do that with PTEST (but not 030 or 060)
static status_t
query_tmap_interrupt(vm_translation_map *map, addr_t va, addr_t *_physical,
	uint32 *_flags)
{
	page_root_entry *pr = map->arch_data->rtdir_virt;
	page_directory_entry *pd;
	page_indirect_entry *pi;
	page_table_entry *pt;
	addr_t physicalPageTable;
	int32 index;
	status_t err = B_ERROR;	// no pagetable here

	if (sQueryPage == NULL)
		return err; // not yet initialized !?

	index = VADDR_TO_PRENT(va);
	if (pr && pr[index].type == DT_ROOT) {
		put_page_table_entry_in_pgtable(&sQueryDesc, PRE_TO_TA(pr[index]), B_KERNEL_READ_AREA, false);
		arch_cpu_invalidate_TLB_range((addr_t)pt, (addr_t)pt);
		pd = (page_directory_entry *)sQueryPage;

		index = VADDR_TO_PDENT(va);
		if (pd && pd[index].type == DT_DIR) {
			put_page_table_entry_in_pgtable(&sQueryDesc, PDE_TO_TA(pd[index]), B_KERNEL_READ_AREA, false);
			arch_cpu_invalidate_TLB_range((addr_t)pt, (addr_t)pt);
			pt = (page_table_entry *)sQueryPage;

			index = VADDR_TO_PTENT(va);
			if (pt && pt[index].type == DT_INDIRECT) {
				pi = (page_indirect_entry *)pt;
				put_page_table_entry_in_pgtable(&sQueryDesc, PIE_TO_TA(pi[index]), B_KERNEL_READ_AREA, false);
				arch_cpu_invalidate_TLB_range((addr_t)pt, (addr_t)pt);
				pt = (page_table_entry *)sQueryPage;
				index = 0; // single descriptor
			}

			if (pt /*&& pt[index].type == DT_PAGE*/) {
				*_physical = PTE_TO_PA(pt[index]);
				// we should only be passed page va, but just in case.
				*_physical += va % B_PAGE_SIZE;
				*_flags |= ((pt[index].write_protect ? 0 : B_KERNEL_WRITE_AREA) | B_KERNEL_READ_AREA)
						| (pt[index].dirty ? PAGE_MODIFIED : 0)
						| (pt[index].accessed ? PAGE_ACCESSED : 0)
						| ((pt[index].type == DT_PAGE) ? PAGE_PRESENT : 0);
				err = B_OK;
			}
		}
	}

	// unmap the pg table from the indirect desc.
	sQueryDesc.type = DT_INVALID;

	return err;
}


static status_t
query_tmap(vm_translation_map *map, addr_t va, addr_t *_physical, uint32 *_flags)
{
	page_table_entry *pt;
	page_indirect_entry *pi;
	page_directory_entry *pd;
	page_directory_entry *pr = map->arch_data->rtdir_virt;
	addr_t pd_pg, pt_pg, pi_pg;
	status_t status;
	int32 index;

	// default the flags to not present
	*_flags = 0;
	*_physical = 0;

	index = VADDR_TO_PRENT(va);
	if (pr[index].type != DT_ROOT) {
		// no pagetable here
		return B_NO_ERROR;
	}

	do {
		status = get_physical_page_tmap_internal(PRE_TO_PA(pr[index]),
			&pd_pg, PHYSICAL_PAGE_DONT_WAIT);
	} while (status < B_OK);
	pd = (page_directory_entry *)pd_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pd += (index % NUM_DIRTBL_PER_PAGE) * NUM_DIRENT_PER_TBL;


	index = VADDR_TO_PDENT(va);
	if (pd[index].type != DT_DIR) {
		// no pagetable here
		put_physical_page_tmap_internal(pd_pg);
		return B_NO_ERROR;
	}

	do {
		status = get_physical_page_tmap_internal(PDE_TO_PA(pd[index]),
			&pt_pg, PHYSICAL_PAGE_DONT_WAIT);
	} while (status < B_OK);
	pt = (page_table_entry *)pt_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pt += (index % NUM_PAGETBL_PER_PAGE) * NUM_PAGEENT_PER_TBL;

	index = VADDR_TO_PTENT(va);

	// handle indirect descriptor
	if (pt[index].type == DT_INDIRECT) {
		pi = (page_indirect_entry *)pt;
		pi_pg = pt_pg;
		do {
			status = get_physical_page_tmap_internal(PIE_TO_PA(pi[index]),
				&pt_pg, PHYSICAL_PAGE_DONT_WAIT);
		} while (status < B_OK);
		pt = (page_table_entry *)pt_pg;
		// add offset from start of page
		pt += PIE_TO_PO(pi[index]) / sizeof(page_table_entry);
		// release the indirect table page
		put_physical_page_tmap_internal(pi_pg);
	}

	*_physical = PTE_TO_PA(pt[index]);

	// read in the page state flags
	if (!pt[index].supervisor)
		*_flags |= (pt[index].write_protect ? 0 : B_WRITE_AREA) | B_READ_AREA;

	*_flags |= (pt[index].write_protect ? 0 : B_KERNEL_WRITE_AREA)
		| B_KERNEL_READ_AREA
		| (pt[index].dirty ? PAGE_MODIFIED : 0)
		| (pt[index].accessed ? PAGE_ACCESSED : 0)
		| ((pt[index].type == DT_PAGE) ? PAGE_PRESENT : 0);

	put_physical_page_tmap_internal(pt_pg);
	put_physical_page_tmap_internal(pd_pg);

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
	page_directory_entry *pd;
	page_root_entry *pr = map->arch_data->rtdir_virt;
	addr_t pd_pg, pt_pg;
	status_t status;
	int index;

	start = ROUNDDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

	TRACE(("protect_tmap: pages 0x%lx to 0x%lx, attributes %lx\n", start, end, attributes));

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
		status = get_physical_page_tmap_internal(PRE_TO_PA(pr[index]),
			&pd_pg, PHYSICAL_PAGE_DONT_WAIT);
	} while (status < B_OK);
	pd = (page_directory_entry *)pd_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pd += (index % NUM_DIRTBL_PER_PAGE) * NUM_DIRENT_PER_TBL;

	index = VADDR_TO_PDENT(start);
	if (pd[index].type != DT_DIR) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE);
		put_physical_page_tmap_internal(pd_pg);
		goto restart;
	}

	do {
		status = get_physical_page_tmap_internal(PDE_TO_PA(pd[index]),
			&pt_pg, PHYSICAL_PAGE_DONT_WAIT);
	} while (status < B_OK);
	pt = (page_table_entry *)pt_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pt += (index % NUM_PAGETBL_PER_PAGE) * NUM_PAGEENT_PER_TBL;

	for (index = VADDR_TO_PTENT(start);
			(index < NUM_PAGEENT_PER_TBL) && (start < end);
			index++, start += B_PAGE_SIZE) {
		// XXX: handle indirect ?
		if (pt[index].type != DT_PAGE /*&& pt[index].type != DT_INDIRECT*/) {
			// page mapping not valid
			continue;
		}

		TRACE(("protect_tmap: protect page 0x%lx\n", start));

		pt[index].supervisor = (attributes & B_USER_PROTECTION) == 0;
		if ((attributes & B_USER_PROTECTION) != 0)
			pt[index].write_protect = (attributes & B_WRITE_AREA) == 0;
		else
			pt[index].write_protect = (attributes & B_KERNEL_WRITE_AREA) == 0;

		if (map->arch_data->num_invalidate_pages < PAGE_INVALIDATE_CACHE_SIZE)
			map->arch_data->pages_to_invalidate[map->arch_data->num_invalidate_pages] = start;

		map->arch_data->num_invalidate_pages++;
	}

	put_physical_page_tmap_internal(pt_pg);
	put_physical_page_tmap_internal(pd_pg);

	goto restart;
}


static status_t
clear_flags_tmap(vm_translation_map *map, addr_t va, uint32 flags)
{
	page_table_entry *pt;
	page_indirect_entry *pi;
	page_directory_entry *pd;
	page_root_entry *pr = map->arch_data->rtdir_virt;
	addr_t pd_pg, pt_pg, pi_pg;
	status_t status;
	int index;
	int tlb_flush = false;

	index = VADDR_TO_PRENT(va);
	if (pr[index].type != DT_ROOT) {
		// no pagetable here
		return B_NO_ERROR;
	}

	do {
		status = get_physical_page_tmap_internal(PRE_TO_PA(pr[index]),
			&pd_pg, PHYSICAL_PAGE_DONT_WAIT);
	} while (status < B_OK);
	pd = (page_directory_entry *)pd_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pd += (index % NUM_DIRTBL_PER_PAGE) * NUM_DIRENT_PER_TBL;


	index = VADDR_TO_PDENT(va);
	if (pd[index].type != DT_DIR) {
		// no pagetable here
		put_physical_page_tmap_internal(pd_pg);
		return B_NO_ERROR;
	}

	do {
		status = get_physical_page_tmap_internal(PDE_TO_PA(pd[index]),
			&pt_pg, PHYSICAL_PAGE_DONT_WAIT);
	} while (status < B_OK);
	pt = (page_table_entry *)pt_pg;
	// we want the table at rindex, not at rindex%(tbl/page)
	pt += (index % NUM_PAGETBL_PER_PAGE) * NUM_PAGEENT_PER_TBL;

	index = VADDR_TO_PTENT(va);

	// handle indirect descriptor
	if (pt[index].type == DT_INDIRECT) {
		pi = (page_indirect_entry *)pt;
		pi_pg = pt_pg;
		do {
			status = get_physical_page_tmap_internal(PIE_TO_PA(pi[index]),
				&pt_pg, PHYSICAL_PAGE_DONT_WAIT);
		} while (status < B_OK);
		pt = (page_table_entry *)pt_pg;
		// add offset from start of page
		pt += PIE_TO_PO(pi[index]) / sizeof(page_table_entry);
		// release the indirect table page
		put_physical_page_tmap_internal(pi_pg);
	}

	// clear out the flags we've been requested to clear
	if (flags & PAGE_MODIFIED) {
		pt[index].dirty = 0;
		tlb_flush = true;
	}
	if (flags & PAGE_ACCESSED) {
		pt[index].accessed = 0;
		tlb_flush = true;
	}

	put_physical_page_tmap_internal(pt_pg);
	put_physical_page_tmap_internal(pd_pg);

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
		} else {
			arch_cpu_user_TLB_invalidate();
		}
	} else {
		TRACE(("flush_tmap: %d pages to invalidate, invalidate list\n",
			map->arch_data->num_invalidate_pages));

		arch_cpu_invalidate_TLB_list(map->arch_data->pages_to_invalidate,
			map->arch_data->num_invalidate_pages);
	}
	map->arch_data->num_invalidate_pages = 0;

	restore_interrupts(state);
}


static status_t
map_iospace_chunk(addr_t va, addr_t pa, uint32 flags)
{
	int i;
	page_table_entry *pt;
	int state;

	pa &= ~(B_PAGE_SIZE - 1); // make sure it's page aligned
	va &= ~(B_PAGE_SIZE - 1); // make sure it's page aligned
	if (va < sIOSpaceBase || va >= (sIOSpaceBase + IOSPACE_SIZE))
		panic("map_iospace_chunk: passed invalid va 0x%lx\n", va);

	pt = &iospace_pgtables[(va - sIOSpaceBase) / B_PAGE_SIZE];
	for (i = 0; i < NUM_PAGEENT_PER_TBL; i++, pa += B_PAGE_SIZE) {
		init_page_table_entry(&pt[i]);
		pt[i].addr = TA_TO_PTEA(pa);
		pt[i].supervisor = 1;
		pt[i].write_protect = 0;
		pt[i].type = DT_PAGE;
		//XXX: not cachable ?
		// 040 or 060 only
#ifdef MMU_HAS_GLOBAL_PAGES
		pt[i].global = 1;
#endif
	}

	state = disable_interrupts();
	arch_cpu_invalidate_TLB_range(va, va + (IOSPACE_CHUNK_SIZE - B_PAGE_SIZE));
	//smp_send_broadcast_ici(SMP_MSG_INVALIDATE_PAGE_RANGE,
	//	va, va + (IOSPACE_CHUNK_SIZE - B_PAGE_SIZE), 0,
	//	NULL, SMP_MSG_FLAG_SYNC);
	restore_interrupts(state);

	return B_OK;
}


static status_t
get_physical_page_tmap_internal(addr_t pa, addr_t *va, uint32 flags)
{
	return generic_get_physical_page(pa, va, flags);
}


static status_t
put_physical_page_tmap_internal(addr_t va)
{
	return generic_put_physical_page(va);
}


static status_t
get_physical_page_tmap(addr_t physicalAddress, addr_t *_virtualAddress,
	void **handle)
{
	return generic_get_physical_page(physicalAddress, _virtualAddress, 0);
}


static status_t
put_physical_page_tmap(addr_t virtualAddress, void *handle)
{
	return generic_put_physical_page(virtualAddress);
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
	put_physical_page_tmap,
	get_physical_page_tmap,	// *_current_cpu()
	put_physical_page_tmap,	// *_current_cpu()
	get_physical_page_tmap,	// *_debug()
	put_physical_page_tmap,	// *_debug()
		// TODO: Replace the *_current_cpu() and *_debug() versions!

	generic_vm_memset_physical,
	generic_vm_memcpy_from_physical,
	generic_vm_memcpy_to_physical,
	generic_vm_memcpy_physical_page
		// TODO: Verify that this is safe to use!
};


//	#pragma mark -
//	VM API


static status_t
m68k_vm_translation_map_init_map(vm_translation_map *map, bool kernel)
{
	if (map == NULL)
		return B_BAD_VALUE;

	TRACE(("vm_translation_map_create\n"));

	// initialize the new object
	map->ops = &tmap_ops;
	map->map_count = 0;

	recursive_lock_init(&map->lock, "translation map");

	map->arch_data = (vm_translation_map_arch_info *)malloc(sizeof(vm_translation_map_arch_info));
	if (map == NULL) {
		recursive_lock_destroy(&map->lock);
		return B_NO_MEMORY;
	}

	map->arch_data->num_invalidate_pages = 0;

	if (!kernel) {
		// user
		// allocate a rtdir
		map->arch_data->rtdir_virt = (page_root_entry *)memalign(
			SIZ_ROOTTBL, SIZ_ROOTTBL);
		if (map->arch_data->rtdir_virt == NULL) {
			free(map->arch_data);
			recursive_lock_destroy(&map->lock);
			return B_NO_MEMORY;
		}
		vm_get_page_mapping(VMAddressSpace::KernelID(),
			(addr_t)map->arch_data->rtdir_virt, (addr_t *)&map->arch_data->rtdir_phys);
	} else {
		// kernel
		// we already know the kernel pgdir mapping
		map->arch_data->rtdir_virt = sKernelVirtualPageRoot;
		map->arch_data->rtdir_phys = sKernelPhysicalPageRoot;
	}

	// zero out the bottom portion of the new rtdir
	memset(map->arch_data->rtdir_virt + FIRST_USER_PGROOT_ENT, 0,
		NUM_USER_PGROOT_ENTS * sizeof(page_root_entry));

	// insert this new map into the map list
	{
		int state = disable_interrupts();
		acquire_spinlock(&tmap_list_lock);

		// copy the top portion of the rtdir from the current one
		memcpy(map->arch_data->rtdir_virt + FIRST_KERNEL_PGROOT_ENT,
			sKernelVirtualPageRoot + FIRST_KERNEL_PGROOT_ENT,
			NUM_KERNEL_PGROOT_ENTS * sizeof(page_root_entry));

		map->next = tmap_list;
		tmap_list = map;

		release_spinlock(&tmap_list_lock);
		restore_interrupts(state);
	}

	return B_OK;
}


static status_t
m68k_vm_translation_map_init_kernel_map_post_sem(vm_translation_map *map)
{
	return B_OK;
}


static status_t
m68k_vm_translation_map_init(kernel_args *args)
{
	status_t error;

	TRACE(("vm_translation_map_init: entry\n"));
#if 0//XXX:HOLE
	// page hole set up in stage2
	page_hole = (page_table_entry *)args->arch_args.page_hole;
	// calculate where the pgdir would be
	page_hole_pgdir = (page_directory_entry *)(((unsigned int)args->arch_args.page_hole) + (B_PAGE_SIZE * 1024 - B_PAGE_SIZE));
	// clear out the bottom 2 GB, unmap everything
	memset(page_hole_pgdir + FIRST_USER_PGDIR_ENT, 0, sizeof(page_directory_entry) * NUM_USER_PGDIR_ENTS);
#endif

	sKernelPhysicalPageRoot = (page_root_entry *)args->arch_args.phys_pgroot;
	sKernelVirtualPageRoot = (page_root_entry *)args->arch_args.vir_pgroot;

	sQueryDesc.type = DT_INVALID;

	B_INITIALIZE_SPINLOCK(&tmap_list_lock);
	tmap_list = NULL;

	// allocate some space to hold physical page mapping info
	//XXX: check page count
	// we already have all page directories allocated by the bootloader,
	// we only need page tables

	iospace_pgtables = (page_table_entry *)vm_allocate_early(args,
		B_PAGE_SIZE * (IOSPACE_SIZE / (B_PAGE_SIZE * NUM_PAGEENT_PER_TBL * NUM_PAGETBL_PER_PAGE)), ~0L,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0);

	TRACE(("iospace_pgtables %p\n", iospace_pgtables));

	// init physical page mapper
	error = generic_vm_physical_page_mapper_init(args, map_iospace_chunk,
		&sIOSpaceBase, IOSPACE_SIZE, IOSPACE_CHUNK_SIZE);
	if (error != B_OK)
		return error;
	TRACE(("iospace at %p\n", sIOSpaceBase));
	// initialize our data structures
	memset(iospace_pgtables, 0, B_PAGE_SIZE * (IOSPACE_SIZE / (B_PAGE_SIZE * NUM_PAGEENT_PER_TBL * NUM_PAGETBL_PER_PAGE)));

	TRACE(("mapping iospace_pgtables\n"));

	// put the array of pgtables directly into the kernel pagedir
	// these will be wired and kept mapped into virtual space to be
	// easy to get to.
	// note the bootloader allocates all page directories for us
	// as a contiguous block.
	// we also still have transparent translation enabled, va==pa.
	{
		addr_t phys_pgtable;
		addr_t virt_pgtable;
		page_root_entry *pr = sKernelVirtualPageRoot;
		page_directory_entry *pd;
		page_directory_entry *e;
		int index;
		int i;

		virt_pgtable = (addr_t)iospace_pgtables;

		for (i = 0; i < (IOSPACE_SIZE / (B_PAGE_SIZE * NUM_PAGEENT_PER_TBL));
			 i++, virt_pgtable += SIZ_PAGETBL) {
			// early_query handles non-page-aligned addresses
			early_query(virt_pgtable, &phys_pgtable);
			index = VADDR_TO_PRENT(sIOSpaceBase) + i / NUM_DIRENT_PER_TBL;
			pd = (page_directory_entry *)PRE_TO_TA(pr[index]);
			e = &pd[(VADDR_TO_PDENT(sIOSpaceBase) + i) % NUM_DIRENT_PER_TBL];
			put_pgtable_in_pgdir(e, phys_pgtable,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		}
	}

	TRACE(("vm_translation_map_init: done\n"));

	return B_OK;
}


static status_t
m68k_vm_translation_map_init_post_sem(kernel_args *args)
{
	return generic_vm_physical_page_mapper_init_post_sem(args);
}


static status_t
m68k_vm_translation_map_init_post_area(kernel_args *args)
{
	// now that the vm is initialized, create a region that represents
	// the page hole
	void *temp;
	status_t error;
	area_id area;
	addr_t queryPage;

	TRACE(("vm_translation_map_init_post_area: entry\n"));

	// unmap the page hole hack we were using before
#warning M68K: FIXME
	//sKernelVirtualPageRoot[1023].present = 0;
#if 0
	page_hole_pgdir = NULL;
	page_hole = NULL;
#endif

	temp = (void *)sKernelVirtualPageRoot;
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

	area = vm_create_null_area(VMAddressSpace::KernelID(),
		"interrupt query pages", (void **)&queryPage, B_ANY_ADDRESS,
		B_PAGE_SIZE, 0);
	if (area < B_OK)
		return area;

	// insert the indirect descriptor in the tree so we can map the page we want from it.

	{
		page_directory_entry *pageDirEntry;
		page_indirect_entry *pageTableEntry;
		addr_t physicalPageDir, physicalPageTable;
		addr_t physicalIndirectDesc;
		int32 index;

		// first get pa for the indirect descriptor

		index = VADDR_TO_PRENT((addr_t)&sQueryDesc);
		physicalPageDir = PRE_TO_PA(sKernelVirtualPageRoot[index]);

		get_physical_page_tmap_internal(physicalPageDir,
			(addr_t *)&pageDirEntry, PHYSICAL_PAGE_DONT_WAIT);

		index = VADDR_TO_PDENT((addr_t)&sQueryDesc);
		physicalPageTable = PDE_TO_PA(pageDirEntry[index]);

		get_physical_page_tmap_internal(physicalPageTable,
			(addr_t *)&pageTableEntry, PHYSICAL_PAGE_DONT_WAIT);

		index = VADDR_TO_PTENT((addr_t)&sQueryDesc);

		// pa of the page
		physicalIndirectDesc = PTE_TO_PA(pageTableEntry[index]);
		// add offset
		physicalIndirectDesc += ((addr_t)&sQueryDesc) % B_PAGE_SIZE;

		put_physical_page_tmap_internal((addr_t)pageTableEntry);
		put_physical_page_tmap_internal((addr_t)pageDirEntry);

		// then the va for the page table for the query page.

		//sQueryPageTable = (page_indirect_entry *)(queryPage);

		index = VADDR_TO_PRENT(queryPage);
		physicalPageDir = PRE_TO_PA(sKernelVirtualPageRoot[index]);

		get_physical_page_tmap_internal(physicalPageDir,
			(addr_t *)&pageDirEntry, PHYSICAL_PAGE_DONT_WAIT);

		index = VADDR_TO_PDENT(queryPage);
		physicalPageTable = PDE_TO_PA(pageDirEntry[index]);

		get_physical_page_tmap_internal(physicalPageTable,
			(addr_t *)&pageTableEntry, PHYSICAL_PAGE_DONT_WAIT);

		index = VADDR_TO_PTENT(queryPage);

		put_page_indirect_entry_in_pgtable(&pageTableEntry[index], physicalIndirectDesc,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, false);

		put_physical_page_tmap_internal((addr_t)pageTableEntry);
		put_physical_page_tmap_internal((addr_t)pageDirEntry);
		//invalidate_TLB(sQueryPageTable);
	}
	// qmery_tmap_interrupt checks for the NULL, now it can use it
	sQueryPage = queryPage;

	TRACE(("vm_translation_map_init_post_area: done\n"));
	return B_OK;
}


// almost directly taken from boot mmu code
// x86:
// XXX horrible back door to map a page quickly regardless of translation map object, etc.
// used only during VM setup.
// uses a 'page hole' set up in the stage 2 bootloader. The page hole is created by pointing one of
// the pgdir entries back at itself, effectively mapping the contents of all of the 4MB of pagetables
// into a 4 MB region. It's only used here, and is later unmapped.

static status_t
m68k_vm_translation_map_early_map(kernel_args *args, addr_t va, addr_t pa,
	uint8 attributes, addr_t (*get_free_page)(kernel_args *))
{
	page_root_entry *pr = (page_root_entry *)sKernelPhysicalPageRoot;
	page_directory_entry *pd;
	page_table_entry *pt;
	addr_t tbl;
	uint32 index;
	uint32 i;
	TRACE(("early_tmap: entry pa 0x%lx va 0x%lx\n", pa, va));

	// everything much simpler here because pa = va
	// thanks to transparent translation which hasn't been disabled yet

	index = VADDR_TO_PRENT(va);
	if (pr[index].type != DT_ROOT) {
		unsigned aindex = index & ~(NUM_DIRTBL_PER_PAGE-1); /* aligned */
		TRACE(("missing page root entry %d ai %d\n", index, aindex));
		tbl = get_free_page(args) * B_PAGE_SIZE;
		if (!tbl)
			return ENOMEM;
		TRACE(("early_map: asked for free page for pgdir. 0x%lx\n", tbl));
		// zero-out
		memset((void *)tbl, 0, B_PAGE_SIZE);
		// for each pgdir on the allocated page:
		for (i = 0; i < NUM_DIRTBL_PER_PAGE; i++) {
			put_pgdir_in_pgroot(&pr[aindex + i], tbl, attributes);
			//TRACE(("inserting tbl @ %p as %08x pr[%d] %08x\n", tbl, TA_TO_PREA(tbl), aindex + i, *(uint32 *)apr));
			// clear the table
			//TRACE(("clearing table[%d]\n", i));
			pd = (page_directory_entry *)tbl;
			for (int32 j = 0; j < NUM_DIRENT_PER_TBL; j++)
				*(page_directory_entry_scalar *)(&pd[j]) = DFL_DIRENT_VAL;
			tbl += SIZ_DIRTBL;
		}
	}
	pd = (page_directory_entry *)PRE_TO_TA(pr[index]);

	index = VADDR_TO_PDENT(va);
	if (pd[index].type != DT_DIR) {
		unsigned aindex = index & ~(NUM_PAGETBL_PER_PAGE-1); /* aligned */
		TRACE(("missing page dir entry %d ai %d\n", index, aindex));
		tbl = get_free_page(args) * B_PAGE_SIZE;
		if (!tbl)
			return ENOMEM;
		TRACE(("early_map: asked for free page for pgtable. 0x%lx\n", tbl));
		// zero-out
		memset((void *)tbl, 0, B_PAGE_SIZE);
		// for each pgdir on the allocated page:
		for (i = 0; i < NUM_PAGETBL_PER_PAGE; i++) {
			put_pgtable_in_pgdir(&pd[aindex + i], tbl, attributes);
			// clear the table
			//TRACE(("clearing table[%d]\n", i));
			pt = (page_table_entry *)tbl;
			for (int32 j = 0; j < NUM_PAGEENT_PER_TBL; j++)
				*(page_table_entry_scalar *)(&pt[j]) = DFL_PAGEENT_VAL;
			tbl += SIZ_PAGETBL;
		}
	}
	pt = (page_table_entry *)PDE_TO_TA(pd[index]);

	index = VADDR_TO_PTENT(va);
	put_page_table_entry_in_pgtable(&pt[index], pa, attributes,
		IS_KERNEL_ADDRESS(va));

	arch_cpu_invalidate_TLB_range(va, va);

	return B_OK;
}


static bool
m68k_vm_translation_map_is_kernel_page_accessible(addr_t virtualAddress,
	uint32 protection)
{
	// TODO: Implement!
	return false;
}
