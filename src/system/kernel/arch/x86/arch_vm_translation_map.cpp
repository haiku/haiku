/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <arch/vm_translation_map.h>

#include <stdlib.h>
#include <string.h>

#include <AutoDeleter.h>

#include <arch_system_info.h>
#include <heap.h>
#include <int.h>
#include <thread.h>
#include <smp.h>
#include <util/AutoLock.h>
#include <util/queue.h>
#include <vm_address_space.h>
#include <vm_page.h>
#include <vm_priv.h>

#include "x86_paging.h"
#include "x86_physical_page_mapper.h"


//#define TRACE_VM_TMAP
#ifdef TRACE_VM_TMAP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

static page_table_entry *page_hole = NULL;
static page_directory_entry *page_hole_pgdir = NULL;
static page_directory_entry *sKernelPhysicalPageDirectory = NULL;
static page_directory_entry *sKernelVirtualPageDirectory = NULL;

static vm_translation_map *tmap_list;
static spinlock tmap_list_lock;

#define CHATTY_TMAP 0

#define FIRST_USER_PGDIR_ENT    (VADDR_TO_PDENT(USER_BASE))
#define NUM_USER_PGDIR_ENTS     (VADDR_TO_PDENT(ROUNDUP(USER_SIZE, B_PAGE_SIZE * 1024)))
#define FIRST_KERNEL_PGDIR_ENT  (VADDR_TO_PDENT(KERNEL_BASE))
#define NUM_KERNEL_PGDIR_ENTS   (VADDR_TO_PDENT(KERNEL_SIZE))
#define IS_KERNEL_MAP(map)		(map->arch_data->pgdir_phys == sKernelPhysicalPageDirectory)

static status_t early_query(addr_t va, addr_t *out_physical);

static void flush_tmap(vm_translation_map *map);


void *
i386_translation_map_get_pgdir(vm_translation_map *map)
{
	return map->arch_data->pgdir_phys;
}


void
x86_update_all_pgdirs(int index, page_directory_entry e)
{
	vm_translation_map *entry;
	unsigned int state = disable_interrupts();

	acquire_spinlock(&tmap_list_lock);

	for(entry = tmap_list; entry != NULL; entry = entry->next)
		entry->arch_data->pgdir_virt[index] = e;

	release_spinlock(&tmap_list_lock);
	restore_interrupts(state);
}


// XXX currently assumes this translation map is active

static status_t
early_query(addr_t va, addr_t *_physicalAddress)
{
	page_table_entry *pentry;

	if (page_hole_pgdir[VADDR_TO_PDENT(va)].present == 0) {
		// no pagetable here
		return B_ERROR;
	}

	pentry = page_hole + va / B_PAGE_SIZE;
	if (pentry->present == 0) {
		// page mapping not valid
		return B_ERROR;
	}

	*_physicalAddress = pentry->addr << 12;
	return B_OK;
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
	unsigned int i;

	if (map == NULL)
		return;

	// remove it from the tmap list
	state = disable_interrupts();
	acquire_spinlock(&tmap_list_lock);

	// TODO: How about using a doubly linked list?
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

	if (map->arch_data->page_mapper != NULL)
		map->arch_data->page_mapper->Delete();

	if (map->arch_data->pgdir_virt != NULL) {
		// cycle through and free all of the user space pgtables
		for (i = VADDR_TO_PDENT(USER_BASE); i <= VADDR_TO_PDENT(USER_BASE + (USER_SIZE - 1)); i++) {
			addr_t pgtable_addr;
			vm_page *page;

			if (map->arch_data->pgdir_virt[i].present == 1) {
				pgtable_addr = map->arch_data->pgdir_virt[i].addr;
				page = vm_lookup_page(pgtable_addr);
				if (!page)
					panic("destroy_tmap: didn't find pgtable page\n");
				vm_page_set_state(page, PAGE_STATE_FREE);
			}
		}
		free(map->arch_data->pgdir_virt);
	}

	free(map->arch_data);
	recursive_lock_destroy(&map->lock);
}


void
x86_put_pgtable_in_pgdir(page_directory_entry *entry,
	addr_t pgtable_phys, uint32 attributes)
{
	page_directory_entry table;
	// put it in the pgdir
	init_page_directory_entry(&table);
	table.addr = ADDR_SHIFT(pgtable_phys);

	// ToDo: we ignore the attributes of the page table - for compatibility
	//	with BeOS we allow having user accessible areas in the kernel address
	//	space. This is currently being used by some drivers, mainly for the
	//	frame buffer. Our current real time data implementation makes use of
	//	this fact, too.
	//	We might want to get rid of this possibility one day, especially if
	//	we intend to port it to a platform that does not support this.
	table.user = 1;
	table.rw = 1;
	table.present = 1;
	update_page_directory_entry(entry, &table);
}


static void
put_page_table_entry_in_pgtable(page_table_entry *entry,
	addr_t physicalAddress, uint32 attributes, bool globalPage)
{
	page_table_entry page;
	init_page_table_entry(&page);

	page.addr = ADDR_SHIFT(physicalAddress);

	// if the page is user accessible, it's automatically
	// accessible in kernel space, too (but with the same
	// protection)
	page.user = (attributes & B_USER_PROTECTION) != 0;
	if (page.user)
		page.rw = (attributes & B_WRITE_AREA) != 0;
	else
		page.rw = (attributes & B_KERNEL_WRITE_AREA) != 0;
	page.present = 1;

	if (globalPage)
		page.global = 1;

	// put it in the page table
	update_page_table_entry(entry, &page);
}


static size_t
map_max_pages_need(vm_translation_map */*map*/, addr_t start, addr_t end)
{
	// If start == 0, the actual base address is not yet known to the caller and
	// we shall assume the worst case.
	if (start == 0) {
		start = 1023 * B_PAGE_SIZE;
		end += 1023 * B_PAGE_SIZE;
	}
	return VADDR_TO_PDENT(end) + 1 - VADDR_TO_PDENT(start);
}


static status_t
map_tmap(vm_translation_map *map, addr_t va, addr_t pa, uint32 attributes)
{
	page_directory_entry *pd;
	page_table_entry *pt;
	unsigned int index;

	TRACE(("map_tmap: entry pa 0x%lx va 0x%lx\n", pa, va));

/*
	dprintf("pgdir at 0x%x\n", pgdir);
	dprintf("index is %d\n", va / B_PAGE_SIZE / 1024);
	dprintf("final at 0x%x\n", &pgdir[va / B_PAGE_SIZE / 1024]);
	dprintf("value is 0x%x\n", *(int *)&pgdir[va / B_PAGE_SIZE / 1024]);
	dprintf("present bit is %d\n", pgdir[va / B_PAGE_SIZE / 1024].present);
	dprintf("addr is %d\n", pgdir[va / B_PAGE_SIZE / 1024].addr);
*/
	pd = map->arch_data->pgdir_virt;

	// check to see if a page table exists for this range
	index = VADDR_TO_PDENT(va);
	if (pd[index].present == 0) {
		addr_t pgtable;
		vm_page *page;

		// we need to allocate a pgtable
		page = vm_page_allocate_page(PAGE_STATE_CLEAR, true);

		// mark the page WIRED
		vm_page_set_state(page, PAGE_STATE_WIRED);

		pgtable = page->physical_page_number * B_PAGE_SIZE;

		TRACE(("map_tmap: asked for free page for pgtable. 0x%lx\n", pgtable));

		// put it in the pgdir
		x86_put_pgtable_in_pgdir(&pd[index], pgtable, attributes
			| (attributes & B_USER_PROTECTION ? B_WRITE_AREA : B_KERNEL_WRITE_AREA));

		// update any other page directories, if it maps kernel space
		if (index >= FIRST_KERNEL_PGDIR_ENT
			&& index < (FIRST_KERNEL_PGDIR_ENT + NUM_KERNEL_PGDIR_ENTS))
			x86_update_all_pgdirs(index, pd[index]);

		map->map_count++;
	}

	// now, fill in the pentry
	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	pt = map->arch_data->page_mapper->GetPageTableAt(
		ADDR_REVERSE_SHIFT(pd[index].addr));
	index = VADDR_TO_PTENT(va);

	put_page_table_entry_in_pgtable(&pt[index], pa, attributes,
		IS_KERNEL_MAP(map));

	pinner.Unlock();

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
	page_directory_entry *pd = map->arch_data->pgdir_virt;
	int index;

	start = ROUNDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

	TRACE(("unmap_tmap: asked to free pages 0x%lx to 0x%lx\n", start, end));

restart:
	if (start >= end)
		return B_OK;

	index = VADDR_TO_PDENT(start);
	if (pd[index].present == 0) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE);
		goto restart;
	}

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	pt = map->arch_data->page_mapper->GetPageTableAt(
		ADDR_REVERSE_SHIFT(pd[index].addr));

	for (index = VADDR_TO_PTENT(start); (index < 1024) && (start < end);
			index++, start += B_PAGE_SIZE) {
		if (pt[index].present == 0) {
			// page mapping not valid
			continue;
		}

		TRACE(("unmap_tmap: removing page 0x%lx\n", start));

		pt[index].present = 0;
		map->map_count--;

		if (map->arch_data->num_invalidate_pages < PAGE_INVALIDATE_CACHE_SIZE)
			map->arch_data->pages_to_invalidate[map->arch_data->num_invalidate_pages] = start;

		map->arch_data->num_invalidate_pages++;
	}

	pinner.Unlock();

	goto restart;
}


static status_t
query_tmap_interrupt(vm_translation_map *map, addr_t va, addr_t *_physical,
	uint32 *_flags)
{
	page_directory_entry *pd = map->arch_data->pgdir_virt;
	page_table_entry *pt;
	addr_t physicalPageTable;
	int32 index;

	*_physical = 0;

	index = VADDR_TO_PDENT(va);
	if (pd[index].present == 0) {
		// no pagetable here
		return B_ERROR;
	}

	// map page table entry
	physicalPageTable = ADDR_REVERSE_SHIFT(pd[index].addr);
	pt = gPhysicalPageMapper->InterruptGetPageTableAt(physicalPageTable);

	index = VADDR_TO_PTENT(va);
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
	page_directory_entry *pd = map->arch_data->pgdir_virt;
	int32 index;

	// default the flags to not present
	*_flags = 0;
	*_physical = 0;

	index = VADDR_TO_PDENT(va);
	if (pd[index].present == 0) {
		// no pagetable here
		return B_NO_ERROR;
	}

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	pt = map->arch_data->page_mapper->GetPageTableAt(
		ADDR_REVERSE_SHIFT(pd[index].addr));
	index = VADDR_TO_PTENT(va);

	*_physical = ADDR_REVERSE_SHIFT(pt[index].addr);

	// read in the page state flags
	if (pt[index].user)
		*_flags |= (pt[index].rw ? B_WRITE_AREA : 0) | B_READ_AREA;

	*_flags |= ((pt[index].rw ? B_KERNEL_WRITE_AREA : 0) | B_KERNEL_READ_AREA)
		| (pt[index].dirty ? PAGE_MODIFIED : 0)
		| (pt[index].accessed ? PAGE_ACCESSED : 0)
		| (pt[index].present ? PAGE_PRESENT : 0);

	pinner.Unlock();

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
	page_directory_entry *pd = map->arch_data->pgdir_virt;
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

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	pt = map->arch_data->page_mapper->GetPageTableAt(
		ADDR_REVERSE_SHIFT(pd[index].addr));

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

	pinner.Unlock();

	goto restart;
}


static status_t
clear_flags_tmap(vm_translation_map *map, addr_t va, uint32 flags)
{
	page_table_entry *pt;
	page_directory_entry *pd = map->arch_data->pgdir_virt;
	int index;
	int tlb_flush = false;

	index = VADDR_TO_PDENT(va);
	if (pd[index].present == 0) {
		// no pagetable here
		return B_OK;
	}

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	pt = map->arch_data->page_mapper->GetPageTableAt(
		ADDR_REVERSE_SHIFT(pd[index].addr));
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

	pinner.Unlock();

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
	if (map->arch_data->num_invalidate_pages <= 0)
		return;

	struct thread* thread = thread_get_current_thread();
	thread_pin_to_current_cpu(thread);

	if (map->arch_data->num_invalidate_pages > PAGE_INVALIDATE_CACHE_SIZE) {
		// invalidate all pages
		TRACE(("flush_tmap: %d pages to invalidate, invalidate all\n",
			map->arch_data->num_invalidate_pages));

		if (IS_KERNEL_MAP(map)) {
			arch_cpu_global_TLB_invalidate();
			smp_send_broadcast_ici(SMP_MSG_GLOBAL_INVALIDATE_PAGES, 0, 0, 0,
				NULL, SMP_MSG_FLAG_SYNC);
		} else {
			arch_cpu_user_TLB_invalidate();

			int cpu = smp_get_current_cpu();
			uint32 cpuMask = map->arch_data->active_on_cpus
				& ~((uint32)1 << cpu);
			if (cpuMask != 0) {
				smp_send_multicast_ici(cpuMask, SMP_MSG_USER_INVALIDATE_PAGES,
					0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
			}
		}
	} else {
		TRACE(("flush_tmap: %d pages to invalidate, invalidate list\n",
			map->arch_data->num_invalidate_pages));

		arch_cpu_invalidate_TLB_list(map->arch_data->pages_to_invalidate,
			map->arch_data->num_invalidate_pages);

		if (IS_KERNEL_MAP(map)) {
			smp_send_broadcast_ici(SMP_MSG_INVALIDATE_PAGE_LIST,
				(uint32)map->arch_data->pages_to_invalidate,
				map->arch_data->num_invalidate_pages, 0, NULL,
				SMP_MSG_FLAG_SYNC);
		} else {
			int cpu = smp_get_current_cpu();
			uint32 cpuMask = map->arch_data->active_on_cpus
				& ~((uint32)1 << cpu);
			if (cpuMask != 0) {
				smp_send_multicast_ici(cpuMask, SMP_MSG_INVALIDATE_PAGE_LIST,
					(uint32)map->arch_data->pages_to_invalidate,
					map->arch_data->num_invalidate_pages, 0, NULL,
					SMP_MSG_FLAG_SYNC);
			}
		}
	}
	map->arch_data->num_invalidate_pages = 0;

	thread_unpin_from_current_cpu(thread);
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
	flush_tmap

	// The physical page ops are initialized by the respective physical page
	// mapper.
};


//	#pragma mark -


void
x86_early_prepare_page_tables(page_table_entry* pageTables, addr_t address,
	size_t size)
{
	memset(pageTables, 0, B_PAGE_SIZE * (size / (B_PAGE_SIZE * 1024)));

	// put the array of pgtables directly into the kernel pagedir
	// these will be wired and kept mapped into virtual space to be easy to get
	// to
	{
		addr_t virtualTable = (addr_t)pageTables;

		for (size_t i = 0; i < (size / (B_PAGE_SIZE * 1024));
				i++, virtualTable += B_PAGE_SIZE) {
			addr_t physicalTable;
			early_query(virtualTable, &physicalTable);
			page_directory_entry* entry = &page_hole_pgdir[
				(address / (B_PAGE_SIZE * 1024)) + i];
			x86_put_pgtable_in_pgdir(entry, physicalTable,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		}
	}
}


//	#pragma mark -
//	VM API


status_t
arch_vm_translation_map_init_map(vm_translation_map *map, bool kernel)
{
	if (map == NULL)
		return B_BAD_VALUE;

	TRACE(("vm_translation_map_create\n"));

	// initialize the new object
	map->ops = &tmap_ops;
	map->map_count = 0;

	recursive_lock_init(&map->lock, "translation map");
	CObjectDeleter<recursive_lock> lockDeleter(&map->lock,
		&recursive_lock_destroy);

	map->arch_data = (vm_translation_map_arch_info *)malloc(sizeof(vm_translation_map_arch_info));
	if (map->arch_data == NULL)
		return B_NO_MEMORY;
	MemoryDeleter archInfoDeleter(map->arch_data);

	map->arch_data->active_on_cpus = 0;
	map->arch_data->num_invalidate_pages = 0;
	map->arch_data->page_mapper = NULL;

	if (!kernel) {
		// user
		// allocate a physical page mapper
		status_t error = gPhysicalPageMapper
			->CreateTranslationMapPhysicalPageMapper(
				&map->arch_data->page_mapper);
		if (error != B_OK)
			return error;

		// allocate a pgdir
		map->arch_data->pgdir_virt = (page_directory_entry *)memalign(
			B_PAGE_SIZE, B_PAGE_SIZE);
		if (map->arch_data->pgdir_virt == NULL) {
			map->arch_data->page_mapper->Delete();
			return B_NO_MEMORY;
		}
		vm_get_page_mapping(vm_kernel_address_space_id(),
			(addr_t)map->arch_data->pgdir_virt, (addr_t *)&map->arch_data->pgdir_phys);
	} else {
		// kernel
		// get the physical page mapper
		map->arch_data->page_mapper = gKernelPhysicalPageMapper;

		// we already know the kernel pgdir mapping
		map->arch_data->pgdir_virt = sKernelVirtualPageDirectory;
		map->arch_data->pgdir_phys = sKernelPhysicalPageDirectory;
	}

	// zero out the bottom portion of the new pgdir
	memset(map->arch_data->pgdir_virt + FIRST_USER_PGDIR_ENT, 0,
		NUM_USER_PGDIR_ENTS * sizeof(page_directory_entry));

	// insert this new map into the map list
	{
		int state = disable_interrupts();
		acquire_spinlock(&tmap_list_lock);

		// copy the top portion of the pgdir from the current one
		memcpy(map->arch_data->pgdir_virt + FIRST_KERNEL_PGDIR_ENT,
			sKernelVirtualPageDirectory + FIRST_KERNEL_PGDIR_ENT,
			NUM_KERNEL_PGDIR_ENTS * sizeof(page_directory_entry));

		map->next = tmap_list;
		tmap_list = map;

		release_spinlock(&tmap_list_lock);
		restore_interrupts(state);
	}

	archInfoDeleter.Detach();
	lockDeleter.Detach();

	return B_OK;
}


status_t
arch_vm_translation_map_init_kernel_map_post_sem(vm_translation_map *map)
{
	return B_OK;
}


status_t
arch_vm_translation_map_init(kernel_args *args)
{
	TRACE(("vm_translation_map_init: entry\n"));

	// page hole set up in stage2
	page_hole = (page_table_entry *)args->arch_args.page_hole;
	// calculate where the pgdir would be
	page_hole_pgdir = (page_directory_entry *)(((unsigned int)args->arch_args.page_hole) + (B_PAGE_SIZE * 1024 - B_PAGE_SIZE));
	// clear out the bottom 2 GB, unmap everything
	memset(page_hole_pgdir + FIRST_USER_PGDIR_ENT, 0, sizeof(page_directory_entry) * NUM_USER_PGDIR_ENTS);

	sKernelPhysicalPageDirectory = (page_directory_entry *)args->arch_args.phys_pgdir;
	sKernelVirtualPageDirectory = (page_directory_entry *)args->arch_args.vir_pgdir;

	B_INITIALIZE_SPINLOCK(&tmap_list_lock);
	tmap_list = NULL;

// TODO: Select the best page mapper!
	large_memory_physical_page_ops_init(args, &tmap_ops);

	// enable global page feature if available
	if (x86_check_feature(IA32_FEATURE_PGE, FEATURE_COMMON)) {
		// this prevents kernel pages from being flushed from TLB on context-switch
		x86_write_cr4(x86_read_cr4() | IA32_CR4_GLOBAL_PAGES);
	}

	TRACE(("vm_translation_map_init: done\n"));

	return B_OK;
}


status_t
arch_vm_translation_map_init_post_sem(kernel_args *args)
{
	return B_OK;
}


status_t
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

	error = gPhysicalPageMapper->InitPostArea(args);
	if (error != B_OK)
		return error;

	TRACE(("vm_translation_map_init_post_area: done\n"));
	return B_OK;
}


// XXX horrible back door to map a page quickly regardless of translation map object, etc.
// used only during VM setup.
// uses a 'page hole' set up in the stage 2 bootloader. The page hole is created by pointing one of
// the pgdir entries back at itself, effectively mapping the contents of all of the 4MB of pagetables
// into a 4 MB region. It's only used here, and is later unmapped.

status_t
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
		x86_put_pgtable_in_pgdir(e, pgtable, attributes);

		// zero it out in it's new mapping
		memset((unsigned int *)((unsigned int)page_hole + (va / B_PAGE_SIZE / 1024) * B_PAGE_SIZE), 0, B_PAGE_SIZE);
	}

	// now, fill in the pentry
	put_page_table_entry_in_pgtable(page_hole + va / B_PAGE_SIZE, pa, attributes,
		IS_KERNEL_ADDRESS(va));

	arch_cpu_invalidate_TLB_range(va, va);

	return B_OK;
}

