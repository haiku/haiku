/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
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

#include <AutoDeleter.h>

#include <arch/cpu.h>
#include <arch/vm_translation_map.h>
#include <block_cache.h>
#include <boot/kernel_args.h>
#include <condition_variable.h>
#include <kernel.h>
#include <low_resource_manager.h>
#include <thread.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <vfs.h>
#include <vm.h>
#include <vm_address_space.h>
#include <vm_priv.h>
#include <vm_page.h>
#include <vm_cache.h>

#include "PageCacheLocker.h"
#include "io_requests.h"


//#define TRACE_VM_PAGE
#ifdef TRACE_VM_PAGE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define SCRUB_SIZE 16
	// this many pages will be cleared at once in the page scrubber thread

#define MAX_PAGE_WRITER_IO_PRIORITY				B_URGENT_DISPLAY_PRIORITY
	// maximum I/O priority of the page writer
#define MAX_PAGE_WRITER_IO_PRIORITY_THRESHOLD	10000
	// the maximum I/O priority shall be reached when this many pages need to
	// be written


typedef struct page_queue {
	vm_page *head;
	vm_page *tail;
	uint32	count;
} page_queue;

int32 gMappedPagesCount;

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

static ConditionVariable sFreePageCondition;
static spinlock sPageLock;

static sem_id sWriterWaitSem;


#if PAGE_ALLOCATION_TRACING

namespace PageAllocationTracing {

class ReservePages : public AbstractTraceEntry {
	public:
		ReservePages(uint32 count)
			:
			fCount(count)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page reserve:   %lu", fCount);
		}

	private:
		uint32		fCount;
};


class UnreservePages : public AbstractTraceEntry {
	public:
		UnreservePages(uint32 count)
			:
			fCount(count)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page unreserve: %lu", fCount);
		}

	private:
		uint32		fCount;
};


class AllocatePage : public AbstractTraceEntry {
	public:
		AllocatePage(bool reserved)
			:
			fReserved(reserved)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page alloc");
			if (fReserved)
				out.Print(" reserved");
		}

	private:
		bool		fReserved;
};


class AllocatePageRun : public AbstractTraceEntry {
	public:
		AllocatePageRun(uint32 length)
			:
			fLength(length)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page alloc run: length: %ld", fLength);
		}

	private:
		uint32		fLength;
};


class FreePage : public AbstractTraceEntry {
	public:
		FreePage()
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page free");
		}
};


class ScrubbingPages : public AbstractTraceEntry {
	public:
		ScrubbingPages(uint32 count)
			:
			fCount(count)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page scrubbing: %lu", fCount);
		}

	private:
		uint32		fCount;
};


class ScrubbedPages : public AbstractTraceEntry {
	public:
		ScrubbedPages(uint32 count)
			:
			fCount(count)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page scrubbed:  %lu", fCount);
		}

	private:
		uint32		fCount;
};


class StolenPage : public AbstractTraceEntry {
	public:
		StolenPage()
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page stolen");
		}
};

}	// namespace PageAllocationTracing

#	define T(x)	new(std::nothrow) PageAllocationTracing::x

#else
#	define T(x)
#endif	// PAGE_ALLOCATION_TRACING


#if PAGE_WRITER_TRACING

namespace PageWriterTracing {

class WritePage : public AbstractTraceEntry {
	public:
		WritePage(vm_page* page)
			:
			fCache(page->cache),
			fPage(page)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page write: %p, cache: %p", fPage, fCache);
		}

	private:
		VMCache*	fCache;
		vm_page*	fPage;
};

}	// namespace PageWriterTracing

#	define TPW(x)	new(std::nothrow) PageWriterTracing::x

#else
#	define TPW(x)
#endif	// PAGE_WRITER_TRACING


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
		if (page->type != PAGE_TYPE_DUMMY)
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
	if (page->type != PAGE_TYPE_DUMMY)
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
	if (page->type != PAGE_TYPE_DUMMY)
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

	if (page->type != PAGE_TYPE_DUMMY)
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

	if (page->type != PAGE_TYPE_DUMMY)
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
	kprintf("physical_number: %lx\n", page->physical_page_number);
	kprintf("cache:           %p\n", page->cache);
	kprintf("cache_offset:    %ld\n", page->cache_offset);
	kprintf("cache_next:      %p\n", page->cache_next);
	kprintf("type:            %d\n", page->type);
	kprintf("state:           %s\n", page_state_to_string(page->state));
	kprintf("wired_count:     %d\n", page->wired_count);
	kprintf("usage_count:     %d\n", page->usage_count);
	kprintf("busy_writing:    %d\n", page->busy_writing);
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
	page_num_t swappableModified = 0;
	page_num_t swappableModifiedInactive = 0;
	uint32 counter[8];
	addr_t i;

	memset(counter, 0, sizeof(counter));

	for (i = 0; i < sNumPages; i++) {
		if (sPages[i].state > 7)
			panic("page %li at %p has invalid state!\n", i, &sPages[i]);

		counter[sPages[i].state]++;

		if (sPages[i].state == PAGE_STATE_MODIFIED && sPages[i].cache != NULL
			&& sPages[i].cache->temporary && sPages[i].wired_count == 0) {
			swappableModified++;
			if (sPages[i].usage_count < 0)
				swappableModifiedInactive++;
		}
	}

	kprintf("page stats:\n");
	kprintf("total: %lu\n", sNumPages);
	kprintf("active: %lu\ninactive: %lu\nbusy: %lu\nunused: %lu\n",
		counter[PAGE_STATE_ACTIVE], counter[PAGE_STATE_INACTIVE],
		counter[PAGE_STATE_BUSY], counter[PAGE_STATE_UNUSED]);
	kprintf("wired: %lu\nmodified: %lu\nfree: %lu\nclear: %lu\n",
		counter[PAGE_STATE_WIRED], counter[PAGE_STATE_MODIFIED],
		counter[PAGE_STATE_FREE], counter[PAGE_STATE_CLEAR]);
	kprintf("reserved pages: %lu\n", sReservedPages);
	kprintf("page deficit: %lu\n", sPageDeficit);
	kprintf("mapped pages: %lu\n", gMappedPagesCount);

	kprintf("\nfree queue: %p, count = %ld\n", &sFreePageQueue,
		sFreePageQueue.count);
	kprintf("clear queue: %p, count = %ld\n", &sClearPageQueue,
		sClearPageQueue.count);
	kprintf("modified queue: %p, count = %ld (%ld temporary, %lu swappable, "
		"inactive: %lu)\n", &sModifiedPageQueue, sModifiedPageQueue.count,
		sModifiedTemporaryPages, swappableModified, swappableModifiedInactive);
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
	if (page->cache != NULL && page->cache->temporary) {
		if (pageState == PAGE_STATE_MODIFIED)
			sModifiedTemporaryPages++;
		else if (page->state == PAGE_STATE_MODIFIED)
			sModifiedTemporaryPages--;
	}

#ifdef PAGE_ALLOCATION_TRACING
	if ((pageState == PAGE_STATE_CLEAR || pageState == PAGE_STATE_FREE)
		&& page->state != PAGE_STATE_CLEAR && page->state != PAGE_STATE_FREE) {
		T(FreePage());
	}
#endif	// PAGE_ALLOCATION_TRACING

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
		&virtualAddress, 0);

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
			vm_page *page[SCRUB_SIZE];
			int32 i, scrubCount;

			// get some pages from the free queue

			InterruptsSpinLocker locker(sPageLock);

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

			if (scrubCount > 0) {
				T(ScrubbingPages(scrubCount));
			}

			locker.Unlock();

			// clear them

			for (i = 0; i < scrubCount; i++) {
				clear_page(page[i]);
			}

			locker.Lock();

			// and put them into the clear queue

			for (i = 0; i < scrubCount; i++) {
				page[i]->state = PAGE_STATE_CLEAR;
				enqueue_page(&sClearPageQueue, page[i]);
			}

			if (scrubCount > 0) {
				T(ScrubbedPages(scrubCount));
			}
		}
	}

	return 0;
}


static status_t
write_page(vm_page *page, uint32 flags, AsyncIOCallback* callback)
{
	TRACE(("write_page(page = %p): offset = %Ld\n", page, (off_t)page->cache_offset << PAGE_SHIFT));

	off_t offset = (off_t)page->cache_offset << PAGE_SHIFT;

	iovec vecs[1];
	vecs->iov_base = (void*)(addr_t)(page->physical_page_number * B_PAGE_SIZE);
	vecs->iov_len = B_PAGE_SIZE;

	if (callback != NULL) {
		// asynchronous I/O
		return page->cache->WriteAsync(offset, vecs, 1, B_PAGE_SIZE,
			flags | B_PHYSICAL_IO_REQUEST, callback);
	}

	// synchronous I/0
	size_t length = B_PAGE_SIZE;
	status_t status = page->cache->Write(offset, vecs, 1,
		flags | B_PHYSICAL_IO_REQUEST, &length);

	if (status == B_OK && length == 0)
		status = B_ERROR;

	return status;
}


static inline bool
is_marker_page(struct vm_page *page)
{
	return page->type == PAGE_TYPE_DUMMY;
}


static void
remove_page_marker(struct vm_page &marker)
{
	if (marker.state == PAGE_STATE_UNUSED)
		return;

	page_queue *queue;

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

	InterruptsSpinLocker locker(sPageLock);
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
		if (!is_marker_page(page) && page->state != PAGE_STATE_BUSY) {
			// insert marker
			marker.state = PAGE_STATE_MODIFIED;
			insert_page_after(&sModifiedPageQueue, page, &marker);
			return page;
		}
	}

	return NULL;
}


class PageWriterCallback;


class PageWriterRun {
public:
	status_t Init(uint32 maxPages);

	void PrepareNextRun();
	void AddPage(vm_page* page);
	void Go();

	void PageWritten(PageWriterCallback* callback, status_t status,
		bool partialTransfer, size_t bytesTransferred);

private:
	uint32				fMaxPages;
	uint32				fPageCount;
	vint32				fPendingPages;
	PageWriterCallback*	fCallbacks;
	ConditionVariable	fAllFinishedCondition;
};


class PageWriterCallback : public AsyncIOCallback {
public:
	void SetTo(PageWriterRun* run, vm_page* page)
	{
		fRun = run;
		fPage = page;
		fCache = page->cache;
		fStatus = B_OK;
		fBusyCondition.Publish(page, "page");
	}

	vm_page* Page() const	{ return fPage; }
	VMCache* Cache() const	{ return fCache; }
	status_t Status() const	{ return fStatus; }

	ConditionVariable& BusyCondition()	{ return fBusyCondition; }

	virtual void IOFinished(status_t status, bool partialTransfer,
		size_t bytesTransferred)
	{
		fStatus = status == B_OK && bytesTransferred == 0 ? B_ERROR : status;
		fRun->PageWritten(this, status, partialTransfer, bytesTransferred);
	}

private:
	PageWriterRun*		fRun;
	vm_page*			fPage;
	VMCache*			fCache;
	status_t			fStatus;
	ConditionVariable	fBusyCondition;
};


status_t
PageWriterRun::Init(uint32 maxPages)
{
	fMaxPages = maxPages;
	fPageCount = 0;
	fPendingPages = 0;

	fCallbacks = new(std::nothrow) PageWriterCallback[maxPages];
	if (fCallbacks == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
PageWriterRun::PrepareNextRun()
{
	fPageCount = 0;
	fPendingPages = 0;
}


void
PageWriterRun::AddPage(vm_page* page)
{
	page->state = PAGE_STATE_BUSY;
	page->busy_writing = true;

	fCallbacks[fPageCount].SetTo(this, page);
	fPageCount++;
}


void
PageWriterRun::Go()
{
	fPendingPages = fPageCount;

	fAllFinishedCondition.Init(this, "page writer wait for I/O");
	ConditionVariableEntry waitEntry;
	fAllFinishedCondition.Add(&waitEntry);

	// schedule writes
	for (uint32 i = 0; i < fPageCount; i++) {
		PageWriterCallback& callback = fCallbacks[i];
		write_page(callback.Page(), B_VIP_IO_REQUEST, &callback);
	}

	// wait until all pages have been written
	waitEntry.Wait();

	// mark pages depending on whether they could be written or not

	for (uint32 i = 0; i < fPageCount; i++) {
		PageWriterCallback& callback = fCallbacks[i];
		vm_page* page = callback.Page();
		vm_cache* cache = callback.Cache();
		cache->Lock();

		if (callback.Status() == B_OK) {
			// put it into the active queue
			InterruptsSpinLocker locker(sPageLock);
			move_page_to_active_or_inactive_queue(page, true);
			page->busy_writing = false;
		} else {
			// We don't have to put the PAGE_MODIFIED bit back, as it's
			// still in the modified pages list.
			{
				InterruptsSpinLocker locker(sPageLock);
				page->state = PAGE_STATE_MODIFIED;
				enqueue_page(&sModifiedPageQueue, page);
			}
			if (!page->busy_writing) {
				// someone has cleared the busy_writing flag which tells
				// us our page has gone invalid
				cache->RemovePage(page);
			} else
				page->busy_writing = false;
		}

		callback.BusyCondition().Unpublish();

		cache->Unlock();
	}

	for (uint32 i = 0; i < fPageCount; i++) {
		vm_cache* cache = fCallbacks[i].Cache();

		// We release the cache references after all pages were made
		// unbusy again - otherwise releasing a vnode could deadlock.
		cache->ReleaseStoreRef();
		cache->ReleaseRef();
	}
}


void
PageWriterRun::PageWritten(PageWriterCallback* callback, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	if (atomic_add(&fPendingPages, -1) == 1)
		fAllFinishedCondition.NotifyAll();
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
	const uint32 kNumPages = 256;
	uint32 writtenPages = 0;
	bigtime_t lastWrittenTime = 0;
	bigtime_t pageCollectionTime = 0;
	bigtime_t pageWritingTime = 0;

	PageWriterRun run;
	if (run.Init(kNumPages) != B_OK) {
		panic("page writer: Failed to init PageWriterRun!");
		return B_ERROR;
	}

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

		// depending on how urgent it becomes to get pages to disk, we adjust
		// our I/O priority
		page_num_t modifiedPages = sModifiedPageQueue.count
			- sModifiedTemporaryPages;
		uint32 lowPagesState = low_resource_state(B_KERNEL_RESOURCE_PAGES);
		int32 ioPriority = B_IDLE_PRIORITY;
		if (lowPagesState >= B_LOW_RESOURCE_CRITICAL
			|| modifiedPages > MAX_PAGE_WRITER_IO_PRIORITY_THRESHOLD) {
			ioPriority = MAX_PAGE_WRITER_IO_PRIORITY;
		} else {
			ioPriority = (uint64)MAX_PAGE_WRITER_IO_PRIORITY * modifiedPages
				/ MAX_PAGE_WRITER_IO_PRIORITY_THRESHOLD;
		}

		thread_set_io_priority(ioPriority);

		uint32 numPages = 0;
		run.PrepareNextRun();

		// TODO: make this laptop friendly, too (ie. only start doing
		// something if someone else did something or there is really
		// enough to do).

		// collect pages to be written
#if ENABLE_SWAP_SUPPORT
		bool lowOnPages = lowPagesState != B_NO_LOW_RESOURCE;
#endif

		pageCollectionTime -= system_time();

		while (numPages < kNumPages) {
			vm_page *page = next_modified_page(marker);
			if (page == NULL)
				break;

			PageCacheLocker cacheLocker(page, false);
			if (!cacheLocker.IsLocked())
				continue;

			vm_cache *cache = page->cache;

			// Don't write back wired (locked) pages and don't write RAM pages
			// until we're low on pages. Also avoid writing temporary pages that
			// are active.
			if (page->wired_count > 0
				|| cache->temporary
#if ENABLE_SWAP_SUPPORT
					&& (!lowOnPages /*|| page->usage_count > 0*/)
#endif
				) {
				continue;
			}

			// we need our own reference to the store, as it might
			// currently be destructed
			if (cache->AcquireUnreferencedStoreRef() != B_OK) {
				cacheLocker.Unlock();
				thread_yield(true);
				continue;
			}

			InterruptsSpinLocker locker(sPageLock);

			// state might have change while we were locking the cache
			if (page->state != PAGE_STATE_MODIFIED) {
				// release the cache reference
				locker.Unlock();
				cache->ReleaseStoreRef();
				continue;
			}

			remove_page_from_queue(&sModifiedPageQueue, page);

			run.AddPage(page);

			locker.Unlock();

			//dprintf("write page %p, cache %p (%ld)\n", page, page->cache, page->cache->ref_count);
			TPW(WritePage(page));

			vm_clear_map_flags(page, PAGE_MODIFIED);
			cache->AcquireRefLocked();
			numPages++;
		}

		pageCollectionTime += system_time();

		if (numPages == 0)
			continue;

		// write pages to disk and do all the cleanup
		pageWritingTime -= system_time();
		run.Go();
		pageWritingTime += system_time();

		// debug output only...
		writtenPages += numPages;
		if (writtenPages >= 1024) {
			bigtime_t now = system_time();
			TRACE(("page writer: wrote 1024 pages (total: %llu ms, "
				"collect: %llu ms, write: %llu ms)\n",
				(now - lastWrittenTime) / 1000,
				pageCollectionTime / 1000, pageWritingTime / 1000));
			writtenPages -= 1024;
			lastWrittenTime = now;
			pageCollectionTime = 0;
			pageWritingTime = 0;
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

	if (marker.state == PAGE_STATE_UNUSED) {
		// Get the first free pages of the (in)active queue
		queue = &sInactivePageQueue;
		page = sInactivePageQueue.head;
		if (page == NULL && stealActive) {
			queue = &sActivePageQueue;
			page = sActivePageQueue.head;
		}
	} else {
		// Get the next page of the current queue
		if (marker.state == PAGE_STATE_INACTIVE)
			queue = &sInactivePageQueue;
		else if (marker.state == PAGE_STATE_ACTIVE)
			queue = &sActivePageQueue;
		else {
			panic("invalid marker %p state", &marker);
			queue = NULL;
		}

		page = marker.queue_next;
		remove_page_from_queue(queue, &marker);
		marker.state = PAGE_STATE_UNUSED;
	}

	while (page != NULL) {
		if (!is_marker_page(page)
			&& (page->state == PAGE_STATE_INACTIVE
				|| (stealActive && page->state == PAGE_STATE_ACTIVE
					&& page->wired_count == 0))) {
			// we found a candidate, insert marker
			marker.state = queue == &sActivePageQueue
				? PAGE_STATE_ACTIVE : PAGE_STATE_INACTIVE;
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
	if (vm_cache_acquire_locked_page_cache(page, false) == NULL)
		return false;

	AutoLocker<VMCache> cacheLocker(page->cache, true, false);
	MethodDeleter<VMCache> _2(page->cache, &VMCache::ReleaseRefLocked);

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

	page->cache->RemovePage(page);

	InterruptsSpinLocker _(sPageLock);
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

					T(StolenPage());
				} else if (stolen < maxCount) {
					pages[stolen] = page;
				}
				stolen++;
				count--;
			} else
				tried = true;
		}

		remove_page_marker(marker);

		InterruptsSpinLocker locker(sPageLock);

		if (reserve && sReservedPages <= free_page_queue_count()
			|| count == 0
			|| !reserve && (sInactivePageQueue.count > 0
				|| free_page_queue_count() > sReservedPages))
			return stolen;

		if (stolen && !tried && sInactivePageQueue.count > 0) {
			count++;
			continue;
		}

		// we need to wait for pages to become inactive

		ConditionVariableEntry freeConditionEntry;
		sPageDeficit++;
		freeConditionEntry.Add(&sFreePageQueue);
		locker.Unlock();

		if (tried) {
			// We tried all potential pages, but one or more couldn't be stolen
			// at that time (likely because their cache was locked). No one
			// else will have any better luck, so we'll just retry a little
			// later.
			freeConditionEntry.Wait(B_RELATIVE_TIMEOUT, 10000);
		} else {
			low_resource(B_KERNEL_RESOURCE_PAGES, count, B_RELATIVE_TIMEOUT, 0);
			//snooze(50000);
				// sleep for 50ms

			freeConditionEntry.Wait();
		}

		locker.Lock();
		sPageDeficit--;

		if (reserve && sReservedPages <= free_page_queue_count())
			return stolen;
	}
}


//	#pragma mark - private kernel API


/*!	Writes a range of modified pages of a cache to disk.
	You need to hold the vm_cache lock when calling this function.
	Note that the cache lock is released in this function.
	\param cache The cache.
	\param firstPage Offset (in page size units) of the first page in the range.
	\param endPage End offset (in page size units) of the page range. The page
		at this offset is not included.
*/
status_t
vm_page_write_modified_page_range(struct VMCache *cache, uint32 firstPage,
	uint32 endPage)
{
	// TODO: join adjacent pages into one vec list

	for (VMCachePagesTree::Iterator it
				= cache->pages.GetIterator(firstPage, true, true);
			vm_page *page = it.Next();) {
		bool dequeuedPage = false;

		if (page->cache_offset >= endPage)
			break;

		if (page->state == PAGE_STATE_MODIFIED) {
			InterruptsSpinLocker locker(&sPageLock);
			remove_page_from_queue(&sModifiedPageQueue, page);
			dequeuedPage = true;
		} else if (page->state == PAGE_STATE_BUSY
				|| !vm_test_map_modification(page)) {
			continue;
		}

		page->state = PAGE_STATE_BUSY;
		page->busy_writing = true;

		ConditionVariable busyCondition;
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

		cache->Unlock();
		status_t status = write_page(page, 0, NULL);
		cache->Lock();

		InterruptsSpinLocker locker(&sPageLock);

		if (status == B_OK) {
			// put it into the active/inactive queue
			move_page_to_active_or_inactive_queue(page, dequeuedPage);
			page->busy_writing = false;
		} else {
			// We don't have to put the PAGE_MODIFIED bit back, as it's still
			// in the modified pages list.
			if (dequeuedPage) {
				page->state = PAGE_STATE_MODIFIED;
				enqueue_page(&sModifiedPageQueue, page);
			}

			if (!page->busy_writing) {
				// someone has cleared the busy_writing flag which tells
				// us our page has gone invalid
				cache->RemovePage(page);
			} else {
				if (!dequeuedPage)
					set_page_state_nolock(page, PAGE_STATE_MODIFIED);

				page->busy_writing = false;
			}
		}

		busyCondition.Unpublish();
	}

	return B_OK;
}


/*!	You need to hold the vm_cache lock when calling this function.
	Note that the cache lock is released in this function.
*/
status_t
vm_page_write_modified_pages(vm_cache *cache)
{
	return vm_page_write_modified_page_range(cache, 0,
		(cache->virtual_end + B_PAGE_SIZE - 1) >> PAGE_SHIFT);
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


/*!	Cache must be locked.
*/
void
vm_page_schedule_write_page_range(struct VMCache *cache, uint32 firstPage,
	uint32 endPage)
{
	uint32 modified = 0;
	for (VMCachePagesTree::Iterator it
				= cache->pages.GetIterator(firstPage, true, true);
			vm_page *page = it.Next();) {
		if (page->cache_offset >= endPage)
			break;

		if (page->state == PAGE_STATE_MODIFIED) {
			vm_page_requeue(page, false);
			modified++;
		}
	}

	if (modified > 0)
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

	TRACE(("first phys page = 0x%lx, end 0x%lx\n", sPhysicalPageOffset,
		physicalPagesEnd));

	sNumPages = physicalPagesEnd - sPhysicalPageOffset;

#ifdef LIMIT_AVAILABLE_MEMORY
	if (sNumPages > LIMIT_AVAILABLE_MEMORY * 256)
		sNumPages = LIMIT_AVAILABLE_MEMORY * 256;
#endif
}


status_t
vm_page_init(kernel_args *args)
{
	TRACE(("vm_page_init: entry\n"));

	// map in the new free page table
	sPages = (vm_page *)vm_allocate_early(args, sNumPages * sizeof(vm_page),
		~0L, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	TRACE(("vm_init: putting free_page_table @ %p, # ents %ld (size 0x%x)\n",
		sPages, sNumPages, (unsigned int)(sNumPages * sizeof(vm_page))));

	// initialize the free page table
	for (uint32 i = 0; i < sNumPages; i++) {
		sPages[i].physical_page_number = sPhysicalPageOffset + i;
		sPages[i].type = PAGE_TYPE_PHYSICAL;
		sPages[i].state = PAGE_STATE_FREE;
		new(&sPages[i].mappings) vm_page_mappings();
		sPages[i].wired_count = 0;
		sPages[i].usage_count = 0;
		sPages[i].busy_writing = false;
		sPages[i].merge_swap = false;
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

	return B_OK;
}


status_t
vm_page_init_post_thread(kernel_args *args)
{
	new (&sFreePageCondition) ConditionVariable;
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

	T(UnreservePages(count));

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

	T(ReservePages(count));

	sReservedPages += count;
	size_t freePages = free_page_queue_count();
	if (sReservedPages <= freePages)
		return;

	locker.Unlock();

	steal_pages(NULL, count + 1, true);
		// we get one more, just in case we can do something someone
		// else can't
}


bool
vm_page_try_reserve_pages(uint32 count)
{
	if (count == 0)
		return true;

	InterruptsSpinLocker locker(sPageLock);

	T(ReservePages(count));

	size_t freePages = free_page_queue_count();
	if (sReservedPages + count > freePages)
		return false;

	sReservedPages += count;
	return true;
}


vm_page *
vm_page_allocate_page(int pageState, bool reserved)
{
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

	T(AllocatePage(reserved));

	vm_page *page = NULL;
	while (true) {
		if (reserved || sReservedPages < free_page_queue_count()) {
			page = dequeue_page(queue);
			if (page == NULL) {
#ifdef DEBUG_PAGE_QUEUE
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
vm_page_allocate_page_run(int pageState, addr_t base, addr_t length)
{
	vm_page *firstPage = NULL;
	uint32 start = base >> PAGE_SHIFT;

	InterruptsSpinLocker locker(sPageLock);

	if (sFreePageQueue.count + sClearPageQueue.count - sReservedPages
			< length) {
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
				sPages[start + i].usage_count = 2;
			}
			firstPage = &sPages[start];
			break;
		} else {
			start += i;
		}
	}

	T(AllocatePageRun(length));

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
		&& cache->temporary) {
		sModifiedTemporaryPages--;
	}

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


/*! There is a subtle distinction between the page counts returned by
	this function and vm_page_num_free_pages():
	The latter returns the number of pages that are completely uncommitted,
	whereas this one returns the number of pages that are available for
	use by being reclaimed as well (IOW it factors in things like cache pages
	as available).
*/
size_t
vm_page_num_available_pages(void)
{
	return vm_available_memory() / B_PAGE_SIZE;
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


void
vm_page_get_stats(system_info *info)
{
	// Get free pages count -- not really exact, since we don't know how many
	// of the reserved pages have already been allocated, but good citizens
	// unreserve chunk-wise as they are allocating the pages, if they have
	// reserved a larger quantity.
	page_num_t reserved = sReservedPages;
	page_num_t free = free_page_queue_count();
	free = free > reserved ? free - reserved : 0;

	// The pages used for the block cache buffers. Those should not be counted
	// as used but as cached pages.
	// TODO: We should subtract the blocks that are in use ATM, since those
	// can't really be freed in a low memory situation.
	page_num_t blockCachePages = block_cache_used_memory() / B_PAGE_SIZE;

	info->max_pages = sNumPages;
	info->used_pages = gMappedPagesCount - blockCachePages;
	info->cached_pages = sNumPages >= free + info->used_pages
		? sNumPages - free - info->used_pages : 0;
	info->page_faults = vm_num_page_faults();

	// TODO: We don't consider pages used for page directories/tables yet.
}
