/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
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
#include <heap.h>
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

#include "VMAnonymousCache.h"
#include "IORequest.h"
#include "PageCacheLocker.h"


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
static mutex sPageLock = MUTEX_INITIALIZER("pages");

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

#if DEBUG_PAGE_QUEUE
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
#if DEBUG_PAGE_QUEUE
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

#if DEBUG_PAGE_QUEUE
	page->queue = queue;
#endif
}


/*!	Enqueues a page to the head of the given queue */
static void
enqueue_page_to_head(page_queue *queue, vm_page *page)
{
#if DEBUG_PAGE_QUEUE
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

#if DEBUG_PAGE_QUEUE
	page->queue = queue;
#endif
}


static void
remove_page_from_queue(page_queue *queue, vm_page *page)
{
#if DEBUG_PAGE_QUEUE
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

#if DEBUG_PAGE_QUEUE
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
#if DEBUG_PAGE_QUEUE
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

#if DEBUG_PAGE_QUEUE
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
			VMAddressSpace *addressSpace = VMAddressSpace::Kernel();
			uint32 flags;

			if (thread_get_current_thread()->team->address_space != NULL)
				addressSpace = thread_get_current_thread()->team->address_space;

			addressSpace->TranslationMap().ops->query_interrupt(
				&addressSpace->TranslationMap(), address, &address, &flags);
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
	#if DEBUG_PAGE_QUEUE
		kprintf("queue:           %p\n", page->queue);
	#endif
	#if DEBUG_PAGE_CACHE_TRANSITIONS
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

		if (pageState != PAGE_STATE_INACTIVE) {
			if (page->cache != NULL)
				panic("to be freed page %p has cache", page);
			if (!page->mappings.IsEmpty() || page->wired_count > 0)
				panic("to be freed page %p has mappings", page);
		}
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
	vm_memset_physical(page->physical_page_number << PAGE_SHIFT, 0,
		B_PAGE_SIZE);
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

		if (sFreePageQueue.count == 0)
			continue;

		MutexLocker locker(sPageLock);

		// Since we temporarily remove pages from the free pages reserve,
		// we must make sure we don't cause a violation of the page
		// reservation warranty. The following is usually stricter than
		// necessary, because we don't have information on how many of the
		// reserved pages have already been allocated.
		int32 scrubCount = SCRUB_SIZE;
		uint32 freeCount = free_page_queue_count();
		if (freeCount <= sReservedPages)
			continue;

		if ((uint32)scrubCount > freeCount - sReservedPages)
			scrubCount = freeCount - sReservedPages;

		// get some pages from the free queue
		vm_page *page[SCRUB_SIZE];
		for (int32 i = 0; i < scrubCount; i++) {
			page[i] = dequeue_page(&sFreePageQueue);
			if (page[i] == NULL) {
				scrubCount = i;
				break;
			}

			page[i]->state = PAGE_STATE_BUSY;
		}

		if (scrubCount == 0)
			continue;

		T(ScrubbingPages(scrubCount));
		locker.Unlock();

		// clear them
		for (int32 i = 0; i < scrubCount; i++)
			clear_page(page[i]);

		locker.Lock();

		// and put them into the clear queue
		for (int32 i = 0; i < scrubCount; i++) {
			page[i]->state = PAGE_STATE_CLEAR;
			enqueue_page(&sClearPageQueue, page[i]);
		}

		T(ScrubbedPages(scrubCount));
	}

	return 0;
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

	MutexLocker locker(sPageLock);
	remove_page_from_queue(queue, &marker);

	marker.state = PAGE_STATE_UNUSED;
}


static vm_page *
next_modified_page(struct vm_page &marker)
{
	MutexLocker locker(sPageLock);
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


// #pragma mark -


class PageWriteTransfer;
class PageWriteWrapper;


class PageWriterRun {
public:
	status_t Init(uint32 maxPages);

	void PrepareNextRun();
	void AddPage(vm_page* page);
	void Go();

	void PageWritten(PageWriteTransfer* transfer, status_t status,
		bool partialTransfer, size_t bytesTransferred);

private:
	uint32				fMaxPages;
	uint32				fWrapperCount;
	uint32				fTransferCount;
	vint32				fPendingTransfers;
	PageWriteWrapper*	fWrappers;
	PageWriteTransfer*	fTransfers;
	ConditionVariable	fAllFinishedCondition;
};


class PageWriteTransfer : public AsyncIOCallback {
public:
	void SetTo(PageWriterRun* run, vm_page* page, int32 maxPages);
	bool AddPage(vm_page* page);

	status_t Schedule(uint32 flags);

	void SetStatus(status_t status, size_t transferred);

	status_t Status() const	{ return fStatus; }
	struct VMCache* Cache() const { return fCache; }
	uint32 PageCount() const { return fPageCount; }

	virtual void IOFinished(status_t status, bool partialTransfer,
		size_t bytesTransferred);
private:
	PageWriterRun*		fRun;
	struct VMCache*		fCache;
	off_t				fOffset;
	uint32				fPageCount;
	int32				fMaxPages;
	status_t			fStatus;
	uint32				fVecCount;
	iovec				fVecs[32]; // TODO: make dynamic/configurable
};


class PageWriteWrapper {
public:
	PageWriteWrapper();
	~PageWriteWrapper();
	void SetTo(vm_page* page, bool dequeuedPage);
	void ClearModifiedFlag();
	void CheckRemoveFromShrunkenCache();
	void Done(status_t result);

private:
	vm_page*			fPage;
	struct VMCache*		fCache;
	bool				fDequeuedPage;
	bool				fIsActive;
	int					fOldPageState;
	ConditionVariable	fBusyCondition;
};


PageWriteWrapper::PageWriteWrapper()
	:
	fIsActive(false)
{
}


PageWriteWrapper::~PageWriteWrapper()
{
	if (fIsActive)
		panic("page write wrapper going out of scope but isn't completed");
}


void
PageWriteWrapper::SetTo(vm_page* page, bool dequeuedPage)
{
	if (page->state == PAGE_STATE_BUSY)
		panic("setting page write wrapper to busy page");

	if (fIsActive)
		panic("re-setting page write wrapper that isn't completed");

	fPage = page;
	fCache = page->cache;
	fDequeuedPage = dequeuedPage;
	fIsActive = true;

	fOldPageState = fPage->state;
	fPage->state = PAGE_STATE_BUSY;
	fPage->busy_writing = true;

	fBusyCondition.Publish(fPage, "page");
}


void
PageWriteWrapper::ClearModifiedFlag()
{
	// We have a modified page - however, while we're writing it back,
	// the page is still mapped. In order not to lose any changes to the
	// page, we mark it clean before actually writing it back; if
	// writing the page fails for some reason, we just keep it in the
	// modified page list, but that should happen only rarely.

	// If the page is changed after we cleared the dirty flag, but
	// before we had the chance to write it back, then we'll write it
	// again later - that will probably not happen that often, though.

	vm_clear_map_flags(fPage, PAGE_MODIFIED);
}


void
PageWriteWrapper::CheckRemoveFromShrunkenCache()
{
	if (fPage->busy_writing)
		return;

	vm_remove_all_page_mappings(fPage, NULL);
	fCache->RemovePage(fPage);
}


void
PageWriteWrapper::Done(status_t result)
{
	if (!fIsActive)
		panic("completing page write wrapper that is not active");

	if (result == B_OK) {
		// put it into the active/inactive queue
		move_page_to_active_or_inactive_queue(fPage, fDequeuedPage);
		fPage->busy_writing = false;
	} else {
		// Writing the page failed -- move to the modified queue. If we dequeued
		// it from there, just enqueue it again, otherwise set the page state
		// explicitly, which will take care of moving between the queues.
		if (fDequeuedPage) {
			fPage->state = PAGE_STATE_MODIFIED;
			enqueue_page(&sModifiedPageQueue, fPage);
		} else {
			fPage->state = fOldPageState;
			set_page_state_nolock(fPage, PAGE_STATE_MODIFIED);
		}

		if (!fPage->busy_writing) {
			// The busy_writing flag was cleared. That means the cache has been
			// shrunk while we were trying to write the page and we have to free
			// it now.

			// Adjust temporary modified pages count, if necessary.
			if (fDequeuedPage && fCache->temporary)
				sModifiedTemporaryPages--;

			// free the page
			set_page_state_nolock(fPage, PAGE_STATE_FREE);
		} else
			fPage->busy_writing = false;
	}

	fBusyCondition.Unpublish();
	fIsActive = false;
}


void
PageWriteTransfer::SetTo(PageWriterRun* run, vm_page* page, int32 maxPages)
{
	fRun = run;
	fCache = page->cache;
	fOffset = page->cache_offset;
	fPageCount = 1;
	fMaxPages = maxPages;
	fStatus = B_OK;

	fVecs[0].iov_base = (void*)(page->physical_page_number << PAGE_SHIFT);
	fVecs[0].iov_len = B_PAGE_SIZE;
	fVecCount = 1;
}


bool
PageWriteTransfer::AddPage(vm_page* page)
{
	if (page->cache != fCache
		|| (fMaxPages >= 0 && fPageCount >= (uint32)fMaxPages))
		return false;

	addr_t nextBase
		= (addr_t)fVecs[fVecCount - 1].iov_base + fVecs[fVecCount - 1].iov_len;

	if (page->physical_page_number << PAGE_SHIFT == nextBase
		&& page->cache_offset == fOffset + fPageCount) {
		// append to last iovec
		fVecs[fVecCount - 1].iov_len += B_PAGE_SIZE;
		fPageCount++;
		return true;
	}

	nextBase = (addr_t)fVecs[0].iov_base - B_PAGE_SIZE;
	if (page->physical_page_number << PAGE_SHIFT == nextBase
		&& page->cache_offset == fOffset - 1) {
		// prepend to first iovec and adjust offset
		fVecs[0].iov_base = (void*)nextBase;
		fVecs[0].iov_len += B_PAGE_SIZE;
		fOffset = page->cache_offset;
		fPageCount++;
		return true;
	}

	if ((page->cache_offset == fOffset + fPageCount
			|| page->cache_offset == fOffset - 1)
		&& fVecCount < sizeof(fVecs) / sizeof(fVecs[0])) {
		// not physically contiguous or not in the right order
		uint32 vectorIndex;
		if (page->cache_offset < fOffset) {
			// we are pre-pending another vector, move the other vecs
			for (uint32 i = fVecCount; i > 0; i--)
				fVecs[i] = fVecs[i - 1];

			fOffset = page->cache_offset;
			vectorIndex = 0;
		} else
			vectorIndex = fVecCount;

		fVecs[vectorIndex].iov_base
			= (void*)(page->physical_page_number << PAGE_SHIFT);
		fVecs[vectorIndex].iov_len = B_PAGE_SIZE;

		fVecCount++;
		fPageCount++;
		return true;
	}

	return false;
}


status_t
PageWriteTransfer::Schedule(uint32 flags)
{
	off_t writeOffset = (off_t)fOffset << PAGE_SHIFT;
	size_t writeLength = fPageCount << PAGE_SHIFT;

	if (fRun != NULL) {
		return fCache->WriteAsync(writeOffset, fVecs, fVecCount, writeLength,
			flags | B_PHYSICAL_IO_REQUEST, this);
	}

	status_t status = fCache->Write(writeOffset, fVecs, fVecCount,
		flags | B_PHYSICAL_IO_REQUEST, &writeLength);

	SetStatus(status, writeLength);
	return fStatus;
}


void
PageWriteTransfer::SetStatus(status_t status, size_t transferred)
{
	// only succeed if all pages up to the last one have been written fully
	// and the last page has at least been written partially
	if (status == B_OK && transferred <= (fPageCount - 1) * B_PAGE_SIZE)
		status = B_ERROR;

	fStatus = status;
}


void
PageWriteTransfer::IOFinished(status_t status, bool partialTransfer,
	size_t bytesTransferred)
{
	SetStatus(status, bytesTransferred);
	fRun->PageWritten(this, fStatus, partialTransfer, bytesTransferred);
}


status_t
PageWriterRun::Init(uint32 maxPages)
{
	fMaxPages = maxPages;
	fWrapperCount = 0;
	fTransferCount = 0;
	fPendingTransfers = 0;

	fWrappers = new(std::nothrow) PageWriteWrapper[maxPages];
	fTransfers = new(std::nothrow) PageWriteTransfer[maxPages];
	if (fWrappers == NULL || fTransfers == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
PageWriterRun::PrepareNextRun()
{
	fWrapperCount = 0;
	fTransferCount = 0;
	fPendingTransfers = 0;
}


void
PageWriterRun::AddPage(vm_page* page)
{
	fWrappers[fWrapperCount++].SetTo(page, true);

	if (fTransferCount == 0 || !fTransfers[fTransferCount - 1].AddPage(page)) {
		fTransfers[fTransferCount++].SetTo(this, page,
			page->cache->MaxPagesPerAsyncWrite());
	}
}


void
PageWriterRun::Go()
{
	fPendingTransfers = fTransferCount;

	fAllFinishedCondition.Init(this, "page writer wait for I/O");
	ConditionVariableEntry waitEntry;
	fAllFinishedCondition.Add(&waitEntry);

	// schedule writes
	for (uint32 i = 0; i < fTransferCount; i++)
		fTransfers[i].Schedule(B_VIP_IO_REQUEST);

	// wait until all pages have been written
	waitEntry.Wait();

	// mark pages depending on whether they could be written or not

	uint32 wrapperIndex = 0;
	for (uint32 i = 0; i < fTransferCount; i++) {
		PageWriteTransfer& transfer = fTransfers[i];
		transfer.Cache()->Lock();

		if (transfer.Status() != B_OK) {
			uint32 checkIndex = wrapperIndex;
			for (uint32 j = 0; j < transfer.PageCount(); j++)
				fWrappers[checkIndex++].CheckRemoveFromShrunkenCache();
		}

		MutexLocker locker(sPageLock);
		for (uint32 j = 0; j < transfer.PageCount(); j++)
			fWrappers[wrapperIndex++].Done(transfer.Status());

		locker.Unlock();
		transfer.Cache()->Unlock();
	}

	ASSERT(wrapperIndex == fWrapperCount);

	for (uint32 i = 0; i < fTransferCount; i++) {
		PageWriteTransfer& transfer = fTransfers[i];
		struct VMCache* cache = transfer.Cache();

		// We've acquired a references for each page
		for (uint32 j = 0; j < transfer.PageCount(); j++) {
			// We release the cache references after all pages were made
			// unbusy again - otherwise releasing a vnode could deadlock.
			cache->ReleaseStoreRef();
			cache->ReleaseRef();
		}
	}
}


void
PageWriterRun::PageWritten(PageWriteTransfer* transfer, status_t status,
	bool partialTransfer, size_t bytesTransferred)
{
	if (atomic_add(&fPendingTransfers, -1) == 1)
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

			VMCache *cache = page->cache;

			// Don't write back wired (locked) pages and don't write RAM pages
			// until we're low on pages. Also avoid writing temporary pages that
			// are active.
			if (page->wired_count > 0
				|| (cache->temporary
#if ENABLE_SWAP_SUPPORT
					&& (!lowOnPages /*|| page->usage_count > 0*/
						|| !cache->CanWritePage(
								(off_t)page->cache_offset << PAGE_SHIFT))
#endif
				)) {
				continue;
			}

			// we need our own reference to the store, as it might
			// currently be destructed
			if (cache->AcquireUnreferencedStoreRef() != B_OK) {
				cacheLocker.Unlock();
				thread_yield(true);
				continue;
			}

			MutexLocker locker(sPageLock);

			// state might have changed while we were locking the cache
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
	MutexLocker locker(sPageLock);
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

	MutexLocker _(sPageLock);
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
					MutexLocker _(sPageLock);
					enqueue_page(&sFreePageQueue, page);
					page->state = PAGE_STATE_FREE;

					T(StolenPage());
				} else if (stolen < maxCount)
					pages[stolen] = page;

				stolen++;
				count--;
			} else
				tried = true;
		}

		remove_page_marker(marker);

		MutexLocker locker(sPageLock);

		if ((reserve && sReservedPages <= free_page_queue_count())
			|| count == 0
			|| ((!reserve && (sInactivePageQueue.count > 0))
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
	You need to hold the VMCache lock when calling this function.
	Note that the cache lock is released in this function.
	\param cache The cache.
	\param firstPage Offset (in page size units) of the first page in the range.
	\param endPage End offset (in page size units) of the page range. The page
		at this offset is not included.
*/
status_t
vm_page_write_modified_page_range(struct VMCache* cache, uint32 firstPage,
	uint32 endPage)
{
	static const int32 kMaxPages = 256;
	int32 maxPages = cache->MaxPagesPerWrite();
	if (maxPages < 0 || maxPages > kMaxPages)
		maxPages = kMaxPages;

	PageWriteWrapper stackWrappers[2];
	PageWriteWrapper* wrapperPool = new(nogrow) PageWriteWrapper[maxPages + 1];
	if (wrapperPool == NULL) {
		// don't fail, just limit our capabilities
		wrapperPool = stackWrappers;
		maxPages = 1;
	}

	int32 nextWrapper = 0;

	PageWriteWrapper* wrappers[maxPages];
	int32 usedWrappers = 0;

	PageWriteTransfer transfer;
	bool transferEmpty = true;

	VMCachePagesTree::Iterator it
		= cache->pages.GetIterator(firstPage, true, true);

	while (true) {
		vm_page* page = it.Next();
		if (page == NULL || page->cache_offset >= endPage) {
			if (transferEmpty)
				break;

			page = NULL;
		}

		bool dequeuedPage = false;
		if (page != NULL) {
			if (page->state == PAGE_STATE_MODIFIED) {
				MutexLocker locker(&sPageLock);
				remove_page_from_queue(&sModifiedPageQueue, page);
				dequeuedPage = true;
			} else if (page->state == PAGE_STATE_BUSY
					|| !vm_test_map_modification(page)) {
				page = NULL;
			}
		}

		PageWriteWrapper* wrapper = NULL;
		if (page != NULL) {
			wrapper = &wrapperPool[nextWrapper++];
			if (nextWrapper > maxPages)
				nextWrapper = 0;

			wrapper->SetTo(page, dequeuedPage);
			wrapper->ClearModifiedFlag();

			if (transferEmpty || transfer.AddPage(page)) {
				if (transferEmpty) {
					transfer.SetTo(NULL, page, maxPages);
					transferEmpty = false;
				}

				wrappers[usedWrappers++] = wrapper;
				continue;
			}
		}

		if (transferEmpty)
			continue;

		cache->Unlock();
		status_t status = transfer.Schedule(0);
		cache->Lock();

		// Before disabling interrupts handle part of the special case that
		// writing the page failed due the cache having been shrunk. We need to
		// remove the page from the cache and free it.
		if (status != B_OK) {
			for (int32 i = 0; i < usedWrappers; i++)
				wrappers[i]->CheckRemoveFromShrunkenCache();
		}

		MutexLocker locker(&sPageLock);

		for (int32 i = 0; i < usedWrappers; i++)
			wrappers[i]->Done(status);

		locker.Unlock();

		usedWrappers = 0;

		if (page != NULL) {
			transfer.SetTo(NULL, page, maxPages);
			wrappers[usedWrappers++] = wrapper;
		} else
			transferEmpty = true;
	}

	if (wrapperPool != stackWrappers)
		delete [] wrapperPool;

	return B_OK;
}


/*!	You need to hold the VMCache lock when calling this function.
	Note that the cache lock is released in this function.
*/
status_t
vm_page_write_modified_pages(VMCache *cache)
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
		#if DEBUG_PAGE_QUEUE
			sPages[i].queue = NULL;
		#endif
		#if DEBUG_PAGE_CACHE_TRANSITIONS
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

	MutexLocker _(sPageLock);

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

	MutexLocker locker(sPageLock);
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

	MutexLocker locker(sPageLock);

	T(ReservePages(count));

	sReservedPages += count;
	size_t freePages = free_page_queue_count();
	if (sReservedPages <= freePages)
		return;

	count = sReservedPages - freePages;
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

	MutexLocker locker(sPageLock);

	T(ReservePages(count));

	size_t freePages = free_page_queue_count();
	if (sReservedPages + count > freePages)
		return false;

	sReservedPages += count;
	return true;
}


// TODO: you must not have locked a cache when calling this function with
// reserved == false. See vm_cache_acquire_locked_page_cache().
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

	MutexLocker locker(sPageLock);

	T(AllocatePage(reserved));

	vm_page *page = NULL;
	while (true) {
		if (reserved || sReservedPages < free_page_queue_count()) {
			page = dequeue_page(queue);
			if (page == NULL) {
#if DEBUG_PAGE_QUEUE
				if (queue->count != 0)
					panic("queue %p corrupted, count = %d\n", queue, queue->count);
#endif

				// if the primary queue was empty, grab the page from the
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

	MutexLocker locker(sPageLock);

	if (free_page_queue_count() - sReservedPages < length) {
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
		} else
			start += i;
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
vm_page_allocate_page_run_no_base(int pageState, addr_t count)
{
	MutexLocker locker(sPageLock);

	if (free_page_queue_count() - sReservedPages < count) {
		// TODO: add more tries, ie. free some inactive, ...
		// no free space
		return NULL;
	}

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

	vm_page *firstPage = NULL;
	for (uint32 twice = 0; twice < 2; twice++) {
		vm_page *page = queue->head;
		for (; page != NULL; page = page->queue_next) {
			vm_page *current = page;
			if (current >= &sPages[sNumPages - count])
				continue;

			bool foundRun = true;
			for (uint32 i = 0; i < count; i++, current++) {
				if (current->state != PAGE_STATE_FREE
					&& current->state != PAGE_STATE_CLEAR) {
					foundRun = false;
					break;
				}
			}

			if (foundRun) {
				// pull the pages out of the appropriate queues
				current = page;
				for (uint32 i = 0; i < count; i++, current++) {
					current->is_cleared = current->state == PAGE_STATE_CLEAR;
					set_page_state_nolock(current, PAGE_STATE_BUSY);
					current->usage_count = 2;
				}

				firstPage = page;
				break;
			}
		}

		if (firstPage != NULL)
			break;

		queue = otherQueue;
	}

	T(AllocatePageRun(count));

	locker.Unlock();

	if (firstPage != NULL && pageState == PAGE_STATE_CLEAR) {
		vm_page *current = firstPage;
		for (uint32 i = 0; i < count; i++, current++) {
			if (!current->is_cleared)
	 			clear_page(current);
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
vm_page_free(VMCache *cache, vm_page *page)
{
	MutexLocker _(sPageLock);

	if (page->cache == NULL && page->state == PAGE_STATE_MODIFIED
		&& cache->temporary) {
		sModifiedTemporaryPages--;
	}

	set_page_state_nolock(page, PAGE_STATE_FREE);
}


status_t
vm_page_set_state(vm_page *page, int pageState)
{
	MutexLocker _(sPageLock);

	return set_page_state_nolock(page, pageState);
}


/*!	Moves a page to either the tail of the head of its current queue,
	depending on \a tail.
*/
void
vm_page_requeue(struct vm_page *page, bool tail)
{
	MutexLocker _(sPageLock);
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
	if (reservedPages >= count)
		return 0;

	return count - reservedPages;
}


size_t
vm_page_num_unused_pages(void)
{
	size_t reservedPages = sReservedPages;
	size_t count = free_page_queue_count();
	if (reservedPages >= count)
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
