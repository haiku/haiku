/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <KernelExport.h>
#include <OS.h>

#include <kernel.h>
#include <arch/cpu.h>
#include <vm.h>
#include <vm_priv.h>
#include <vm_page.h>
#include <vm_cache.h>
#include <arch/vm_translation_map.h>
#include <boot/kernel_args.h>

#include <string.h>
#include <stdlib.h>

typedef struct page_queue {
	vm_page *head;
	vm_page *tail;
	int count;
} page_queue;

extern bool trimming_cycle;

static page_queue page_free_queue;
static page_queue page_clear_queue;
static page_queue page_modified_queue;
static page_queue page_active_queue;

static vm_page *all_pages;
static addr_t physical_page_offset;
static unsigned int num_pages;

static spinlock page_lock;

static sem_id modified_pages_available;

static int dump_page(int argc, char **argv);
static int dump_page_queue(int argc, char **argv);
static int dump_page_stats(int argc, char **argv);
static int dump_free_page_table(int argc, char **argv);
static int vm_page_set_state_nolock(vm_page *page, int page_state);
static void clear_page(addr_t pa);
static int32 page_scrubber(void *);


static vm_page *
dequeue_page(page_queue *q)
{
	vm_page *page;

	page = q->tail;
	if (page != NULL) {
		if (q->head == page)
			q->head = NULL;
		if (page->queue_prev != NULL)
			page->queue_prev->queue_next = NULL;

		q->tail = page->queue_prev;
		q->count--;
	}

	return page;
}


static void
enqueue_page(page_queue *q, vm_page *page)
{
	if (q->head != NULL)
		q->head->queue_prev = page;
	page->queue_next = q->head;
	q->head = page;
	page->queue_prev = NULL;
	if (q->tail == NULL)
		q->tail = page;
	q->count++;

	if (q == &page_modified_queue) {
		if (q->count == 1)
			release_sem_etc(modified_pages_available, 1, B_DO_NOT_RESCHEDULE);
	}
}


static void
remove_page_from_queue(page_queue *q, vm_page *page)
{
	if (page->queue_prev != NULL)
		page->queue_prev->queue_next = page->queue_next;
	else
		q->head = page->queue_next;

	if (page->queue_next != NULL)
		page->queue_next->queue_prev = page->queue_prev;
	else
		q->tail = page->queue_prev;

	q->count--;
}


static void
move_page_to_queue(page_queue *from_q, page_queue *to_q, vm_page *page)
{
	if (from_q != to_q) {
		remove_page_from_queue(from_q, page);
		enqueue_page(to_q, page);
	}
}


#if 0
static int pageout_daemon()
{
	int state;
	vm_page *page;
	vm_region *region;
	IOVECS(vecs, 1);
	ssize_t err;

	dprintf("pageout daemon starting\n");

	for (;;) {
		acquire_sem(modified_pages_available);

		dprintf("here\n");

		state = disable_interrupts();
		acquire_spinlock(&page_lock);
		page = dequeue_page(&page_modified_queue);
		page->state = PAGE_STATE_BUSY;
		vm_cache_acquire_ref(page->cache_ref, true);
		release_spinlock(&page_lock);
		restore_interrupts(state);

		dprintf("got page %p\n", page);

		if(page->cache_ref->cache->temporary && !trimming_cycle) {
			// unless we're in the trimming cycle, dont write out pages
			// that back anonymous stores
			state = disable_interrupts();
			acquire_spinlock(&page_lock);
			enqueue_page(&page_modified_queue, page);
			page->state = PAGE_STATE_MODIFIED;
			release_spinlock(&page_lock);
			restore_interrupts(state);
			vm_cache_release_ref(page->cache_ref);
			continue;
		}

		/* clear the modified flag on this page in all it's mappings */
		mutex_lock(&page->cache_ref->lock);
		for(region = page->cache_ref->region_list; region; region = region->cache_next) {
			if(page->offset > region->cache_offset
			  && page->offset < region->cache_offset + region->size) {
				vm_translation_map *map = &region->aspace->translation_map;
				map->ops->lock(map);
				map->ops->clear_flags(map, page->offset - region->cache_offset + region->base, PAGE_MODIFIED);
				map->ops->unlock(map);
			}
		}
		mutex_unlock(&page->cache_ref->lock);

		/* write the page out to it's backing store */
		vecs->num = 1;
		vecs->total_len = PAGE_SIZE;
		vm_get_physical_page(page->ppn * PAGE_SIZE, (addr_t *)&vecs->vec[0].iov_base, PHYSICAL_PAGE_CAN_WAIT);
		vecs->vec[0].iov_len = PAGE_SIZE;

		err = page->cache_ref->cache->store->ops->write(page->cache_ref->cache->store, page->offset, vecs);

		vm_put_physical_page((addr_t)vecs->vec[0].iov_base);

		state = disable_interrupts();
		acquire_spinlock(&page_lock);
		if(page->ref_count > 0) {
			page->state = PAGE_STATE_ACTIVE;
		} else {
			page->state = PAGE_STATE_INACTIVE;
		}
		enqueue_page(&page_active_queue, page);
		release_spinlock(&page_lock);
		restore_interrupts(state);

		vm_cache_release_ref(page->cache_ref);
	}
}
#endif


int
vm_page_init(kernel_args *ka)
{
	unsigned int i;

	dprintf("vm_page_init: entry\n");

	page_lock = 0;

	// initialize queues
	page_free_queue.head = NULL;
	page_free_queue.tail = NULL;
	page_free_queue.count = 0;
	page_clear_queue.head = NULL;
	page_clear_queue.tail = NULL;
	page_clear_queue.count = 0;
	page_modified_queue.head = NULL;
	page_modified_queue.tail = NULL;
	page_modified_queue.count = 0;
	page_active_queue.head = NULL;
	page_active_queue.tail = NULL;
	page_active_queue.count = 0;

	// calculate the size of memory by looking at the physical_memory_range array
	{
		unsigned int last_phys_page = 0;

		physical_page_offset = ka->physical_memory_range[0].start / PAGE_SIZE;
		for (i = 0; i<ka->num_physical_memory_ranges; i++) {
			last_phys_page = (ka->physical_memory_range[i].start + ka->physical_memory_range[i].size) / PAGE_SIZE - 1;
		}
		dprintf("first phys page = 0x%lx, last 0x%x\n", physical_page_offset, last_phys_page);
		num_pages = last_phys_page - physical_page_offset;
	}

	// map in the new free page table
	all_pages = (vm_page *)vm_alloc_from_ka_struct(ka, num_pages * sizeof(vm_page), B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	dprintf("vm_init: putting free_page_table @ %p, # ents %d (size 0x%x)\n",
		all_pages, num_pages, (unsigned int)(num_pages * sizeof(vm_page)));

	// initialize the free page table
	for (i = 0; i < num_pages - 1; i++) {
		all_pages[i].ppn = physical_page_offset + i;
		all_pages[i].type = PAGE_TYPE_PHYSICAL;
		all_pages[i].state = PAGE_STATE_FREE;
		all_pages[i].ref_count = 0;
		enqueue_page(&page_free_queue, &all_pages[i]);
	}

	dprintf("initialized table\n");

	// mark some of the page ranges inuse
	for (i = 0; i < ka->num_physical_allocated_ranges; i++) {
		vm_mark_page_range_inuse(ka->physical_allocated_range[i].start / PAGE_SIZE,
			ka->physical_allocated_range[i].size / PAGE_SIZE);
	}

	// set the global max_commit variable
	vm_increase_max_commit(num_pages*PAGE_SIZE);

	dprintf("vm_page_init: exit\n");

	return 0;
}


int
vm_page_init2(kernel_args *ka)
{
	void *null;

	null = all_pages;
	vm_create_anonymous_region(vm_get_kernel_aspace_id(), "page_structures", &null, B_EXACT_KERNEL_ADDRESS,
		PAGE_ALIGN(num_pages * sizeof(vm_page)), B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	add_debugger_command("page_stats", &dump_page_stats, "Dump statistics about page usage");
	add_debugger_command("free_pages", &dump_free_page_table, "Dump list of free pages");
	add_debugger_command("page", &dump_page, "Dump page info");
	add_debugger_command("page_queue", &dump_page_queue, "Dump page queue");

	return 0;
}


int
vm_page_init_postthread(kernel_args *ka)
{
	thread_id thread;

	// create a kernel thread to clear out pages
	thread = spawn_kernel_thread(&page_scrubber, "page scrubber", B_LOWEST_ACTIVE_PRIORITY, NULL);
	resume_thread(thread);

	modified_pages_available = create_sem(0, "modified_pages_avail_sem");
#if 0
	// create a kernel thread to schedule modified pages to write
	tid = thread_create_kernel_thread("pageout daemon", &pageout_daemon, B_FIRST_REAL_TIME_PRIORITY + 1);
	thread_resume_thread(tid);
#endif
	return 0;
}


static int32
page_scrubber(void *unused)
{
#define SCRUB_SIZE 16
	int state;
	vm_page *page[SCRUB_SIZE];
	int i;
	int scrub_count;

	(void)(unused);

	dprintf("page_scrubber starting...\n");

	for (;;) {
		snooze(100000); // 100ms

		if (page_free_queue.count > 0) {
			state = disable_interrupts();
			acquire_spinlock(&page_lock);

			for (i = 0; i < SCRUB_SIZE; i++) {
				page[i] = dequeue_page(&page_free_queue);
				if (page[i] == NULL)
					break;
			}

			release_spinlock(&page_lock);
			restore_interrupts(state);

			scrub_count = i;

			for (i = 0; i < scrub_count; i++) {
				clear_page(page[i]->ppn * PAGE_SIZE);
			}

			state = disable_interrupts();
			acquire_spinlock(&page_lock);

			for (i = 0; i < scrub_count; i++) {
				page[i]->state = PAGE_STATE_CLEAR;
				enqueue_page(&page_clear_queue, page[i]);
			}

			release_spinlock(&page_lock);
			restore_interrupts(state);
		}
	}

	return 0;
}


static void
clear_page(addr_t pa)
{
	addr_t va;

//	dprintf("clear_page: clearing page 0x%x\n", pa);

	vm_get_physical_page(pa, &va, PHYSICAL_PAGE_CAN_WAIT);

	memset((void *)va, 0, PAGE_SIZE);

	vm_put_physical_page(va);
}


int
vm_mark_page_inuse(addr_t page)
{
	return vm_mark_page_range_inuse(page, 1);
}


int
vm_mark_page_range_inuse(addr_t start_page, addr_t len)
{
	vm_page *page;
	addr_t i;
	int state;

	// XXX remove
	dprintf("vm_mark_page_range_inuse: start 0x%lx, len 0x%lx\n", start_page, len);

	if (physical_page_offset > start_page) {
		dprintf("vm_mark_page_range_inuse: start page %ld is before free list\n", start_page);
		return EINVAL;
	}
	start_page -= physical_page_offset;
	if (start_page + len >= num_pages) {
		dprintf("vm_mark_page_range_inuse: range would extend past free list\n");
		return EINVAL;
	}

	state = disable_interrupts();
	acquire_spinlock(&page_lock);

	for (i = 0; i < len; i++) {
		page = &all_pages[start_page + i];
		switch (page->state) {
			case PAGE_STATE_FREE:
			case PAGE_STATE_CLEAR:
				vm_page_set_state_nolock(page, PAGE_STATE_UNUSED);
				break;
			case PAGE_STATE_WIRED:
				break;
			case PAGE_STATE_ACTIVE:
			case PAGE_STATE_INACTIVE:
			case PAGE_STATE_BUSY:
			case PAGE_STATE_MODIFIED:
			case PAGE_STATE_UNUSED:
			default:
				// uh
				dprintf("vm_mark_page_range_inuse: page 0x%lx in non-free state %d!\n", start_page + i, page->state);
		}
	}

	release_spinlock(&page_lock);
	restore_interrupts(state);

	return i;
}


vm_page *
vm_page_allocate_specific_page(addr_t page_num, int page_state)
{
	vm_page *p;
	int old_page_state = PAGE_STATE_BUSY;
	int state;

	state = disable_interrupts();
	acquire_spinlock(&page_lock);

	p = vm_lookup_page(page_num);
	if (p == NULL)
		goto out;

	switch (p->state) {
		case PAGE_STATE_FREE:
			remove_page_from_queue(&page_free_queue, p);
			break;
		case PAGE_STATE_CLEAR:
			remove_page_from_queue(&page_clear_queue, p);
			break;
		case PAGE_STATE_UNUSED:
			break;
		default:
			// we can't allocate this page
			p = NULL;
	}
	if (p == NULL)
		goto out;

	old_page_state = p->state;
	p->state = PAGE_STATE_BUSY;

	if (old_page_state != PAGE_STATE_UNUSED)
		enqueue_page(&page_active_queue, p);

out:
	release_spinlock(&page_lock);
	restore_interrupts(state);

	if (p != NULL && page_state == PAGE_STATE_CLEAR
		&& (old_page_state == PAGE_STATE_FREE || old_page_state == PAGE_STATE_UNUSED))
		clear_page(p->ppn * PAGE_SIZE);

	return p;
}


vm_page *
vm_page_allocate_page(int page_state)
{
	vm_page *p;
	page_queue *q;
	page_queue *q_other;
	int state;
	int old_page_state;

	switch (page_state) {
		case PAGE_STATE_FREE:
			q = &page_free_queue;
			q_other = &page_clear_queue;
			break;
		case PAGE_STATE_CLEAR:
			q = &page_clear_queue;
			q_other = &page_free_queue;
			break;
		default:
			return NULL; // invalid
	}

	state = disable_interrupts();
	acquire_spinlock(&page_lock);

	p = dequeue_page(q);
	if (p == NULL) {
		// if the primary queue was empty, grap the page from the
		// secondary queue
		p = dequeue_page(q_other);
		if (p == NULL) {
			// XXX hmm
			panic("vm_allocate_page: out of memory! page state = %d\n", page_state);
		}
	}

	old_page_state = p->state;
	p->state = PAGE_STATE_BUSY;

	enqueue_page(&page_active_queue, p);

	release_spinlock(&page_lock);
	restore_interrupts(state);

	// if needed take the page from the free queue and zero it out
	if (page_state == PAGE_STATE_CLEAR && old_page_state == PAGE_STATE_FREE)
		clear_page(p->ppn * PAGE_SIZE);

	return p;
}


vm_page *
vm_page_allocate_page_run(int page_state, addr_t len)
{
	unsigned int start;
	unsigned int i;
	vm_page *first_page = NULL;
	int state;

	start = 0;

	state = disable_interrupts();
	acquire_spinlock(&page_lock);

	for (;;) {
		bool foundit = true;
		if (start + len >= num_pages)
			break;

		for (i = 0; i < len; i++) {
			if (all_pages[start + i].state != PAGE_STATE_FREE
				&& all_pages[start + i].state != PAGE_STATE_CLEAR) {
				foundit = false;
				i++;
				break;
			}
		}
		if (foundit) {
			// pull the pages out of the appropriate queues
			for (i = 0; i < len; i++)
				vm_page_set_state_nolock(&all_pages[start + i], PAGE_STATE_BUSY);
			first_page = &all_pages[start];
			break;
		} else {
			start += i;
			if (start >= num_pages) {
				// no more pages to look through
				break;
			}
		}
	}
	release_spinlock(&page_lock);
	restore_interrupts(state);

	return first_page;
}


vm_page *
vm_lookup_page(addr_t page_num)
{
	if (page_num < physical_page_offset)
		return NULL;

	page_num -= physical_page_offset;
	if (page_num >= num_pages)
		return NULL;

	return &all_pages[page_num];
}


static int
vm_page_set_state_nolock(vm_page *page, int page_state)
{
	page_queue *from_q = NULL;
	page_queue *to_q = NULL;

	switch (page->state) {
		case PAGE_STATE_BUSY:
		case PAGE_STATE_ACTIVE:
		case PAGE_STATE_INACTIVE:
		case PAGE_STATE_WIRED:
		case PAGE_STATE_UNUSED:
			from_q = &page_active_queue;
			break;
		case PAGE_STATE_MODIFIED:
			from_q = &page_modified_queue;
			break;
		case PAGE_STATE_FREE:
			from_q = &page_free_queue;
			break;
		case PAGE_STATE_CLEAR:
			from_q = &page_clear_queue;
			break;
		default:
			panic("vm_page_set_state: vm_page %p in invalid state %d\n", page, page->state);
	}

	switch (page_state) {
		case PAGE_STATE_BUSY:
		case PAGE_STATE_ACTIVE:
		case PAGE_STATE_INACTIVE:
		case PAGE_STATE_WIRED:
		case PAGE_STATE_UNUSED:
			to_q = &page_active_queue;
			break;
		case PAGE_STATE_MODIFIED:
			to_q = &page_modified_queue;
			break;
		case PAGE_STATE_FREE:
			to_q = &page_free_queue;
			break;
		case PAGE_STATE_CLEAR:
			to_q = &page_clear_queue;
			break;
		default:
			panic("vm_page_set_state: invalid target state %d\n", page_state);
	}
	move_page_to_queue(from_q, to_q, page);
	page->state = page_state;

	return 0;
}


int
vm_page_set_state(vm_page *page, int page_state)
{
	int err;
	int state = disable_interrupts();
	acquire_spinlock(&page_lock);

	err = vm_page_set_state_nolock(page, page_state);

	release_spinlock(&page_lock);
	restore_interrupts(state);

	return err;
}


addr_t
vm_page_num_pages(void)
{
	return num_pages;
}


addr_t
vm_page_num_free_pages(void)
{
	return page_free_queue.count + page_clear_queue.count;
}


static int
dump_free_page_table(int argc, char **argv)
{
	dprintf("not finished\n");
	return 0;
}


static int
dump_page(int argc, char **argv)
{
	struct vm_page *page;

	if (argc < 2
		|| strlen(argv[1]) <= 2
		|| argv[1][0] != '0'
		|| argv[1][1] != 'x') {
		dprintf("usage: page_queue <address>\n");
		return 0;
	}

	page = (struct vm_page *)(atoul(argv[1]));

	dprintf("queue_next = %p, queue_prev = %p, type = %d, state = %d\n", page->queue_next, page->queue_prev, page->type, page->state);
	return 0;
}


static int
dump_page_queue(int argc, char **argv)
{
	struct page_queue *queue;

	if (argc < 2
		|| strlen(argv[1]) <= 2
		|| argv[1][0] != '0'
		|| argv[1][1] != 'x') {
		dprintf("usage: page_queue <address> [list]\n");
		return 0;
	}

	queue = (struct page_queue *)(atoul(argv[1]));

	dprintf("queue->head = %p, queue->tail = %p, queue->count = %d\n", queue->head, queue->tail, queue->count);

	if (argc == 3) {
		struct vm_page *page = queue->head;
		int i;
		
		for (i = 0; page; i++, page = page->queue_next) {
			dprintf("%5d. queue_next = %p, queue_prev = %p, type = %d, state = %d\n", i, page->queue_next, page->queue_prev, page->type, page->state);
		}
	}
	return 0;
}


static int
dump_page_stats(int argc, char **argv)
{
	unsigned int page_types[8];
	addr_t i;

	memset(page_types, 0, sizeof(page_types));

	for (i = 0; i < num_pages; i++) {
		page_types[all_pages[i].state]++;
	}

	dprintf("page stats:\n");
	dprintf("active: %d\ninactive: %d\nbusy: %d\nunused: %d\n",
		page_types[PAGE_STATE_ACTIVE], page_types[PAGE_STATE_INACTIVE], page_types[PAGE_STATE_BUSY], page_types[PAGE_STATE_UNUSED]);
	dprintf("modified: %d\nfree: %d\nclear: %d\nwired: %d\n",
		page_types[PAGE_STATE_MODIFIED], page_types[PAGE_STATE_FREE], page_types[PAGE_STATE_CLEAR], page_types[PAGE_STATE_WIRED]);

	dprintf("\nfree_queue: %p, count = %d\n", &page_free_queue, page_free_queue.count);
	dprintf("clear_queue: %p, count = %d\n", &page_clear_queue, page_clear_queue.count);
	dprintf("modified_queue: %p, count = %d\n", &page_modified_queue, page_modified_queue.count);
	dprintf("active_queue: %p, count = %d\n", &page_active_queue, page_active_queue.count);

	return 0;
}


#if 0
static int dump_free_page_table(int argc, char **argv)
{
	unsigned int i = 0;
	unsigned int free_start = END_OF_LIST;
	unsigned int inuse_start = PAGE_INUSE;

	dprintf("dump_free_page_table():\n");
	dprintf("first_free_page_index = %d\n", first_free_page_index);

	while(i < free_page_table_size) {
		if(free_page_table[i] == PAGE_INUSE) {
			if(inuse_start != PAGE_INUSE) {
				i++;
				continue;
			}
			if(free_start != END_OF_LIST) {
				dprintf("free from %d -> %d\n", free_start + free_page_table_base, i-1 + free_page_table_base);
				free_start = END_OF_LIST;
			}
			inuse_start = i;
		} else {
			if(free_start != END_OF_LIST) {
				i++;
				continue;
			}
			if(inuse_start != PAGE_INUSE) {
				dprintf("inuse from %d -> %d\n", inuse_start + free_page_table_base, i-1 + free_page_table_base);
				inuse_start = PAGE_INUSE;
			}
			free_start = i;
		}
		i++;
	}
	if(inuse_start != PAGE_INUSE) {
		dprintf("inuse from %d -> %d\n", inuse_start + free_page_table_base, i-1 + free_page_table_base);
	}
	if(free_start != END_OF_LIST) {
		dprintf("free from %d -> %d\n", free_start + free_page_table_base, i-1 + free_page_table_base);
	}
/*
	for(i=0; i<free_page_table_size; i++) {
		dprintf("%d->%d ", i, free_page_table[i]);
	}
*/
	return 0;
}
#endif


static addr_t
vm_alloc_vspace_from_ka_struct(kernel_args *ka, unsigned int size)
{
	addr_t spot = 0;
	uint32 i;
	int last_valloc_entry = 0;

	size = PAGE_ALIGN(size);
	// find a slot in the virtual allocation addr range
	for (i = 1; i < ka->num_virtual_allocated_ranges; i++) {
		last_valloc_entry = i;
		// check to see if the space between this one and the last is big enough
		if (ka->virtual_allocated_range[i].start 
			- (ka->virtual_allocated_range[i-1].start 
				+ ka->virtual_allocated_range[i-1].size) >= size) {
			spot = ka->virtual_allocated_range[i-1].start + ka->virtual_allocated_range[i-1].size;
			ka->virtual_allocated_range[i-1].size += size;
			goto out;
		}
	}
	if (spot == 0) {
		// we hadn't found one between allocation ranges. this is ok.
		// see if there's a gap after the last one
		if (ka->virtual_allocated_range[last_valloc_entry].start 
				+ ka->virtual_allocated_range[last_valloc_entry].size + size 
			<= KERNEL_BASE + (KERNEL_SIZE - 1)) {
			spot = ka->virtual_allocated_range[last_valloc_entry].start + ka->virtual_allocated_range[last_valloc_entry].size;
			ka->virtual_allocated_range[last_valloc_entry].size += size;
			goto out;
		}
		// see if there's a gap before the first one
		if (ka->virtual_allocated_range[0].start > KERNEL_BASE) {
			if (ka->virtual_allocated_range[0].start - KERNEL_BASE >= size) {
				ka->virtual_allocated_range[0].start -= size;
				spot = ka->virtual_allocated_range[0].start;
				goto out;
			}
		}
	}

out:
	return spot;
}


static bool
is_page_in_phys_range(kernel_args *ka, addr_t paddr)
{
	// XXX horrible brute-force method of determining if the page can be allocated
	unsigned int i;

	for (i = 0; i < ka->num_physical_memory_ranges; i++) {
		if (paddr >= ka->physical_memory_range[i].start 
			&& paddr < ka->physical_memory_range[i].start 
						+ ka->physical_memory_range[i].size) {
			return true;
		}
	}
	return false;
}


static addr_t
vm_alloc_ppage_from_kernel_struct(kernel_args *ka)
{
	uint32 i;

	for (i = 0; i < ka->num_physical_allocated_ranges; i++) {
		addr_t next_page;

		next_page = ka->physical_allocated_range[i].start + ka->physical_allocated_range[i].size;
		// see if the page after the next allocated paddr run can be allocated
		if (i + 1 < ka->num_physical_allocated_ranges && ka->physical_allocated_range[i+1].size != 0) {
			// see if the next page will collide with the next allocated range
			if (next_page >= ka->physical_allocated_range[i+1].start)
				continue;
		}
		// see if the next physical page fits in the memory block
		if (is_page_in_phys_range(ka, next_page)) {
			// we got one!
			ka->physical_allocated_range[i].size += PAGE_SIZE;
			return ((ka->physical_allocated_range[i].start + ka->physical_allocated_range[i].size - PAGE_SIZE) / PAGE_SIZE);
		}
	}

	return 0;	// could not allocate a block
}


addr_t
vm_alloc_from_ka_struct(kernel_args *ka, unsigned int size, int lock)
{
	addr_t vspot, pspot;
	uint32 i;

	// find the vaddr to allocate at
	vspot = vm_alloc_vspace_from_ka_struct(ka, size);
	//dprintf("alloc_from_ka_struct: vaddr 0x%lx\n", vspot);

	// map the pages
	for (i = 0; i < PAGE_ALIGN(size) / B_PAGE_SIZE; i++) {
		pspot = vm_alloc_ppage_from_kernel_struct(ka);
		//dprintf("alloc_from_ka_struct: paddr 0x%lx\n", pspot);
		if (pspot == 0)
			panic("error allocating page from ka_struct!\n");
		vm_translation_map_quick_map(ka, vspot + i*PAGE_SIZE,
			pspot * PAGE_SIZE, lock, &vm_alloc_ppage_from_kernel_struct);
	}

	return vspot;
}
