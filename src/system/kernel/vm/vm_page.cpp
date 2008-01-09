/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include <KernelExport.h>
#include <OS.h>

#include <arch/cpu.h>
#include <arch/vm_translation_map.h>
#include <boot/kernel_args.h>
#include <condition_variable.h>
#include <kernel.h>
#include <thread.h>
#include <util/AutoLock.h>
#include <vm.h>
#include <vm_address_space.h>
#include <vm_low_memory.h>
#include <vm_priv.h>
#include <vm_page.h>
#include <vm_cache.h>

#include "PageCacheLocker.h"


//#define TRACE_VM_PAGE
#ifdef TRACE_VM_PAGE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

//#define TRACK_PAGE_ALLOCATIONS 1
	// [Debugging feature] Enables a small history of page allocation operations
	// that can be listed in KDL via "page_allocations".

#define SCRUB_SIZE 16
	// this many pages will be cleared at once in the page scrubber thread

typedef struct page_queue {
	vm_page *head;
	vm_page *tail;
	uint32	count;
} page_queue;

static page_queue sFreePageQueue;
static page_queue sClearPageQueue;
static page_queue sModifiedPageQueue;
static page_queue sInactivePageQueue;
static page_queue sActivePageQueue;

static vm_page *sPages;
static addr_t sPhysicalPageOffset;
static size_t sNumPages;
static size_t sReservedPages;
static vint32 sPageDeficit;
static size_t sModifiedTemporaryPages;

static ConditionVariable<page_queue> sFreePageCondition;
static spinlock sPageLock;

static sem_id sWriterWaitSem;


#if TRACK_PAGE_ALLOCATIONS


enum {
	NO_PAGE_OPERATION = 0,
	RESERVE_PAGES,
	UNRESERVE_PAGES,
	ALLOCATE_PAGE,
	ALLOCATE_PAGE_RUN,
	FREE_PAGE,
	SCRUBBING_PAGES,
	SCRUBBED_PAGES,
	STOLEN_PAGE
};


struct allocation_history_entry {
	bigtime_t	time;
	thread_id	thread;
	uint32		operation;
	uint32		count;
	uint32		flags;
	uint32		free;
	uint32		reserved;
};


static const int kAllocationHistoryCapacity = 512;
static allocation_history_entry sAllocationHistory[kAllocationHistoryCapacity];
static int kAllocationHistoryIndex = 0;


/*!	sPageLock must be held.
*/
static void
add_allocation_history_entry(uint32 operation, uint32 count, uint32 flags)
{
	allocation_history_entry& entry
		= sAllocationHistory[kAllocationHistoryIndex];
	kAllocationHistoryIndex = (kAllocationHistoryIndex + 1)
		% kAllocationHistoryCapacity;

	entry.time = system_time();
	entry.thread = find_thread(NULL);
	entry.operation = operation;
	entry.count = count;
	entry.flags = flags;
	entry.free = sFreePageQueue.count + sClearPageQueue.count;
	entry.reserved = sReservedPages;
}


static int
dump_page_allocations(int argc, char **argv)
{
	kprintf("thread  operation  count  flags      free  reserved        time\n");
	kprintf("---------------------------------------------------------------\n");

	for (int i = 0; i < kAllocationHistoryCapacity; i++) {
		int index = (kAllocationHistoryIndex + i) % kAllocationHistoryCapacity;
		allocation_history_entry& entry = sAllocationHistory[index];

		const char* operation;
		switch (entry.operation) {
			case NO_PAGE_OPERATION:
				operation = "no op";
				break;
			case RESERVE_PAGES:
				operation = "reserve";
				break;
			case UNRESERVE_PAGES:
				operation = "unreserve";
				break;
			case ALLOCATE_PAGE:
				operation = "alloc";
				break;
			case ALLOCATE_PAGE_RUN:
				operation = "alloc run";
				break;
			case FREE_PAGE:
				operation = "free";
				break;
			case SCRUBBING_PAGES:
				operation = "scrubbing";
				break;
			case SCRUBBED_PAGES:
				operation = "scrubbed";
				break;
			case STOLEN_PAGE:
				operation = "stolen";
				break;
			default:
				operation = "invalid";
				break;
		}

		kprintf("%6ld  %-9s  %5ld  %5lx  %8ld  %8ld  %10lld\n",
			entry.thread, operation, entry.count, entry.flags, entry.free,
			entry.reserved, entry.time);
	}

	return 0;
}


#endif	// TRACK_PAGE_ALLOCATIONS


/*!	Dequeues a page from the head of the given queue */
static vm_page *
dequeue_page(page_queue *queue)
{
	vm_page *page;

	page = queue->head;
	if (page != NULL) {
		if (queue->tail == page)
			queue->tail = NULL;
		if (page->queue_next != NULL)
			page->queue_next->queue_prev = NULL;

		queue->head = page->queue_next;
		queue->count--;

#ifdef DEBUG_PAGE_QUEUE
		if (page->queue != queue) {
			panic("dequeue_page(queue: %p): page %p thinks it is in queue "
				"%p", queue, page, page->queue);
		}

		page->queue = NULL;
#endif	// DEBUG_PAGE_QUEUE
	}

	return page;
}


/*!	Enqueues a page to the tail of the given queue */
static void
enqueue_page(page_queue *queue, vm_page *page)
{
#ifdef DEBUG_PAGE_QUEUE
	if (page->queue != NULL) {
		panic("enqueue_page(queue: %p, page: %p): page thinks it is "
			"already in queue %p", queue, page, page->queue);
	}
#endif	// DEBUG_PAGE_QUEUE

	if (queue->tail != NULL)
		queue->tail->queue_next = page;
	page->queue_prev = queue->tail;
	queue->tail = page;
	page->queue_next = NULL;
	if (queue->head == NULL)
		queue->head = page;
	queue->count++;

#ifdef DEBUG_PAGE_QUEUE
	page->queue = queue;
#endif
}


/*!	Enqueues a page to the head of the given queue */
static void
enqueue_page_to_head(page_queue *queue, vm_page *page)
{
#ifdef DEBUG_PAGE_QUEUE
	if (page->queue != NULL) {
		panic("enqueue_page_to_head(queue: %p, page: %p): page thinks it is "
			"already in queue %p", queue, page, page->queue);
	}
#endif	// DEBUG_PAGE_QUEUE

	if (queue->head != NULL)
		queue->head->queue_prev = page;
	page->queue_next = queue->head;
	queue->head = page;
	page->queue_prev = NULL;
	if (queue->tail == NULL)
		queue->tail = page;
	queue->count++;

#ifdef DEBUG_PAGE_QUEUE
	page->queue = queue;
#endif
}


static void
remove_page_from_queue(page_queue *queue, vm_page *page)
{
#ifdef DEBUG_PAGE_QUEUE
	if (page->queue != queue) {
		panic("remove_page_from_queue(queue: %p, page: %p): page thinks it "
			"is in queue %p", queue, page, page->queue);
	}
#endif	// DEBUG_PAGE_QUEUE

	if (page->queue_next != NULL)
		page->queue_next->queue_prev = page->queue_prev;
	else
		queue->tail = page->queue_prev;

	if (page->queue_prev != NULL)
		page->queue_prev->queue_next = page->queue_next;
	else
		queue->head = page->queue_next;

	queue->count--;

#ifdef DEBUG_PAGE_QUEUE
	page->queue = NULL;
#endif
}


/*!	Moves a page to the tail of the given queue, but only does so if
	the page is currently in another queue.
*/
static void
move_page_to_queue(page_queue *fromQueue, page_queue *toQueue, vm_page *page)
{
	if (fromQueue != toQueue) {
		remove_page_from_queue(fromQueue, page);
		enqueue_page(toQueue, page);
	}
}


/*! Inserts \a page after the \a before page in the \a queue. */
static void
insert_page_after(page_queue *queue, vm_page *before, vm_page *page)
{
#ifdef DEBUG_PAGE_QUEUE
	if (page->queue != NULL) {
		panic("enqueue_page(queue: %p, page: %p): page thinks it is "
			"already in queue %p", queue, page, page->queue);
	}
#endif	// DEBUG_PAGE_QUEUE

	if (before == NULL) {
		enqueue_page(queue, page);
		return;
	}

	page->queue_next = before->queue_next;
	if (page->queue_next != NULL)
		page->queue_next->queue_prev = page;
	page->queue_prev = before;
	before->queue_next = page;

	if (queue->tail == before)
		queue->tail = page;

	queue->count++;

#ifdef DEBUG_PAGE_QUEUE
	page->queue = queue;
#endif
}


static int
find_page(int argc, char **argv)
{
	struct vm_page *page;
	addr_t address;
	int32 index = 1;
	int i;

	struct {
		const char*	name;
		page_queue*	queue;
	} pageQueueInfos[] = {
		{ "free",		&sFreePageQueue },
		{ "clear",		&sClearPageQueue },
		{ "modified",	&sModifiedPageQueue },
		{ "active",		&sActivePageQueue },
		{ NULL, NULL }
	};

	if (argc < 2
		|| strlen(argv[index]) <= 2
		|| argv[index][0] != '0'
		|| argv[index][1] != 'x') {
		kprintf("usage: find_page <address>\n");
		return 0;
	}

	address = strtoul(argv[index], NULL, 0);
	page = (vm_page*)address;

	for (i = 0; pageQueueInfos[i].name; i++) {
		vm_page* p = pageQueueInfos[i].queue->head;
		while (p) {
			if (p == page) {
				kprintf("found page %p in queue %p (%s)\n", page,
					pageQueueInfos[i].queue, pageQueueInfos[i].name);
				return 0;
			}
			p = p->queue_next;
		}
	}

	kprintf("page %p isn't in any queue\n", page);

	return 0;
}


const char *
page_state_to_string(int state)
{
	switch(state) {
		case PAGE_STATE_ACTIVE:
			return "active";
		case PAGE_STATE_INACTIVE:
			return "inactive";
		case PAGE_STATE_BUSY:
			return "busy";
		case PAGE_STATE_MODIFIED:
			return "modified";
		case PAGE_STATE_FREE:
			return "free";
		case PAGE_STATE_CLEAR:
			return "clear";
		case PAGE_STATE_WIRED:
			return "wired";
		case PAGE_STATE_UNUSED:
			return "unused";
		default:
			return "unknown";
	}
}


static int
dump_page(int argc, char **argv)
{
	struct vm_page *page;
	addr_t address;
	bool physical = false;
	int32 index = 1;

	if (argc > 2) {
		if (!strcmp(argv[1], "-p")) {
			physical = true;
			index++;
		} else if (!strcmp(argv[1], "-v"))
			index++;
	}

	if (argc < 2
		|| strlen(argv[index]) <= 2
		|| argv[index][0] != '0'
		|| argv[index][1] != 'x') {
		kprintf("usage: page [-p|-v] <address>\n"
			"  -v looks up a virtual address for the page, -p a physical address.\n"
			"  Default is to look for the page structure address directly.\n");
		return 0;
	}

	address = strtoul(argv[index], NULL, 0);

	if (index == 2) {
		if (!physical) {
			vm_address_space *addressSpace = vm_kernel_address_space();
			uint32 flags;

			if (thread_get_current_thread()->team->address_space != NULL)
				addressSpace = thread_get_current_thread()->team->address_space;

			addressSpace->translation_map.ops->query_interrupt(
				&addressSpace->translation_map, address, &address, &flags);
		}
		page = vm_lookup_page(address / B_PAGE_SIZE);
	} else
		page = (struct vm_page *)address;

	kprintf("PAGE: %p\n", page);
	kprintf("queue_next,prev: %p, %p\n", page->queue_next, page->queue_prev);
	kprintf("hash_next:       %p\n", page->hash_next);
	kprintf("physical_number: %lx\n", page->physical_page_number);
	kprintf("cache:           %p\n", page->cache);
	kprintf("cache_offset:    %ld\n", page->cache_offset);
	kprintf("cache_next,prev: %p, %p\n", page->cache_next, page->cache_prev);
	kprintf("type:            %d\n", page->type);
	kprintf("state:           %s\n", page_state_to_string(page->state));
	kprintf("wired_count:     %d\n", page->wired_count);
	kprintf("usage_count:     %d\n", page->usage_count);
	#ifdef DEBUG_PAGE_QUEUE
		kprintf("queue:           %p\n", page->queue);
	#endif
	#ifdef DEBUG_PAGE_CACHE_TRANSITIONS
		kprintf("debug_flags:     0x%lx\n", page->debug_flags);
		kprintf("collided page:   %p\n", page->collided_page);
	#endif	// DEBUG_PAGE_CACHE_TRANSITIONS
	kprintf("area mappings:\n");

	vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
	vm_page_mapping *mapping;
	while ((mapping = iterator.Next()) != NULL) {
		kprintf("  %p (%#lx)\n", mapping->area, mapping->area->id);
		mapping = mapping->page_link.next;
	}

	return 0;
}


static int
dump_page_queue(int argc, char **argv)
{
	struct page_queue *queue;

	if (argc < 2) {
		kprintf("usage: page_queue <address/name> [list]\n");
		return 0;
	}

	if (strlen(argv[1]) >= 2 && argv[1][0] == '0' && argv[1][1] == 'x')
		queue = (struct page_queue *)strtoul(argv[1], NULL, 16);
	if (!strcmp(argv[1], "free"))
		queue = &sFreePageQueue;
	else if (!strcmp(argv[1], "clear"))
		queue = &sClearPageQueue;
	else if (!strcmp(argv[1], "modified"))
		queue = &sModifiedPageQueue;
	else if (!strcmp(argv[1], "active"))
		queue = &sActivePageQueue;
	else if (!strcmp(argv[1], "inactive"))
		queue = &sInactivePageQueue;
	else {
		kprintf("page_queue: unknown queue \"%s\".\n", argv[1]);
		return 0;
	}

	kprintf("queue = %p, queue->head = %p, queue->tail = %p, queue->count = %ld\n",
		queue, queue->head, queue->tail, queue->count);

	if (argc == 3) {
		struct vm_page *page = queue->head;
		const char *type = "none";
		int i;

		if (page->cache != NULL) {
			switch (page->cache->type) {
				case CACHE_TYPE_RAM:
					type = "RAM";
					break;
				case CACHE_TYPE_DEVICE:
					type = "device";
					break;
				case CACHE_TYPE_VNODE:
					type = "vnode";
					break;
				case CACHE_TYPE_NULL:
					type = "null";
					break;
				default:
					type = "???";
					break;
			}
		}

		kprintf("page        cache       type       state  wired  usage\n");
		for (i = 0; page; i++, page = page->queue_next) {
			kprintf("%p  %p  %-7s %8s  %5d  %5d\n", page, page->cache,
				type, page_state_to_string(page->state),
				page->wired_count, page->usage_count);
		}
	}
	return 0;
}


static int
dump_page_stats(int argc, char **argv)
{
	uint32 counter[8];
	int32 totalActive;
	addr_t i;

	memset(counter, 0, sizeof(counter));

	for (i = 0; i < sNumPages; i++) {
		if (sPages[i].state > 7)
			panic("page %li at %p has invalid state!\n", i, &sPages[i]);

		counter[sPages[i].state]++;
	}

	kprintf("page stats:\n");
	kprintf("active: %lu\ninactive: %lu\nbusy: %lu\nunused: %lu\n",
		counter[PAGE_STATE_ACTIVE], counter[PAGE_STATE_INACTIVE],
		counter[PAGE_STATE_BUSY], counter[PAGE_STATE_UNUSED]);
	kprintf("wired: %lu\nmodified: %lu\nfree: %lu\nclear: %lu\n",
		counter[PAGE_STATE_WIRED], counter[PAGE_STATE_MODIFIED],
		counter[PAGE_STATE_FREE], counter[PAGE_STATE_CLEAR]);
	kprintf("reserved pages: %lu\n", sReservedPages);
	kprintf("page deficit: %lu\n", sPageDeficit);

	kprintf("\nfree queue: %p, count = %ld\n", &sFreePageQueue,
		sFreePageQueue.count);
	kprintf("clear queue: %p, count = %ld\n", &sClearPageQueue,
		sClearPageQueue.count);
	kprintf("modified queue: %p, count = %ld (%ld temporary)\n",
		&sModifiedPageQueue, sModifiedPageQueue.count, sModifiedTemporaryPages);
	kprintf("active queue: %p, count = %ld\n", &sActivePageQueue,
		sActivePageQueue.count);
	kprintf("inactive queue: %p, count = %ld\n", &sInactivePageQueue,
		sInactivePageQueue.count);
	return 0;
}


static inline size_t
free_page_queue_count(void)
{
	return sFreePageQueue.count + sClearPageQueue.count;
}


static status_t
set_page_state_nolock(vm_page *page, int pageState)
{
	if (pageState == page->state)
		return B_OK;

	page_queue *fromQueue = NULL;
	page_queue *toQueue = NULL;

	switch (page->state) {
		case PAGE_STATE_BUSY:
		case PAGE_STATE_ACTIVE:
		case PAGE_STATE_WIRED:
		case PAGE_STATE_UNUSED:
			fromQueue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			fromQueue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			fromQueue = &sModifiedPageQueue;
			break;
		case PAGE_STATE_FREE:
			fromQueue = &sFreePageQueue;
			break;
		case PAGE_STATE_CLEAR:
			fromQueue = &sClearPageQueue;
			break;
		default:
			panic("vm_page_set_state: vm_page %p in invalid state %d\n",
				page, page->state);
			break;
	}

	if (page->state == PAGE_STATE_CLEAR || page->state == PAGE_STATE_FREE) {
		if (page->cache != NULL)
			panic("free page %p has cache", page);
	}

	switch (pageState) {
		case PAGE_STATE_BUSY:
		case PAGE_STATE_ACTIVE:
		case PAGE_STATE_WIRED:
		case PAGE_STATE_UNUSED:
			toQueue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			toQueue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			toQueue = &sModifiedPageQueue;
			break;
		case PAGE_STATE_FREE:
			toQueue = &sFreePageQueue;
			break;
		case PAGE_STATE_CLEAR:
			toQueue = &sClearPageQueue;
			break;
		default:
			panic("vm_page_set_state: invalid target state %d\n", pageState);
	}

	if (pageState == PAGE_STATE_CLEAR || pageState == PAGE_STATE_FREE
		|| pageState == PAGE_STATE_INACTIVE) {
		if (sPageDeficit > 0)
			sFreePageCondition.NotifyOne();

		if (pageState != PAGE_STATE_INACTIVE && page->cache != NULL)
			panic("to be freed page %p has cache", page);
	}
	if (page->cache != NULL) {
		if (pageState == PAGE_STATE_MODIFIED && page->cache->temporary)
			sModifiedTemporaryPages++;
		else if (page->state == PAGE_STATE_MODIFIED && page->cache->temporary)
			sModifiedTemporaryPages--;
	}

#if TRACK_PAGE_ALLOCATIONS
	if ((pageState == PAGE_STATE_CLEAR || pageState == PAGE_STATE_FREE)
		&& page->state != PAGE_STATE_CLEAR && page->state != PAGE_STATE_FREE) {
		add_allocation_history_entry(FREE_PAGE, 1, 0);
		
	}
#endif	// TRACK_PAGE_ALLOCATIONS

	page->state = pageState;
	move_page_to_queue(fromQueue, toQueue, page);

	return B_OK;
}


/*! Moves a modified page into either the active or inactive page queue
	depending on its usage count and wiring.
*/
static void
move_page_to_active_or_inactive_queue(vm_page *page, bool dequeued)
{
	// Note, this logic must be in sync with what the page daemon does
	int32 state;
	if (!page->mappings.IsEmpty() || page->usage_count >= 0
		|| page->wired_count)
		state = PAGE_STATE_ACTIVE;
	else
		state = PAGE_STATE_INACTIVE;

	if (dequeued) {
		page->state = state;
		enqueue_page(state == PAGE_STATE_ACTIVE
			? &sActivePageQueue : &sInactivePageQueue, page);
		if (page->cache->temporary)
			sModifiedTemporaryPages--;
	} else
		set_page_state_nolock(page, state);
}


static void
clear_page(struct vm_page *page)
{
	addr_t virtualAddress;
	vm_get_physical_page(page->physical_page_number << PAGE_SHIFT,
		&virtualAddress, PHYSICAL_PAGE_CAN_WAIT);

	memset((void *)virtualAddress, 0, B_PAGE_SIZE);

	vm_put_physical_page(virtualAddress);
}


/*!
	This is a background thread that wakes up every now and then (every 100ms)
	and moves some pages from the free queue over to the clear queue.
	Given enough time, it will clear out all pages from the free queue - we
	could probably slow it down after having reached a certain threshold.
*/
static int32
page_scrubber(void *unused)
{
	(void)(unused);

	TRACE(("page_scrubber starting...\n"));

	for (;;) {
		snooze(100000); // 100ms

		if (sFreePageQueue.count > 0) {
			cpu_status state;
			vm_page *page[SCRUB_SIZE];
			int32 i, scrubCount;

			// get some pages from the free queue

			state = disable_interrupts();
			acquire_spinlock(&sPageLock);

			// Since we temporarily remove pages from the free pages reserve,
			// we must make sure we don't cause a violation of the page
			// reservation warranty. The following is usually stricter than
			// necessary, because we don't have information on how many of the
			// reserved pages have already been allocated.
			scrubCount = SCRUB_SIZE;
			uint32 freeCount = free_page_queue_count();
			if (freeCount < sReservedPages)
				scrubCount = 0;
			else if ((uint32)scrubCount > freeCount - sReservedPages)
				scrubCount = freeCount - sReservedPages;

			for (i = 0; i < scrubCount; i++) {
				page[i] = dequeue_page(&sFreePageQueue);
				if (page[i] == NULL)
					break;
				page[i]->state = PAGE_STATE_BUSY;
			}

			scrubCount = i;

#if TRACK_PAGE_ALLOCATIONS
			if (scrubCount > 0)
				add_allocation_history_entry(SCRUBBING_PAGES, scrubCount, 0);
#endif

			release_spinlock(&sPageLock);
			restore_interrupts(state);

			// clear them

			for (i = 0; i < scrubCount; i++) {
				clear_page(page[i]);
			}

			state = disable_interrupts();
			acquire_spinlock(&sPageLock);

			// and put them into the clear queue

			for (i = 0; i < scrubCount; i++) {
				page[i]->state = PAGE_STATE_CLEAR;
				enqueue_page(&sClearPageQueue, page[i]);
			}

#if TRACK_PAGE_ALLOCATIONS
			if (scrubCount > 0)
				add_allocation_history_entry(SCRUBBED_PAGES, scrubCount, 0);
#endif

			release_spinlock(&sPageLock);
			restore_interrupts(state);
		}
	}

	return 0;
}


static status_t
write_page(vm_page *page, bool fsReenter)
{
	vm_store *store = page->cache->store;
	size_t length = B_PAGE_SIZE;
	status_t status;
	iovec vecs[1];

	TRACE(("write_page(page = %p): offset = %Ld\n", page, (off_t)page->cache_offset << PAGE_SHIFT));

	status = vm_get_physical_page(page->physical_page_number * B_PAGE_SIZE,
		(addr_t *)&vecs[0].iov_base, PHYSICAL_PAGE_CAN_WAIT);
	if (status < B_OK)
		panic("could not map page!");
	vecs->iov_len = B_PAGE_SIZE;

	status = store->ops->write(store, (off_t)page->cache_offset << PAGE_SHIFT,
		vecs, 1, &length, fsReenter);

	vm_put_physical_page((addr_t)vecs[0].iov_base);
#if 0
	if (status < B_OK) {
		dprintf("write_page(page = %p): offset = %lx, status = %ld\n",
			page, page->cache_offset, status);
	}
#endif
	return status;
}


static void
remove_page_marker(struct vm_page &marker)
{
	if (marker.state == PAGE_STATE_UNUSED)
		return;

	page_queue *queue;
	vm_page *page;

	switch (marker.state) {
		case PAGE_STATE_ACTIVE:
			queue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			queue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			queue = &sModifiedPageQueue;
			break;

		default:
			return;
	}

	remove_page_from_queue(queue, &marker);
	marker.state = PAGE_STATE_UNUSED;
}


static vm_page *
next_modified_page(struct vm_page &marker)
{
	InterruptsSpinLocker locker(sPageLock);
	vm_page *page;

	if (marker.state == PAGE_STATE_MODIFIED) {
		page = marker.queue_next;
		remove_page_from_queue(&sModifiedPageQueue, &marker);
		marker.state = PAGE_STATE_UNUSED;
	} else
		page = sModifiedPageQueue.head;

	for (; page != NULL; page = page->queue_next) {
		if (page->type != PAGE_TYPE_DUMMY && page->state != PAGE_STATE_BUSY) {
			// insert marker
			marker.state = PAGE_STATE_MODIFIED;
			insert_page_after(&sModifiedPageQueue, page, &marker);
			return page;
		}
	}

	return NULL;
}


/*!	The page writer continuously takes some pages from the modified
	queue, writes them back, and moves them back to the active queue.
	It runs in its own thread, and is only there to keep the number
	of modified pages low, so that more pages can be reused with
	fewer costs.
*/
status_t
page_writer(void* /*unused*/)
{
	vm_page marker;
	marker.type = PAGE_TYPE_DUMMY;
	marker.cache = NULL;
	marker.state = PAGE_STATE_UNUSED;

	while (true) {
		if (sModifiedPageQueue.count - sModifiedTemporaryPages < 1024) {
			int32 count = 0;
			get_sem_count(sWriterWaitSem, &count);
			if (count == 0)
				count = 1;

			acquire_sem_etc(sWriterWaitSem, count, B_RELATIVE_TIMEOUT, 3000000);
				// all 3 seconds when no one triggers us
		}

		const uint32 kNumPages = 32;
		ConditionVariable<vm_page> busyConditions[kNumPages];
		vm_page *pages[kNumPages];
		uint32 numPages = 0;

		// TODO: once the I/O scheduler is there, we should write
		// a lot more pages back.
		// TODO: make this laptop friendly, too (ie. only start doing
		// something if someone else did something or there is really
		// enough to do).

		// collect pages to be written

		while (numPages < kNumPages) {
			vm_page *page = next_modified_page(marker);
			if (page == NULL)
				break;

			PageCacheLocker cacheLocker(page, false);
			if (!cacheLocker.IsLocked())
				continue;

			vm_cache *cache = page->cache;
			// TODO: write back temporary ones as soon as we have swap file support
			if (cache->temporary/* && vm_low_memory_state() == B_NO_LOW_MEMORY*/)
				continue;

			if (cache->store->ops->acquire_unreferenced_ref != NULL) {
				// we need our own reference to the store, as it might
				// currently be destructed
				if (cache->store->ops->acquire_unreferenced_ref(cache->store)
						!= B_OK) {
					cacheLocker.Unlock();
					thread_yield();
					continue;
				}
			}

			InterruptsSpinLocker locker(sPageLock);
			remove_page_from_queue(&sModifiedPageQueue, page);
			page->state = PAGE_STATE_BUSY;

			busyConditions[numPages].Publish(page, "page");

			locker.Unlock();

			//dprintf("write page %p, cache %p (%ld)\n", page, page->cache, page->cache->ref_count);
			vm_clear_map_flags(page, PAGE_MODIFIED);
			vm_cache_acquire_ref(cache);
			pages[numPages++] = page;
		}

		if (numPages == 0)
			continue;

		// write pages to disk

		// TODO: put this as requests into the I/O scheduler
		status_t writeStatus[kNumPages];
		for (uint32 i = 0; i < numPages; i++) {
			writeStatus[i] = write_page(pages[i], false);
		}

		// mark pages depending on whether they could be written or not

		for (uint32 i = 0; i < numPages; i++) {
			vm_cache *cache = pages[i]->cache;
			mutex_lock(&cache->lock);

			if (writeStatus[i] == B_OK) {
				// put it into the active queue
				InterruptsSpinLocker locker(sPageLock);
				move_page_to_active_or_inactive_queue(pages[i], true);
			} else {
				// We don't have to put the PAGE_MODIFIED bit back, as it's
				// still in the modified pages list.
				InterruptsSpinLocker locker(sPageLock);
				pages[i]->state = PAGE_STATE_MODIFIED;
				enqueue_page(&sModifiedPageQueue, pages[i]);
			}

			busyConditions[i].Unpublish();

			mutex_unlock(&cache->lock);
			if (cache->store->ops->release_ref != NULL)
				cache->store->ops->release_ref(cache->store);
			vm_cache_release_ref(cache);
		}
	}

	remove_page_marker(marker);
	return B_OK;
}


static vm_page *
find_page_candidate(struct vm_page &marker, bool stealActive)
{
	InterruptsSpinLocker locker(sPageLock);
	page_queue *queue;
	vm_page *page;

	switch (marker.state) {
		case PAGE_STATE_ACTIVE:
			queue = &sActivePageQueue;
			page = marker.queue_next;
			remove_page_from_queue(queue, &marker);
			marker.state = PAGE_STATE_UNUSED;
			break;
		case PAGE_STATE_INACTIVE:
			queue = &sInactivePageQueue;
			page = marker.queue_next;
			remove_page_from_queue(queue, &marker);
			marker.state = PAGE_STATE_UNUSED;
			break;
		default:
			queue = &sInactivePageQueue;
			page = sInactivePageQueue.head;
			if (page == NULL && stealActive) {
				queue = &sActivePageQueue;
				page = sActivePageQueue.head;
			}
			break;
	}

	while (page != NULL) {
		if (page->type != PAGE_TYPE_DUMMY
			&& (page->state == PAGE_STATE_INACTIVE
				|| (stealActive && page->state == PAGE_STATE_ACTIVE
					&& page->wired_count == 0))) {
			// insert marker
			marker.state = queue == &sActivePageQueue ? PAGE_STATE_ACTIVE : PAGE_STATE_INACTIVE;
			insert_page_after(queue, page, &marker);
			return page;
		}

		page = page->queue_next;
		if (page == NULL && stealActive && queue != &sActivePageQueue) {
			queue = &sActivePageQueue;
			page = sActivePageQueue.head;
		}
	}

	return NULL;
}


static bool
steal_page(vm_page *page, bool stealActive)
{
	// try to lock the page's cache

	class PageCacheTryLocker {
	public:
		PageCacheTryLocker(vm_page *page)
			:
			fIsLocked(false),
			fOwnsLock(false)
		{
			fCache = vm_cache_acquire_page_cache_ref(page);
			if (fCache != NULL) {
				if (fCache->lock.holder != thread_get_current_thread_id()) {
					if (mutex_trylock(&fCache->lock) != B_OK)
						return;

					fOwnsLock = true;
				}

				if (fCache == page->cache)
					fIsLocked = true;
			}
		}

		~PageCacheTryLocker()
		{
			if (fOwnsLock)
				mutex_unlock(&fCache->lock);
			if (fCache != NULL)
				vm_cache_release_ref(fCache);
		}

		bool IsLocked() { return fIsLocked; }

	private:
		vm_cache *fCache;
		bool fIsLocked;
		bool fOwnsLock;
	} cacheLocker(page);

	if (!cacheLocker.IsLocked())
		return false;

	// check again if that page is still a candidate
	if (page->state != PAGE_STATE_INACTIVE
		&& (!stealActive || page->state != PAGE_STATE_ACTIVE
			|| page->wired_count != 0))
		return false;

	// recheck eventual last minute changes
	uint32 flags;
	vm_remove_all_page_mappings(page, &flags);
	if ((flags & PAGE_MODIFIED) != 0) {
		// page was modified, don't steal it
		vm_page_set_state(page, PAGE_STATE_MODIFIED);
		return false;
	} else if ((flags & PAGE_ACCESSED) != 0) {
		// page is in active use, don't steal it
		vm_page_set_state(page, PAGE_STATE_ACTIVE);
		return false;
	}

	// we can now steal this page

	//dprintf("  steal page %p from cache %p%s\n", page, page->cache,
	//	page->state == PAGE_STATE_INACTIVE ? "" : " (ACTIVE)");

	vm_cache_remove_page(page->cache, page);
	remove_page_from_queue(page->state == PAGE_STATE_ACTIVE
		? &sActivePageQueue : &sInactivePageQueue, page);
	return true;
}


static size_t
steal_pages(vm_page **pages, size_t count, bool reserve)
{
	size_t maxCount = count;

	while (true) {
		vm_page marker;
		marker.type = PAGE_TYPE_DUMMY;
		marker.cache = NULL;
		marker.state = PAGE_STATE_UNUSED;

		bool tried = false;
		size_t stolen = 0;

		while (count > 0) {
			vm_page *page = find_page_candidate(marker, false);
			if (page == NULL)
				break;

			if (steal_page(page, false)) {
				if (reserve || stolen >= maxCount) {
					InterruptsSpinLocker _(sPageLock);
					enqueue_page(&sFreePageQueue, page);
					page->state = PAGE_STATE_FREE;

#if TRACK_PAGE_ALLOCATIONS
					add_allocation_history_entry(STOLEN_PAGE, 1, 0);
#endif
				} else if (stolen < maxCount) {
					pages[stolen] = page;
				}
				stolen++;
				count--;
			} else
				tried = true;
		}

		InterruptsSpinLocker locker(sPageLock);
		remove_page_marker(marker);

		if (reserve && sReservedPages <= free_page_queue_count()
			|| count == 0
			|| !reserve && (sInactivePageQueue.count > 0
				|| free_page_queue_count() > sReservedPages))
			return stolen;

		if (stolen && !tried && sInactivePageQueue.count > 0) {
			count++;
			continue;
		}
		if (tried) {
			// we had our go, but there are pages left, let someone else
			// try
			locker.Unlock();
			sFreePageCondition.NotifyOne();
			locker.Lock();
		}

		// we need to wait for pages to become inactive

		ConditionVariableEntry<page_queue> freeConditionEntry;
		sPageDeficit++;
		freeConditionEntry.Add(&sFreePageQueue);
		locker.Unlock();

		vm_low_memory(count);
		//snooze(50000);
			// sleep for 50ms

		freeConditionEntry.Wait();

		locker.Lock();
		sPageDeficit--;

		if (reserve && sReservedPages <= free_page_queue_count())
			return stolen;
	}
}


//	#pragma mark - private kernel API


/*!	You need to hold the vm_cache lock when calling this function.
	Note that the cache lock is released in this function.
*/
status_t
vm_page_write_modified_pages(vm_cache *cache, bool fsReenter)
{
	// ToDo: join adjacent pages into one vec list

	for (vm_page *page = cache->page_list; page; page = page->cache_next) {
		bool dequeuedPage = false;

		if (page->state == PAGE_STATE_MODIFIED) {
			InterruptsSpinLocker locker(&sPageLock);
			remove_page_from_queue(&sModifiedPageQueue, page);
			dequeuedPage = true;
		} else if (page->state == PAGE_STATE_BUSY
				|| !vm_test_map_modification(page)) {
			continue;
		}

		page->state = PAGE_STATE_BUSY;

		ConditionVariable<vm_page> busyCondition;
		busyCondition.Publish(page, "page");

		// We have a modified page - however, while we're writing it back,
		// the page is still mapped. In order not to lose any changes to the
		// page, we mark it clean before actually writing it back; if writing
		// the page fails for some reason, we just keep it in the modified page
		// list, but that should happen only rarely.

		// If the page is changed after we cleared the dirty flag, but before we
		// had the chance to write it back, then we'll write it again later -
		// that will probably not happen that often, though.

		// clear the modified flag
		vm_clear_map_flags(page, PAGE_MODIFIED);

		mutex_unlock(&cache->lock);
		status_t status = write_page(page, fsReenter);
		mutex_lock(&cache->lock);

		InterruptsSpinLocker locker(&sPageLock);

		if (status == B_OK) {
			// put it into the active/inactive queue
			move_page_to_active_or_inactive_queue(page, dequeuedPage);
		} else {
			// We don't have to put the PAGE_MODIFIED bit back, as it's still
			// in the modified pages list.
			if (dequeuedPage) {
				page->state = PAGE_STATE_MODIFIED;
				enqueue_page(&sModifiedPageQueue, page);
			} else
				set_page_state_nolock(page, PAGE_STATE_MODIFIED);
		}

		busyCondition.Unpublish();
	}

	return B_OK;
}


/*!	Schedules the page writer to write back the specified \a page.
	Note, however, that it might not do this immediately, and it can well
	take several seconds until the page is actually written out.
*/
void
vm_page_schedule_write_page(vm_page *page)
{
	ASSERT(page->state == PAGE_STATE_MODIFIED);

	vm_page_requeue(page, false);

	release_sem_etc(sWriterWaitSem, 1, B_DO_NOT_RESCHEDULE);
}


void
vm_page_init_num_pages(kernel_args *args)
{
	uint32 i;

	// calculate the size of memory by looking at the physical_memory_range array
	addr_t physicalPagesEnd = 0;
	sPhysicalPageOffset = args->physical_memory_range[0].start / B_PAGE_SIZE;

	for (i = 0; i < args->num_physical_memory_ranges; i++) {
		physicalPagesEnd = (args->physical_memory_range[i].start
			+ args->physical_memory_range[i].size) / B_PAGE_SIZE;
	}

	TRACE(("first phys page = 0x%lx, end 0x%x\n", sPhysicalPageOffset,
		physicalPagesEnd));

	sNumPages = physicalPagesEnd - sPhysicalPageOffset;
}


status_t
vm_page_init(kernel_args *args)
{
	TRACE(("vm_page_init: entry\n"));

	// map in the new free page table
	sPages = (vm_page *)vm_allocate_early(args, sNumPages * sizeof(vm_page),
		~0L, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	TRACE(("vm_init: putting free_page_table @ %p, # ents %d (size 0x%x)\n",
		sPages, sNumPages, (unsigned int)(sNumPages * sizeof(vm_page))));

	// initialize the free page table
	for (uint32 i = 0; i < sNumPages; i++) {
		sPages[i].physical_page_number = sPhysicalPageOffset + i;
		sPages[i].type = PAGE_TYPE_PHYSICAL;
		sPages[i].state = PAGE_STATE_FREE;
		new(&sPages[i].mappings) vm_page_mappings();
		sPages[i].wired_count = 0;
		sPages[i].usage_count = 0;
		sPages[i].cache = NULL;
		#ifdef DEBUG_PAGE_QUEUE
			sPages[i].queue = NULL;
		#endif
		#ifdef DEBUG_PAGE_CACHE_TRANSITIONS
			sPages[i].debug_flags = 0;
			sPages[i].collided_page = NULL;
		#endif	// DEBUG_PAGE_CACHE_TRANSITIONS
		enqueue_page(&sFreePageQueue, &sPages[i]);
	}

	TRACE(("initialized table\n"));

	// mark some of the page ranges inuse
	for (uint32 i = 0; i < args->num_physical_allocated_ranges; i++) {
		vm_mark_page_range_inuse(args->physical_allocated_range[i].start / B_PAGE_SIZE,
			args->physical_allocated_range[i].size / B_PAGE_SIZE);
	}

	TRACE(("vm_page_init: exit\n"));

	return B_OK;
}


status_t
vm_page_init_post_area(kernel_args *args)
{
	void *dummy;

	dummy = sPages;
	create_area("page structures", &dummy, B_EXACT_ADDRESS,
		PAGE_ALIGN(sNumPages * sizeof(vm_page)), B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	add_debugger_command("page_stats", &dump_page_stats, "Dump statistics about page usage");
	add_debugger_command("page", &dump_page, "Dump page info");
	add_debugger_command("page_queue", &dump_page_queue, "Dump page queue");
	add_debugger_command("find_page", &find_page,
		"Find out which queue a page is actually in");
#if TRACK_PAGE_ALLOCATIONS
	add_debugger_command("page_allocations", &dump_page_allocations,
		"Dump page allocation history");
#endif

	return B_OK;
}


status_t
vm_page_init_post_thread(kernel_args *args)
{
	new (&sFreePageCondition) ConditionVariable<page_queue>;
	sFreePageCondition.Publish(&sFreePageQueue, "free page");

	// create a kernel thread to clear out pages

	thread_id thread = spawn_kernel_thread(&page_scrubber, "page scrubber",
		B_LOWEST_ACTIVE_PRIORITY, NULL);
	send_signal_etc(thread, SIGCONT, B_DO_NOT_RESCHEDULE);

	// start page writer

	sWriterWaitSem = create_sem(0, "page writer");

	thread = spawn_kernel_thread(&page_writer, "page writer",
		B_NORMAL_PRIORITY + 1, NULL);
	send_signal_etc(thread, SIGCONT, B_DO_NOT_RESCHEDULE);

	return B_OK;
}


status_t
vm_mark_page_inuse(addr_t page)
{
	return vm_mark_page_range_inuse(page, 1);
}


status_t
vm_mark_page_range_inuse(addr_t startPage, addr_t length)
{
	TRACE(("vm_mark_page_range_inuse: start 0x%lx, len 0x%lx\n",
		startPage, length));

	if (sPhysicalPageOffset > startPage) {
		TRACE(("vm_mark_page_range_inuse: start page %ld is before free list\n",
			startPage));
		return B_BAD_VALUE;
	}
	startPage -= sPhysicalPageOffset;
	if (startPage + length > sNumPages) {
		TRACE(("vm_mark_page_range_inuse: range would extend past free list\n"));
		return B_BAD_VALUE;
	}

	cpu_status state = disable_interrupts();
	acquire_spinlock(&sPageLock);

	for (addr_t i = 0; i < length; i++) {
		vm_page *page = &sPages[startPage + i];
		switch (page->state) {
			case PAGE_STATE_FREE:
			case PAGE_STATE_CLEAR:
				set_page_state_nolock(page, PAGE_STATE_UNUSED);
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
				dprintf("vm_mark_page_range_inuse: page 0x%lx in non-free state %d!\n",
					startPage + i, page->state);
				break;
		}
	}

	release_spinlock(&sPageLock);
	restore_interrupts(state);

	return B_OK;
}


/*!	Unreserve pages previously reserved with vm_page_reserve_pages().
	Note, you specify the same \a count here that you specified when
	reserving the pages - you don't need to keep track how many pages
	you actually needed of that upfront allocation.
*/
void
vm_page_unreserve_pages(uint32 count)
{
	if (count == 0)
		return;

	InterruptsSpinLocker locker(sPageLock);
	ASSERT(sReservedPages >= count);

#if TRACK_PAGE_ALLOCATIONS
	add_allocation_history_entry(UNRESERVE_PAGES, count, 0);
#endif

	sReservedPages -= count;

	if (sPageDeficit > 0)
		sFreePageCondition.NotifyAll();
}


/*!	With this call, you can reserve a number of free pages in the system.
	They will only be handed out to someone who has actually reserved them.
	This call returns as soon as the number of requested pages has been
	reached.
*/
void
vm_page_reserve_pages(uint32 count)
{
	if (count == 0)
		return;

	InterruptsSpinLocker locker(sPageLock);

#if TRACK_PAGE_ALLOCATIONS
	add_allocation_history_entry(RESERVE_PAGES, count, 0);
#endif

	sReservedPages += count;
	size_t freePages = free_page_queue_count();
	if (sReservedPages <= freePages)
		return;

	locker.Unlock();

	steal_pages(NULL, count + 1, true);
		// we get one more, just in case we can do something someone
		// else can't
}


vm_page *
vm_page_allocate_page(int pageState, bool reserved)
{
	ConditionVariableEntry<page_queue> freeConditionEntry;
	page_queue *queue;
	page_queue *otherQueue;

	switch (pageState) {
		case PAGE_STATE_FREE:
			queue = &sFreePageQueue;
			otherQueue = &sClearPageQueue;
			break;
		case PAGE_STATE_CLEAR:
			queue = &sClearPageQueue;
			otherQueue = &sFreePageQueue;
			break;
		default:
			return NULL; // invalid
	}

	InterruptsSpinLocker locker(sPageLock);

#if TRACK_PAGE_ALLOCATIONS
	add_allocation_history_entry(ALLOCATE_PAGE, 1, reserved ? 1 : 0);
#endif

	vm_page *page = NULL;
	while (true) {
		if (reserved || sReservedPages < free_page_queue_count()) {
			page = dequeue_page(queue);
			if (page == NULL) {
#ifdef DEBUG
				if (queue->count != 0)
					panic("queue %p corrupted, count = %d\n", queue, queue->count);
#endif

				// if the primary queue was empty, grap the page from the
				// secondary queue
				page = dequeue_page(otherQueue);
			}
		}

		if (page != NULL)
			break;

		if (reserved)
			panic("Had reserved page, but there is none!");

		// steal one from the inactive list
		locker.Unlock();
		size_t stolen = steal_pages(&page, 1, false);
		locker.Lock();

		if (stolen > 0)
			break;
	}

	if (page->cache != NULL)
		panic("supposed to be free page %p has cache\n", page);

	int oldPageState = page->state;
	page->state = PAGE_STATE_BUSY;
	page->usage_count = 2;

	enqueue_page(&sActivePageQueue, page);

	locker.Unlock();

	// if needed take the page from the free queue and zero it out
	if (pageState == PAGE_STATE_CLEAR && oldPageState != PAGE_STATE_CLEAR)
		clear_page(page);

	return page;
}


/*!	Allocates a number of pages and puts their pointers into the provided
	array. All pages are marked busy.
	Returns B_OK on success, and B_NO_MEMORY when there aren't any free
	pages left to allocate.
*/
status_t
vm_page_allocate_pages(int pageState, vm_page **pages, uint32 numPages)
{
	uint32 i;
	
	for (i = 0; i < numPages; i++) {
		pages[i] = vm_page_allocate_page(pageState, false);
		if (pages[i] == NULL) {
			// allocation failed, we need to free what we already have
			while (i-- > 0)
				vm_page_set_state(pages[i], pageState);

			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


vm_page *
vm_page_allocate_page_run(int pageState, addr_t length)
{
	vm_page *firstPage = NULL;
	uint32 start = 0;

	InterruptsSpinLocker locker(sPageLock);

	if (sFreePageQueue.count + sClearPageQueue.count - sReservedPages < length) {
		// TODO: add more tries, ie. free some inactive, ...
		// no free space
		return NULL;
	}

	for (;;) {
		bool foundRun = true;
		if (start + length > sNumPages)
			break;

		uint32 i;
		for (i = 0; i < length; i++) {
			if (sPages[start + i].state != PAGE_STATE_FREE
				&& sPages[start + i].state != PAGE_STATE_CLEAR) {
				foundRun = false;
				i++;
				break;
			}
		}
		if (foundRun) {
			// pull the pages out of the appropriate queues
			for (i = 0; i < length; i++) {
				sPages[start + i].is_cleared
					= sPages[start + i].state == PAGE_STATE_CLEAR;
				set_page_state_nolock(&sPages[start + i], PAGE_STATE_BUSY);
				sPages[i].usage_count = 2;
			}
			firstPage = &sPages[start];
			break;
		} else {
			start += i;
		}
	}

#if TRACK_PAGE_ALLOCATIONS
	add_allocation_history_entry(ALLOCATE_PAGE_RUN, length, 0);
#endif

	locker.Unlock();

	if (firstPage != NULL && pageState == PAGE_STATE_CLEAR) {
		for (uint32 i = 0; i < length; i++) {
			if (!sPages[start + i].is_cleared)
	 			clear_page(&sPages[start + i]);
		}
	}

	return firstPage;
}


vm_page *
vm_page_at_index(int32 index)
{
	return &sPages[index];
}


vm_page *
vm_lookup_page(addr_t pageNumber)
{
	if (pageNumber < sPhysicalPageOffset)
		return NULL;

	pageNumber -= sPhysicalPageOffset;
	if (pageNumber >= sNumPages)
		return NULL;

	return &sPages[pageNumber];
}


/*!	Free the page that belonged to a certain cache.
	You can use vm_page_set_state() manually if you prefer, but only
	if the page does not equal PAGE_STATE_MODIFIED.
*/
void
vm_page_free(vm_cache *cache, vm_page *page)
{
	InterruptsSpinLocker _(sPageLock);

	if (page->cache == NULL && page->state == PAGE_STATE_MODIFIED
		&& cache->temporary)
		sModifiedTemporaryPages--;

	set_page_state_nolock(page, PAGE_STATE_FREE);
}


status_t
vm_page_set_state(vm_page *page, int pageState)
{
	InterruptsSpinLocker _(sPageLock);

	return set_page_state_nolock(page, pageState);
}


/*!	Moves a page to either the tail of the head of its current queue,
	depending on \a tail.
*/
void
vm_page_requeue(struct vm_page *page, bool tail)
{
	InterruptsSpinLocker _(sPageLock);
	page_queue *queue = NULL;

	switch (page->state) {
		case PAGE_STATE_BUSY:
		case PAGE_STATE_ACTIVE:
		case PAGE_STATE_WIRED:
		case PAGE_STATE_UNUSED:
			queue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			queue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			queue = &sModifiedPageQueue;
			break;
		case PAGE_STATE_FREE:
			queue = &sFreePageQueue;
			break;
		case PAGE_STATE_CLEAR:
			queue = &sClearPageQueue;
			break;
		default:
			panic("vm_page_touch: vm_page %p in invalid state %d\n",
				page, page->state);
			break;
	}

	remove_page_from_queue(queue, page);

	if (tail)
		enqueue_page(queue, page);
	else
		enqueue_page_to_head(queue, page);
}


size_t
vm_page_num_pages(void)
{
	return sNumPages;
}


size_t
vm_page_num_free_pages(void)
{
	size_t reservedPages = sReservedPages;
	size_t count = free_page_queue_count() + sInactivePageQueue.count;
	if (reservedPages > count)
		return 0;

	return count - reservedPages;
}

