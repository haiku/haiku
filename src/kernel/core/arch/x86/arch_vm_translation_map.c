/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <memheap.h>
#include <int.h>
#include <smp.h>
#include <vm.h>
#include <vm_page.h>
#include <vm_priv.h>
#include <debug.h>
#include <lock.h>
#include <OS.h>
#include <queue.h>
#include <string.h>
#include <stage2.h>
#include <Errors.h>

#include <arch/cpu.h>
#include <arch/vm_translation_map.h>


// 256 MB of iospace
#define IOSPACE_SIZE (256*1024*1024)
// put it 256 MB into kernel space
#define IOSPACE_BASE (KERNEL_BASE + IOSPACE_SIZE)
// 4 MB chunks, to optimize for 4 MB pages
#define IOSPACE_CHUNK_SIZE (4*1024*1024)

// data and structures used to represent physical pages mapped into iospace
typedef struct paddr_chunk_descriptor {
	struct paddr_chunk_descriptor *next_q; // must remain first in structure, queue code uses it
	int ref_count;
	addr va;
} paddr_chunk_desc;

static paddr_chunk_desc *paddr_desc;         // will be one per physical chunk
static paddr_chunk_desc **virtual_pmappings; // will be one ptr per virtual chunk in iospace
static int first_free_vmapping;
static int num_virtual_chunks;
static queue mapped_paddr_lru;
static ptentry *iospace_pgtables = NULL;
static mutex iospace_mutex;
static sem_id iospace_full_sem;

#define PAGE_INVALIDATE_CACHE_SIZE 64

// vm_translation object stuff
typedef struct vm_translation_map_arch_info_struct {
	pdentry *pgdir_virt;
	pdentry *pgdir_phys;
	int num_invalidate_pages;
	addr pages_to_invalidate[PAGE_INVALIDATE_CACHE_SIZE];
} vm_translation_map_arch_info;

static ptentry *page_hole = NULL;
static pdentry *page_hole_pgdir = NULL;
static pdentry *kernel_pgdir_phys = NULL;
static pdentry *kernel_pgdir_virt = NULL;

static vm_translation_map *tmap_list;
static spinlock_t tmap_list_lock;

#define CHATTY_TMAP 0

#define ADDR_SHIFT(x) ((x)>>12)
#define ADDR_REVERSE_SHIFT(x) ((x)<<12)

#define VADDR_TO_PDENT(va) (((va) / PAGE_SIZE) / 1024)
#define VADDR_TO_PTENT(va) (((va) / PAGE_SIZE) % 1024)

#define FIRST_USER_PGDIR_ENT    (VADDR_TO_PDENT(USER_BASE))
#define NUM_USER_PGDIR_ENTS     (VADDR_TO_PDENT(USER_SIZE))
#define FIRST_KERNEL_PGDIR_ENT  (VADDR_TO_PDENT(KERNEL_BASE))
#define NUM_KERNEL_PGDIR_ENTS   (VADDR_TO_PDENT(KERNEL_SIZE))

static int vm_translation_map_quick_query(addr va, addr *out_physical);

static void flush_tmap(vm_translation_map *map);

static void init_pdentry(pdentry *e)
{
	*(int *)e = 0;
}

static void init_ptentry(ptentry *e)
{
	*(int *)e = 0;
}

static void _update_all_pgdirs(int index, pdentry e)
{
	unsigned int state;
	vm_translation_map *entry;

	state = int_disable_interrupts();
	acquire_spinlock(&tmap_list_lock);

	for(entry = tmap_list; entry != NULL; entry = entry->next)
		entry->arch_data->pgdir_virt[index] = e;

	release_spinlock(&tmap_list_lock);
	int_restore_interrupts(state);
}

static int lock_tmap(vm_translation_map *map)
{
//	dprintf("lock_tmap: map 0x%x\n", map);
	if(recursive_lock_lock(&map->lock) == true) {
		// we were the first one to grab the lock
//		dprintf("clearing invalidated page count\n");
		map->arch_data->num_invalidate_pages = 0;
	}

	return 0;
}

static int unlock_tmap(vm_translation_map *map)
{
//	dprintf("unlock_tmap: map 0x%x\n", map);

	if(recursive_lock_get_recursion(&map->lock) == 1) {
		// we're about to release it for the last time
		flush_tmap(map);
	}
	recursive_lock_unlock(&map->lock);
	return 0;
}

static void destroy_tmap(vm_translation_map *map)
{
	int state;
	vm_translation_map *entry;
	vm_translation_map *last = NULL;
	unsigned int i;

	if(map == NULL)
		return;

	// remove it from the tmap list
	state = int_disable_interrupts();
	acquire_spinlock(&tmap_list_lock);

	entry = tmap_list;
	while(entry != NULL) {
		if(entry == map) {
			if(last != NULL) {
				last->next = entry->next;
			} else {
				tmap_list = entry->next;
			}
			break;
		}
		last = entry;
		entry = entry->next;
	}

	release_spinlock(&tmap_list_lock);
	int_restore_interrupts(state);


	if(map->arch_data->pgdir_virt != NULL) {
		// cycle through and free all of the user space pgtables
		for(i = VADDR_TO_PDENT(USER_BASE); i < VADDR_TO_PDENT(USER_BASE + (USER_SIZE - 1)); i++) {
			addr pgtable_addr;
			vm_page *page;

			if(map->arch_data->pgdir_virt[i].present == 1) {
				pgtable_addr = map->arch_data->pgdir_virt[i].addr;
				page = vm_lookup_page(pgtable_addr);
				if(!page)
					panic("destroy_tmap: didn't find pgtable page\n");
				vm_page_set_state(page, PAGE_STATE_FREE);
			}
		}
		kfree(map->arch_data->pgdir_virt);
	}

	kfree(map->arch_data);
	recursive_lock_destroy(&map->lock);
}

static void put_pgtable_in_pgdir(pdentry *e, addr pgtable_phys, int attributes)
{
	// put it in the pgdir
	init_pdentry(e);
	e->addr = ADDR_SHIFT(pgtable_phys);
	e->user = !(attributes & LOCK_KERNEL);
	e->rw = attributes & LOCK_RW;
	e->present = 1;
}

static int map_tmap(vm_translation_map *map, addr va, addr pa, unsigned int attributes)
{
	pdentry *pd;
	ptentry *pt;
	unsigned int index;
	int err;

#if CHATTY_TMAP
	dprintf("map_tmap: entry pa 0x%x va 0x%x\n", pa, va);
#endif
/*
	dprintf("pgdir at 0x%x\n", pgdir);
	dprintf("index is %d\n", va / PAGE_SIZE / 1024);
	dprintf("final at 0x%x\n", &pgdir[va / PAGE_SIZE / 1024]);
	dprintf("value is 0x%x\n", *(int *)&pgdir[va / PAGE_SIZE / 1024]);
	dprintf("present bit is %d\n", pgdir[va / PAGE_SIZE / 1024].present);
	dprintf("addr is %d\n", pgdir[va / PAGE_SIZE / 1024].addr);
*/
	pd = map->arch_data->pgdir_virt;

	// check to see if a page table exists for this range
	index = VADDR_TO_PDENT(va);
	if(pd[index].present == 0) {
		addr pgtable;
		vm_page *page;

		// we need to allocate a pgtable
		page = vm_page_allocate_page(PAGE_STATE_CLEAR);
		pgtable = page->ppn * PAGE_SIZE;
#if CHATTY_TMAP
		dprintf("map_tmap: asked for free page for pgtable. 0x%x\n", pgtable);
#endif
		// put it in the pgdir
		put_pgtable_in_pgdir(&pd[index], pgtable, attributes | LOCK_RW);

		// update any other page directories, if it maps kernel space
		if(index >= FIRST_KERNEL_PGDIR_ENT && index < (FIRST_KERNEL_PGDIR_ENT + NUM_KERNEL_PGDIR_ENTS))
			_update_all_pgdirs(index, pd[index]);

		map->map_count++;
	}

	// now, fill in the pentry
	do {
		err = vm_get_physical_page(ADDR_REVERSE_SHIFT(pd[index].addr), (addr *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while(err < 0);
	index = VADDR_TO_PTENT(va);

	init_ptentry(&pt[index]);
	pt[index].addr = ADDR_SHIFT(pa);
	pt[index].user = !(attributes & LOCK_KERNEL);
	pt[index].rw = attributes & LOCK_RW;
	pt[index].present = 1;

	vm_put_physical_page((addr)pt);

	if(map->arch_data->num_invalidate_pages < PAGE_INVALIDATE_CACHE_SIZE) {
		map->arch_data->pages_to_invalidate[map->arch_data->num_invalidate_pages] = va;
	}
	map->arch_data->num_invalidate_pages++;

	map->map_count++;

	return 0;
}

static int unmap_tmap(vm_translation_map *map, addr start, addr end)
{
	ptentry *pt;
	pdentry *pd = map->arch_data->pgdir_virt;
	int index;
	int err;

	start = ROUNDOWN(start, PAGE_SIZE);
	end = ROUNDUP(end, PAGE_SIZE);

#if CHATTY_TMAP
		dprintf("unmap_tmap: asked to free pages 0x%x to 0x%x\n", start, end);
#endif

restart:
	if(start >= end)
		return 0;

	index = VADDR_TO_PDENT(start);
	if(pd[index].present == 0) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, PAGE_SIZE);
		goto restart;
	}

	do {
		err = vm_get_physical_page(ADDR_REVERSE_SHIFT(pd[index].addr), (addr *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while(err < 0);

	for(index = VADDR_TO_PTENT(start); (index < 1024) && (start < end); index++, start += PAGE_SIZE) {
		if(pt[index].present == 0) {
			// page mapping not valid
			continue;
		}
#if CHATTY_TMAP
		dprintf("unmap_tmap: removing page 0x%x\n", start);
#endif
		pt[index].present = 0;
		map->map_count--;

		if(map->arch_data->num_invalidate_pages < PAGE_INVALIDATE_CACHE_SIZE) {
			map->arch_data->pages_to_invalidate[map->arch_data->num_invalidate_pages] = start;
		}
		map->arch_data->num_invalidate_pages++;
	}

	vm_put_physical_page((addr)pt);

	goto restart;
}

static int query_tmap(vm_translation_map *map, addr va, addr *out_physical, unsigned int *out_flags)
{
	ptentry *pt;
	pdentry *pd = map->arch_data->pgdir_virt;
	int index;
	int err;

	// default the flags to not present
	*out_flags = 0;
	*out_physical = 0;

	index = VADDR_TO_PDENT(va);
	if(pd[index].present == 0) {
		// no pagetable here
		return B_NO_ERROR;
	}

	do {
		err = vm_get_physical_page(ADDR_REVERSE_SHIFT(pd[index].addr), (addr *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while(err < 0);
	index = VADDR_TO_PTENT(va);

	*out_physical = ADDR_REVERSE_SHIFT(pt[index].addr);

	// read in the page state flags, clearing the modified and accessed flags in the process
	*out_flags = 0;
	*out_flags |= pt[index].rw ? LOCK_RW : LOCK_RO;
	*out_flags |= pt[index].user ? 0 : LOCK_KERNEL;
	*out_flags |= pt[index].dirty ? PAGE_MODIFIED : 0;
	*out_flags |= pt[index].accessed ? PAGE_ACCESSED : 0;
	*out_flags |= pt[index].present ? PAGE_PRESENT : 0;

	vm_put_physical_page((addr)pt);

//	dprintf("query_tmap: returning pa 0x%x for va 0x%x\n", *out_physical, va);

	return 0;
}

static addr get_mapped_size_tmap(vm_translation_map *map)
{
	return map->map_count;
}

static int protect_tmap(vm_translation_map *map, addr base, addr top, unsigned int attributes)
{
	// XXX finish
	panic("protect_tmap called, not implemented\n");
	return ERR_UNIMPLEMENTED;
}

static int clear_flags_tmap(vm_translation_map *map, addr va, unsigned int flags)
{
	ptentry *pt;
	pdentry *pd = map->arch_data->pgdir_virt;
	int index;
	int err;
	int tlb_flush = false;

	index = VADDR_TO_PDENT(va);
	if(pd[index].present == 0) {
		// no pagetable here
		return B_NO_ERROR;
	}

	do {
		err = vm_get_physical_page(ADDR_REVERSE_SHIFT(pd[index].addr), (addr *)&pt, PHYSICAL_PAGE_NO_WAIT);
	} while(err < 0);
	index = VADDR_TO_PTENT(va);

	// clear out the flags we've been requested to clear
	if(flags & PAGE_MODIFIED) {
		pt[index].dirty = 0;
		tlb_flush = true;
	}
	if(flags & PAGE_ACCESSED) {
		pt[index].accessed = 0;
		tlb_flush = true;
	}

	vm_put_physical_page((addr)pt);

	if(tlb_flush) {
		if(map->arch_data->num_invalidate_pages < PAGE_INVALIDATE_CACHE_SIZE) {
			map->arch_data->pages_to_invalidate[map->arch_data->num_invalidate_pages] = va;
		}
		map->arch_data->num_invalidate_pages++;
	}

//	dprintf("query_tmap: returning pa 0x%x for va 0x%x\n", *out_physical, va);

	return 0;
}


static void flush_tmap(vm_translation_map *map)
{
	int state;

	if(map->arch_data->num_invalidate_pages <= 0)
		return;

	state = int_disable_interrupts();
	if(map->arch_data->num_invalidate_pages > PAGE_INVALIDATE_CACHE_SIZE) {
		// invalidate all pages
//		dprintf("flush_tmap: %d pages to invalidate, doing global invalidation\n", map->arch_data->num_invalidate_pages);
		arch_cpu_global_TLB_invalidate();
		smp_send_broadcast_ici(SMP_MSG_GLOBAL_INVL_PAGE, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
	} else {
//		dprintf("flush_tmap: %d pages to invalidate, doing local invalidation\n", map->arch_data->num_invalidate_pages);
		arch_cpu_invalidate_TLB_list(map->arch_data->pages_to_invalidate, map->arch_data->num_invalidate_pages);
		smp_send_broadcast_ici(SMP_MSG_INVL_PAGE_LIST, (unsigned long)map->arch_data->pages_to_invalidate,
			map->arch_data->num_invalidate_pages, 0, NULL, SMP_MSG_FLAG_SYNC);
	}
	map->arch_data->num_invalidate_pages = 0;
	int_restore_interrupts(state);
}

static int map_iospace_chunk(addr va, addr pa)
{
	int i;
	ptentry *pt;
	addr ppn;
	int state;

	pa &= ~(PAGE_SIZE - 1); // make sure it's page aligned
	va &= ~(PAGE_SIZE - 1); // make sure it's page aligned
	if(va < IOSPACE_BASE || va >= (IOSPACE_BASE + IOSPACE_SIZE))
		panic("map_iospace_chunk: passed invalid va 0x%lx\n", va);

	ppn = ADDR_SHIFT(pa);
	pt = &iospace_pgtables[(va - IOSPACE_BASE)/PAGE_SIZE];
	for(i=0; i<1024; i++) {
		init_ptentry(&pt[i]);
		pt[i].addr = ppn + i;
		pt[i].user = 0;
		pt[i].rw = 1;
		pt[i].present = 1;
	}

	state = int_disable_interrupts();
	arch_cpu_invalidate_TLB_range(va, va + (IOSPACE_CHUNK_SIZE - PAGE_SIZE));
	smp_send_broadcast_ici(SMP_MSG_INVL_PAGE_RANGE, va, va + (IOSPACE_CHUNK_SIZE - PAGE_SIZE), 0,
		NULL, SMP_MSG_FLAG_SYNC);
	int_restore_interrupts(state);

	return 0;
}

static int get_physical_page_tmap(addr pa, addr *va, int flags)
{
	int index;
	paddr_chunk_desc *replaced_pchunk;

restart:

	mutex_lock(&iospace_mutex);

	// see if the page is already mapped
	index = pa / IOSPACE_CHUNK_SIZE;
	if(paddr_desc[index].va != 0) {
		if(paddr_desc[index].ref_count++ == 0) {
			// pull this descriptor out of the lru list
			queue_remove_item(&mapped_paddr_lru, &paddr_desc[index]);
		}
		*va = paddr_desc[index].va + pa % IOSPACE_CHUNK_SIZE;
		mutex_unlock(&iospace_mutex);
		return 0;
	}

	// map it
	if(first_free_vmapping < num_virtual_chunks) {
		// there's a free hole
		paddr_desc[index].va = first_free_vmapping * IOSPACE_CHUNK_SIZE + IOSPACE_BASE;
		*va = paddr_desc[index].va + pa % IOSPACE_CHUNK_SIZE;
		virtual_pmappings[first_free_vmapping] = &paddr_desc[index];
		paddr_desc[index].ref_count++;

		// push up the first_free_vmapping pointer
		for(; first_free_vmapping < num_virtual_chunks; first_free_vmapping++) {
			if(virtual_pmappings[first_free_vmapping] == NULL)
				break;
		}

		map_iospace_chunk(paddr_desc[index].va, index * IOSPACE_CHUNK_SIZE);
		mutex_unlock(&iospace_mutex);

		return 0;
	}

	// replace an earlier mapping
	if(queue_peek(&mapped_paddr_lru) == NULL) {
		// no free slots available
		if(flags == PHYSICAL_PAGE_NO_WAIT) {
			// punt back to the caller and let them handle this
			mutex_unlock(&iospace_mutex);
			return ERR_NO_MEMORY;
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
	return 0;
}

static int put_physical_page_tmap(addr va)
{
	paddr_chunk_desc *desc;

	if(va < IOSPACE_BASE || va >= IOSPACE_BASE + IOSPACE_SIZE)
		panic("someone called put_physical_page on an invalid va 0x%lx\n", va);
	va -= IOSPACE_BASE;

	mutex_lock(&iospace_mutex);

	desc = virtual_pmappings[va / IOSPACE_CHUNK_SIZE];
	if(desc == NULL) {
		mutex_unlock(&iospace_mutex);
		panic("put_physical_page called on page at va 0x%lx which is not checked out\n", va);
		return ERR_VM_GENERAL;
	}

	if(--desc->ref_count == 0) {
		// put it on the mapped lru list
		queue_enqueue(&mapped_paddr_lru, desc);
		// no sense rescheduling on this one, there's likely a race in the waiting
		// thread to grab the iospace_mutex, which would block and eventually get back to
		// this thread. waste of time.
		release_sem_etc(iospace_full_sem, 1, B_DO_NOT_RESCHEDULE);
	}

	mutex_unlock(&iospace_mutex);

	return 0;
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

int vm_translation_map_create(vm_translation_map *new_map, bool kernel)
{
	if(new_map == NULL)
		return ERR_INVALID_ARGS;

dprintf("vm_translation_map_create\n");
	// initialize the new object
	new_map->ops = &tmap_ops;
	new_map->map_count = 0;
	if(recursive_lock_create(&new_map->lock) < 0)
		return ERR_NO_MEMORY;

	new_map->arch_data = (vm_translation_map_arch_info *)kmalloc(sizeof(vm_translation_map_arch_info));
	if(new_map == NULL)
		return ERR_NO_MEMORY;

	new_map->arch_data->num_invalidate_pages = 0;

	if(!kernel) {
		// user
		// allocate a pgdir
		new_map->arch_data->pgdir_virt = kmalloc(PAGE_SIZE);
		if(new_map->arch_data->pgdir_virt == NULL) {
			kfree(new_map->arch_data);
			return ERR_NO_MEMORY;
		}
		if(((addr)new_map->arch_data->pgdir_virt % PAGE_SIZE) != 0)
			panic("vm_translation_map_create: malloced pgdir and found it wasn't aligned!\n");
		vm_get_page_mapping(vm_get_kernel_aspace_id(), (addr)new_map->arch_data->pgdir_virt, (addr *)&new_map->arch_data->pgdir_phys);
	} else {
		// kernel
		// we already know the kernel pgdir mapping
		(addr)new_map->arch_data->pgdir_virt = kernel_pgdir_virt;
		(addr)new_map->arch_data->pgdir_phys = kernel_pgdir_phys;
	}

	// zero out the bottom portion of the new pgdir
	memset(new_map->arch_data->pgdir_virt + FIRST_USER_PGDIR_ENT, 0, NUM_USER_PGDIR_ENTS * sizeof(pdentry));

	// insert this new map into the map list
	{
		int state = int_disable_interrupts();
		acquire_spinlock(&tmap_list_lock);

		// copy the top portion of the pgdir from the current one
		memcpy(new_map->arch_data->pgdir_virt + FIRST_KERNEL_PGDIR_ENT, kernel_pgdir_virt + FIRST_KERNEL_PGDIR_ENT,
			NUM_KERNEL_PGDIR_ENTS * sizeof(pdentry));

		new_map->next = tmap_list;
		tmap_list = new_map;

		release_spinlock(&tmap_list_lock);
		int_restore_interrupts(state);
	}

	return 0;
}

int vm_translation_map_module_init(kernel_args *ka)
{
	int i;

	dprintf("vm_translation_map_module_init: entry\n");

	// page hole set up in stage2
	page_hole = (ptentry *)ka->arch_args.page_hole;
	// calculate where the pgdir would be
	page_hole_pgdir = (pdentry *)(((unsigned int)ka->arch_args.page_hole) + (PAGE_SIZE * 1024 - PAGE_SIZE));
	// clear out the bottom 2 GB, unmap everything
	memset(page_hole_pgdir + FIRST_USER_PGDIR_ENT, 0, sizeof(pdentry) * NUM_USER_PGDIR_ENTS);

	kernel_pgdir_phys = (pdentry *)ka->arch_args.phys_pgdir;
	kernel_pgdir_virt = (pdentry *)ka->arch_args.vir_pgdir;

	tmap_list_lock = 0;
	tmap_list = NULL;

	// allocate some space to hold physical page mapping info
	paddr_desc = (paddr_chunk_desc *)vm_alloc_from_ka_struct(ka,
		sizeof(paddr_chunk_desc) * 1024, LOCK_RW|LOCK_KERNEL);
	num_virtual_chunks = IOSPACE_SIZE / IOSPACE_CHUNK_SIZE;
	virtual_pmappings = (paddr_chunk_desc **)vm_alloc_from_ka_struct(ka,
		sizeof(paddr_chunk_desc *) * num_virtual_chunks, LOCK_RW|LOCK_KERNEL);
	iospace_pgtables = (ptentry *)vm_alloc_from_ka_struct(ka,
		PAGE_SIZE * (IOSPACE_SIZE / (PAGE_SIZE * 1024)), LOCK_RW|LOCK_KERNEL);

	dprintf("paddr_desc %p, virtual_pmappings %p, iospace_pgtables %p\n",
		paddr_desc, virtual_pmappings, iospace_pgtables);

	// initialize our data structures
	memset(paddr_desc, 0, sizeof(paddr_chunk_desc) * 1024);
	memset(virtual_pmappings, 0, sizeof(paddr_chunk_desc *) * num_virtual_chunks);
	first_free_vmapping = 0;
	queue_init(&mapped_paddr_lru);
	memset(iospace_pgtables, 0, PAGE_SIZE * (IOSPACE_SIZE / (PAGE_SIZE * 1024)));
	iospace_mutex.sem = -1;
	iospace_mutex.count = 0;
	iospace_full_sem = -1;

	dprintf("mapping iospace_pgtables\n");

	// put the array of pgtables directly into the kernel pagedir
	// these will be wired and kept mapped into virtual space to be easy to get to
	{
		addr phys_pgtable;
		addr virt_pgtable;
		pdentry *e;

		virt_pgtable = (addr)iospace_pgtables;
		for(i = 0; i < (IOSPACE_SIZE / (PAGE_SIZE * 1024)); i++, virt_pgtable += PAGE_SIZE) {
			vm_translation_map_quick_query(virt_pgtable, &phys_pgtable);
			e = &page_hole_pgdir[(IOSPACE_BASE / (PAGE_SIZE * 1024)) + i];
			put_pgtable_in_pgdir(e, phys_pgtable, LOCK_RW|LOCK_KERNEL);
		}
	}

	dprintf("vm_translation_map_module_init: done\n");

	return 0;
}


void vm_translation_map_module_init_post_sem(kernel_args *ka)
{
	mutex_init(&iospace_mutex, "iospace_mutex");
	iospace_full_sem = create_sem(1, "iospace_full_sem");
}

int vm_translation_map_module_init2(kernel_args *ka)
{
	// now that the vm is initialized, create an region that represents
	// the page hole
	void *temp;

	dprintf("vm_translation_map_module_init2: entry\n");

	// unmap the page hole hack we were using before
	kernel_pgdir_virt[1023].present = 0;
	page_hole_pgdir = NULL;
	page_hole = NULL;

	temp = (void *)kernel_pgdir_virt;
	vm_create_anonymous_region(vm_get_kernel_aspace_id(), "kernel_pgdir", &temp,
		REGION_ADDR_EXACT_ADDRESS, PAGE_SIZE, REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);

	temp = (void *)paddr_desc;
	vm_create_anonymous_region(vm_get_kernel_aspace_id(), "physical_page_mapping_descriptors", &temp,
		REGION_ADDR_EXACT_ADDRESS, ROUNDUP(sizeof(paddr_chunk_desc) * 1024, PAGE_SIZE),
		REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);

	temp = (void *)virtual_pmappings;
	vm_create_anonymous_region(vm_get_kernel_aspace_id(), "iospace_virtual_chunk_descriptors", &temp,
		REGION_ADDR_EXACT_ADDRESS, ROUNDUP(sizeof(paddr_chunk_desc *) * num_virtual_chunks, PAGE_SIZE),
		REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);

	temp = (void *)iospace_pgtables;
	vm_create_anonymous_region(vm_get_kernel_aspace_id(), "iospace_pgtables", &temp,
		REGION_ADDR_EXACT_ADDRESS, PAGE_SIZE * (IOSPACE_SIZE / (PAGE_SIZE * 1024)),
		REGION_WIRING_WIRED_ALREADY, LOCK_RW|LOCK_KERNEL);

	dprintf("vm_translation_map_module_init2: creating iospace\n");
	temp = (void *)IOSPACE_BASE;
	vm_create_null_region(vm_get_kernel_aspace_id(), "iospace", &temp,
		REGION_ADDR_EXACT_ADDRESS, IOSPACE_SIZE);

	dprintf("vm_translation_map_module_init2: done\n");

	return 0;
}

// XXX horrible back door to map a page quickly regardless of translation map object, etc.
// used only during VM setup.
// uses a 'page hole' set up in the stage 2 bootloader. The page hole is created by pointing one of
// the pgdir entries back at itself, effectively mapping the contents of all of the 4MB of pagetables
// into a 4 MB region. It's only used here, and is later unmapped.
int vm_translation_map_quick_map(kernel_args *ka, addr va, addr pa, unsigned int attributes, addr (*get_free_page)(kernel_args *))
{
	ptentry *pentry;
	int index;

#if CHATTY_TMAP
	dprintf("quick_tmap: entry pa 0x%x va 0x%x\n", pa, va);
#endif

	// check to see if a page table exists for this range
	index = VADDR_TO_PDENT(va);
	if(page_hole_pgdir[index].present == 0) {
		addr pgtable;
		pdentry *e;
		// we need to allocate a pgtable
		pgtable = get_free_page(ka);
		// pgtable is in pages, convert to physical address
		pgtable *= PAGE_SIZE;
#if CHATTY_TMAP
		dprintf("quick_map: asked for free page for pgtable. 0x%x\n", pgtable);
#endif

		// put it in the pgdir
		e = &page_hole_pgdir[index];
		put_pgtable_in_pgdir(e, pgtable, attributes);

		// zero it out in it's new mapping
		memset((unsigned int *)((unsigned int)page_hole + (va / PAGE_SIZE / 1024) * PAGE_SIZE), 0, PAGE_SIZE);
	}
	// now, fill in the pentry
	pentry = page_hole + va / PAGE_SIZE;

	init_ptentry(pentry);
	pentry->addr = ADDR_SHIFT(pa);
	pentry->user = !(attributes & LOCK_KERNEL);
	pentry->rw = attributes & LOCK_RW;
	pentry->present = 1;

	arch_cpu_invalidate_TLB_range(va, va+1);

	return 0;
}

// XXX currently assumes this translation map is active
static int vm_translation_map_quick_query(addr va, addr *out_physical)
{
	ptentry *pentry;

	if(page_hole_pgdir[VADDR_TO_PDENT(va)].present == 0) {
		// no pagetable here
		return ERR_VM_PAGE_NOT_PRESENT;
	}

	pentry = page_hole + va / PAGE_SIZE;
	if(pentry->present == 0) {
		// page mapping not valid
		return ERR_VM_PAGE_NOT_PRESENT;
	}

	*out_physical = pentry->addr << 12;

	return 0;
}

addr vm_translation_map_get_pgdir(vm_translation_map *map)
{
	return (addr)map->arch_data->pgdir_phys;
}
