/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <vm.h>
#include <vm_page.h>
#include <vm_priv.h>
#include <smp.h>
#include <util/queue.h>
#include <memheap.h>
#include <arch_system_info.h>
#include <arch/vm_translation_map.h>

#include <string.h>
#include <stdlib.h>

//#define TRACE_VM_TMAP
#ifdef TRACE_VM_TMAP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// 256 MB of iospace
#define IOSPACE_SIZE (256*1024*1024)
// put it 256 MB into kernel space
#define IOSPACE_BASE (KERNEL_BASE + IOSPACE_SIZE)
// 4 MB chunks, to optimize for 4 MB pages
#define IOSPACE_CHUNK_SIZE (4*1024*1024)

typedef struct page_table_entry {
	uint32	present:1;
	uint32	rw:1;
	uint32	user:1;
	uint32	write_through:1;
	uint32	cache_disabled:1;
	uint32	accessed:1;
	uint32	dirty:1;
	uint32	reserved:1;
	uint32	global:1;
	uint32	avail:3;
	uint32	addr:20;
} page_table_entry;

typedef struct page_directory_entry {
	uint32	present:1;
	uint32	rw:1;
	uint32	user:1;
	uint32	write_through:1;
	uint32	cache_disabled:1;
	uint32	accessed:1;
	uint32	reserved:1;
	uint32	page_size:1;
	uint32	global:1;
	uint32	avail:3;
	uint32	addr:20;
} page_directory_entry;

// data and structures used to represent physical pages mapped into iospace
typedef struct paddr_chunk_descriptor {
	struct paddr_chunk_descriptor *next_q; // must remain first in structure, queue code uses it
	int ref_count;
	addr_t va;
} paddr_chunk_desc;

static paddr_chunk_desc *paddr_desc;         // will be one per physical chunk
static paddr_chunk_desc **virtual_pmappings; // will be one ptr per virtual chunk in iospace
static int first_free_vmapping;
static int num_virtual_chunks;
static queue mapped_paddr_lru;
static page_table_entry *iospace_pgtables = NULL;
static mutex iospace_mutex;
static sem_id iospace_full_sem;

#define PAGE_INVALIDATE_CACHE_SIZE 64

// vm_translation object stuff
typedef struct vm_translation_map_arch_info {
	page_directory_entry *pgdir_virt;
	page_directory_entry *pgdir_phys;
	int num_invalidate_pages;
	addr_t pages_to_invalidate[PAGE_INVALIDATE_CACHE_SIZE];
} vm_translation_map_arch_info;


static page_table_entry *page_hole = NULL;
static page_directory_entry *page_hole_pgdir = NULL;
static page_directory_entry *sKernelPhysicalPageDirectory = NULL;
static page_directory_entry *sKernelVirtualPageDirectory = NULL;

static vm_translation_map *tmap_list;
static spinlock tmap_list_lock;

#define CHATTY_TMAP 0

#define ADDR_SHIFT(x) ((x)>>12)
#define ADDR_REVERSE_SHIFT(x) ((x)<<12)

#define VADDR_TO_PDENT(va) (((va) / B_PAGE_SIZE) / 1024)
#define VADDR_TO_PTENT(va) (((va) / B_PAGE_SIZE) % 1024)

#define FIRST_USER_PGDIR_ENT    (VADDR_TO_PDENT(USER_BASE))
#define NUM_USER_PGDIR_ENTS     (VADDR_TO_PDENT(ROUNDUP(USER_SIZE, B_PAGE_SIZE * 1024)))
#define FIRST_KERNEL_PGDIR_ENT  (VADDR_TO_PDENT(KERNEL_BASE))
#define NUM_KERNEL_PGDIR_ENTS   (VADDR_TO_PDENT(KERNEL_SIZE))
#define IS_KERNEL_MAP(map)		(map->arch_data->pgdir_phys == sKernelPhysicalPageDirectory)

static status_t early_query(addr_t va, addr_t *out_physical);
static status_t get_physical_page_tmap(addr_t pa, addr_t *va, uint32 flags);
static status_t put_physical_page_tmap(addr_t va);

static void flush_tmap(vm_translation_map *map);


void *
i386_translation_map_get_pgdir(vm_translation_map *map)
{
	return map->arch_data->pgdir_phys;
}


static inline void
init_page_directory_entry(page_directory_entry *entry)
{
	*(uint32 *)entry = 0;
}


static inline void
update_page_directory_entry(page_directory_entry *entry, page_directory_entry *with)
{
	// update page directory entry atomically
	*(uint32 *)entry = *(uint32 *)with;
}


static inline void
init_page_table_entry(page_table_entry *entry)
{
	*(uint32 *)entry = 0;
}


static inline void
update_page_table_entry(page_table_entry *entry, page_table_entry *with)
{
	// update page table entry atomically
	*(uint32 *)entry = *(uint32 *)with;
}


static void
_update_all_pgdirs(int index, page_directory_entry e)
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


static status_t
lock_tmap(vm_translation_map *map)
{
	TRACE(("lock_tmap: map %p\n", map));

	if (recursive_lock_lock(&map->lock) == true) {
		// we were the first one to grab the lock
		TRACE(("clearing invalidated page count\n"));
		map->arch_data->num_invalidate_pages = 0;
	}

	return 0;
}


static status_t
unlock_tmap(vm_translation_map *map)
{
	TRACE(("unlock_tmap: map %p\n", map));

	if (recursive_lock_get_recursion(&map->lock) == 1) {
		// we're about to release it for the last time
		flush_tmap(map);
	}

	recursive_lock_unlock(&map->lock);
	return 0;
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


static void
put_pgtable_in_pgdir(page_directory_entry *entry,
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


static status_t
map_tmap(vm_translation_map *map, addr_t va, addr_t pa, uint32 attributes)
{
	page_directory_entry *pd;
	page_table_entry *pt;
	unsigned int index;
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
	pd = map->arch_data->pgdir_virt;

	// check to see if a page table exists for this range
	index = VADDR_TO_PDENT(va);
	if (pd[index].present == 0) {
		addr_t pgtable;
		vm_page *page;

		// we need to allocate a pgtable
		page = vm_page_allocate_page(PAGE_STATE_CLEAR);

		// mark the page WIRED 
		vm_page_set_state(page, PAGE_STATE_WIRED);

		pgtable = page->ppn * B_PAGE_SIZE;

		TRACE(("map_tmap: asked for free page for pgtable. 0x%lx\n", pgtable));

		// put it in the pgdir
		put_pgtable_in_pgdir(&pd[index], pgtable, attributes
			| (attributes & B_USER_PROTECTION ? B_WRITE_AREA : B_KERNEL_WRITE_AREA));

		// update any other page directories, if it maps kernel space
		if (index >= FIRST_KERNEL_PGDIR_ENT
			&& index < (FIRST_KERNEL_PGDIR_ENT + NUM_KERNEL_PGDIR_ENTS))
			_update_all_pgdirs(index, pd[index]);

		map->map_count++;
	}

	// now, fill in the pentry
	do {
		err = get_physical_page_tmap(ADDR_REVERSE_SHIFT(pd[index].addr),
				(addr_t *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while (err < 0);
	index = VADDR_TO_PTENT(va);

	put_page_table_entry_in_pgtable(&pt[index], pa, attributes,
		IS_KERNEL_MAP(map));

	put_physical_page_tmap((addr_t)pt);

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
	int err;

	start = ROUNDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

	TRACE(("unmap_tmap: asked to free pages 0x%lx to 0x%lx\n", start, end));

restart:
	if (start >= end)
		return 0;

	index = VADDR_TO_PDENT(start);
	if (pd[index].present == 0) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE);
		goto restart;
	}

	do {
		err = get_physical_page_tmap(ADDR_REVERSE_SHIFT(pd[index].addr), (addr_t *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while (err < 0);

	for (index = VADDR_TO_PTENT(start); (index < 1024) && (start < end); index++, start += B_PAGE_SIZE) {
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

	put_physical_page_tmap((addr_t)pt);

	goto restart;
}


static status_t
query_tmap(vm_translation_map *map, addr_t va, addr_t *out_physical, uint32 *out_flags)
{
	page_table_entry *pt;
	page_directory_entry *pd = map->arch_data->pgdir_virt;
	int index;
	int err;

	// default the flags to not present
	*out_flags = 0;
	*out_physical = 0;

	index = VADDR_TO_PDENT(va);
	if (pd[index].present == 0) {
		// no pagetable here
		return B_NO_ERROR;
	}

	do {
		err = get_physical_page_tmap(ADDR_REVERSE_SHIFT(pd[index].addr), (addr_t *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while (err < 0);
	index = VADDR_TO_PTENT(va);

	*out_physical = ADDR_REVERSE_SHIFT(pt[index].addr);

	// read in the page state flags, clearing the modified and accessed flags in the process
	*out_flags = 0;
	if (pt[index].user)
		*out_flags |= (pt[index].rw ? B_WRITE_AREA : 0) | B_READ_AREA;

	*out_flags |= (pt[index].rw ? B_KERNEL_WRITE_AREA : 0) | B_KERNEL_READ_AREA;
	*out_flags |= pt[index].dirty ? PAGE_MODIFIED : 0;
	*out_flags |= pt[index].accessed ? PAGE_ACCESSED : 0;
	*out_flags |= pt[index].present ? PAGE_PRESENT : 0;

	put_physical_page_tmap((addr_t)pt);

	TRACE(("query_tmap: returning pa 0x%lx for va 0x%lx\n", *out_physical, va));

	return 0;
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
	int err;

	start = ROUNDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

	TRACE(("protect_tmap: pages 0x%lx to 0x%lx, attributes %lx\n", start, end, attributes));

restart:
	if (start >= end)
		return 0;

	index = VADDR_TO_PDENT(start);
	if (pd[index].present == 0) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE);
		goto restart;
	}

	do {
		err = get_physical_page_tmap(ADDR_REVERSE_SHIFT(pd[index].addr),
				(addr_t *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while (err < 0);

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
	page_directory_entry *pd = map->arch_data->pgdir_virt;
	int index;
	int err;
	int tlb_flush = false;

	index = VADDR_TO_PDENT(va);
	if (pd[index].present == 0) {
		// no pagetable here
		return B_NO_ERROR;
	}

	do {
		err = get_physical_page_tmap(ADDR_REVERSE_SHIFT(pd[index].addr), (addr_t *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while (err < 0);
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
	if (va < IOSPACE_BASE || va >= (IOSPACE_BASE + IOSPACE_SIZE))
		panic("map_iospace_chunk: passed invalid va 0x%lx\n", va);

	ppn = ADDR_SHIFT(pa);
	pt = &iospace_pgtables[(va - IOSPACE_BASE) / B_PAGE_SIZE];
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
	int index;
	paddr_chunk_desc *replaced_pchunk;

restart:
	mutex_lock(&iospace_mutex);

	// see if the page is already mapped
	index = pa / IOSPACE_CHUNK_SIZE;
	if (paddr_desc[index].va != 0) {
		if (paddr_desc[index].ref_count++ == 0) {
			// pull this descriptor out of the lru list
			queue_remove_item(&mapped_paddr_lru, &paddr_desc[index]);
		}
		*va = paddr_desc[index].va + pa % IOSPACE_CHUNK_SIZE;
		mutex_unlock(&iospace_mutex);
		return B_OK;
	}

	// map it
	if (first_free_vmapping < num_virtual_chunks) {
		// there's a free hole
		paddr_desc[index].va = first_free_vmapping * IOSPACE_CHUNK_SIZE + IOSPACE_BASE;
		*va = paddr_desc[index].va + pa % IOSPACE_CHUNK_SIZE;
		virtual_pmappings[first_free_vmapping] = &paddr_desc[index];
		paddr_desc[index].ref_count++;

		// push up the first_free_vmapping pointer
		for (; first_free_vmapping < num_virtual_chunks; first_free_vmapping++) {
			if(virtual_pmappings[first_free_vmapping] == NULL)
				break;
		}

		map_iospace_chunk(paddr_desc[index].va, index * IOSPACE_CHUNK_SIZE);
		mutex_unlock(&iospace_mutex);

		return B_OK;
	}

	// replace an earlier mapping
	if (queue_peek(&mapped_paddr_lru) == NULL) {
		// no free slots available
		if (flags == PHYSICAL_PAGE_NO_WAIT) {
			// punt back to the caller and let them handle this
			mutex_unlock(&iospace_mutex);
			return B_NO_MEMORY;
		} else {
			mutex_unlock(&iospace_mutex);
			acquire_sem(iospace_full_sem);
			goto restart;
		}
	}

	replaced_pchunk = queue_dequeue(&mapped_paddr_lru);
	paddr_desc[index].va = replaced_pchunk->va;
	replaced_pchunk->va = 0;
	*va = paddr_desc[index].va + pa % IOSPACE_CHUNK_SIZE;
	paddr_desc[index].ref_count++;

	map_iospace_chunk(paddr_desc[index].va, index * IOSPACE_CHUNK_SIZE);

	mutex_unlock(&iospace_mutex);
	return B_OK;
}


static status_t
put_physical_page_tmap(addr_t va)
{
	paddr_chunk_desc *desc;

	if (va < IOSPACE_BASE || va >= IOSPACE_BASE + IOSPACE_SIZE)
		panic("someone called put_physical_page on an invalid va 0x%lx\n", va);
	va -= IOSPACE_BASE;

	mutex_lock(&iospace_mutex);

	desc = virtual_pmappings[va / IOSPACE_CHUNK_SIZE];
	if (desc == NULL) {
		mutex_unlock(&iospace_mutex);
		panic("put_physical_page called on page at va 0x%lx which is not checked out\n", va);
		return B_ERROR;
	}

	if (--desc->ref_count == 0) {
		// put it on the mapped lru list
		queue_enqueue(&mapped_paddr_lru, desc);
		// no sense rescheduling on this one, there's likely a race in the waiting
		// thread to grab the iospace_mutex, which would block and eventually get back to
		// this thread. waste of time.
		release_sem_etc(iospace_full_sem, 1, B_DO_NOT_RESCHEDULE);
	}

	mutex_unlock(&iospace_mutex);

	return B_OK;
}


static vm_translation_map_ops tmap_ops = {
	destroy_tmap,
	lock_tmap,
	unlock_tmap,
	map_tmap,
	unmap_tmap,
	query_tmap,
	get_mapped_size_tmap,
	protect_tmap,
	clear_flags_tmap,
	flush_tmap,
	get_physical_page_tmap,
	put_physical_page_tmap
};


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
		map->arch_data->pgdir_virt = memalign(B_PAGE_SIZE, B_PAGE_SIZE);
		if (map->arch_data->pgdir_virt == NULL) {
			free(map->arch_data);
			recursive_lock_destroy(&map->lock);
			return B_NO_MEMORY;
		}
		vm_get_page_mapping(vm_get_kernel_aspace_id(),
			(addr_t)map->arch_data->pgdir_virt, (addr_t *)&map->arch_data->pgdir_phys);
	} else {
		// kernel
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

	return B_OK;
}


status_t
arch_vm_translation_map_init_kernel_map_post_sem(vm_translation_map *map)
{
	if (recursive_lock_init(&map->lock, "translation map") < B_OK)
		return map->lock.sem;

	return B_OK;
}


status_t
arch_vm_translation_map_init(kernel_args *args)
{
	cpuid_info info;

	TRACE(("vm_translation_map_init: entry\n"));

	// page hole set up in stage2
	page_hole = (page_table_entry *)args->arch_args.page_hole;
	// calculate where the pgdir would be
	page_hole_pgdir = (page_directory_entry *)(((unsigned int)args->arch_args.page_hole) + (B_PAGE_SIZE * 1024 - B_PAGE_SIZE));
	// clear out the bottom 2 GB, unmap everything
	memset(page_hole_pgdir + FIRST_USER_PGDIR_ENT, 0, sizeof(page_directory_entry) * NUM_USER_PGDIR_ENTS);

	sKernelPhysicalPageDirectory = (page_directory_entry *)args->arch_args.phys_pgdir;
	sKernelVirtualPageDirectory = (page_directory_entry *)args->arch_args.vir_pgdir;

	tmap_list_lock = 0;
	tmap_list = NULL;

	// allocate some space to hold physical page mapping info
	paddr_desc = (paddr_chunk_desc *)vm_alloc_from_kernel_args(args,
		sizeof(paddr_chunk_desc) * 1024, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	num_virtual_chunks = IOSPACE_SIZE / IOSPACE_CHUNK_SIZE;
	virtual_pmappings = (paddr_chunk_desc **)vm_alloc_from_kernel_args(args,
		sizeof(paddr_chunk_desc *) * num_virtual_chunks, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	iospace_pgtables = (page_table_entry *)vm_alloc_from_kernel_args(args,
		B_PAGE_SIZE * (IOSPACE_SIZE / (B_PAGE_SIZE * 1024)), B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	TRACE(("paddr_desc %p, virtual_pmappings %p, iospace_pgtables %p\n",
		paddr_desc, virtual_pmappings, iospace_pgtables));

	// initialize our data structures
	memset(paddr_desc, 0, sizeof(paddr_chunk_desc) * 1024);
	memset(virtual_pmappings, 0, sizeof(paddr_chunk_desc *) * num_virtual_chunks);
	first_free_vmapping = 0;
	queue_init(&mapped_paddr_lru);
	memset(iospace_pgtables, 0, B_PAGE_SIZE * (IOSPACE_SIZE / (B_PAGE_SIZE * 1024)));
	iospace_mutex.sem = -1;
	iospace_mutex.holder = -1;
	iospace_full_sem = -1;

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
			e = &page_hole_pgdir[(IOSPACE_BASE / (B_PAGE_SIZE * 1024)) + i];
			put_pgtable_in_pgdir(e, phys_pgtable, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		}
	}

	// enable global page feature if available
	get_current_cpuid(&info, 1);
	if (info.eax_1.features & IA32_FEATURE_GLOBAL_PAGES) {
		// this prevents kernel pages from being flushed from TLB on context-switch
		x86_write_cr4(x86_read_cr4() | IA32_CR4_GLOBAL_PAGES);
	}

	TRACE(("vm_translation_map_init: done\n"));

	return B_OK;
}


status_t
arch_vm_translation_map_init_post_sem(kernel_args *args)
{
	mutex_init(&iospace_mutex, "iospace_mutex");
	iospace_full_sem = create_sem(1, "iospace_full_sem");

	return iospace_full_sem >= B_OK ? B_OK : iospace_full_sem;
}


status_t
arch_vm_translation_map_init_post_area(kernel_args *args)
{
	// now that the vm is initialized, create a region that represents
	// the page hole
	void *temp;

	TRACE(("vm_translation_map_init_post_area: entry\n"));

	// unmap the page hole hack we were using before
	sKernelVirtualPageDirectory[1023].present = 0;
	page_hole_pgdir = NULL;
	page_hole = NULL;

	temp = (void *)sKernelVirtualPageDirectory;
	create_area("kernel_pgdir", &temp, B_EXACT_ADDRESS, B_PAGE_SIZE,
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	temp = (void *)paddr_desc;
	create_area("physical_page_mapping_descriptors", &temp, B_EXACT_ADDRESS,
		ROUNDUP(sizeof(paddr_chunk_desc) * 1024, B_PAGE_SIZE),
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	temp = (void *)virtual_pmappings;
	create_area("iospace_virtual_chunk_descriptors", &temp, B_EXACT_ADDRESS,
		ROUNDUP(sizeof(paddr_chunk_desc *) * num_virtual_chunks, B_PAGE_SIZE),
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	temp = (void *)iospace_pgtables;
	create_area("iospace_pgtables", &temp, B_EXACT_ADDRESS,
		B_PAGE_SIZE * (IOSPACE_SIZE / (B_PAGE_SIZE * 1024)),
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	TRACE(("vm_translation_map_init_post_area: creating iospace\n"));
	temp = (void *)IOSPACE_BASE;
	vm_create_null_area(vm_get_kernel_aspace_id(), "iospace", &temp,
		B_EXACT_ADDRESS, IOSPACE_SIZE);

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

