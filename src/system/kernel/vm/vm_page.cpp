/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <signal.h>
#include <string.h>
#include <stdlib.h>

#include <algorithm>

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
#include <vm/vm.h>
#include <vm/vm_priv.h>
#include <vm/vm_page.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>
#include <vm/VMCache.h>

#include "IORequest.h"
#include "PageCacheLocker.h"
#include "VMAnonymousCache.h"
#include "VMPageQueue.h"


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


// The page reserve an allocation of the certain priority must not touch.
static const size_t kPageReserveForPriority[] = {
	VM_PAGE_RESERVE_USER,		// user
	VM_PAGE_RESERVE_SYSTEM,		// system
	0							// VIP
};

static const uint32 kMinimumSystemReserve = VM_PAGE_RESERVE_USER;


int32 gMappedPagesCount;

static VMPageQueue sFreePageQueue;
static VMPageQueue sClearPageQueue;
static VMPageQueue sModifiedPageQueue;
static VMPageQueue sInactivePageQueue;
static VMPageQueue sActivePageQueue;

static vm_page *sPages;
static addr_t sPhysicalPageOffset;
static size_t sNumPages;
static vint32 sUnreservedFreePages;
static vint32 sSystemReservedPages;
static vint32 sPageDeficit;
static vint32 sModifiedTemporaryPages;

static ConditionVariable sFreePageCondition;
static mutex sPageDeficitLock = MUTEX_INITIALIZER("page deficit");

static rw_lock sFreePageQueuesLock
	= RW_LOCK_INITIALIZER("free/clear page queues");

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
		AllocatePage()
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page alloc");
		}
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
			fCache(page->Cache()),
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


static int
find_page(int argc, char **argv)
{
	struct vm_page *page;
	addr_t address;
	int32 index = 1;
	int i;

	struct {
		const char*	name;
		VMPageQueue*	queue;
	} pageQueueInfos[] = {
		{ "free",		&sFreePageQueue },
		{ "clear",		&sClearPageQueue },
		{ "modified",	&sModifiedPageQueue },
		{ "active",		&sActivePageQueue },
		{ "inactive",	&sInactivePageQueue },
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
		VMPageQueue::Iterator it = pageQueueInfos[i].queue->GetIterator();
		while (vm_page* p = it.Next()) {
			if (p == page) {
				kprintf("found page %p in queue %p (%s)\n", page,
					pageQueueInfos[i].queue, pageQueueInfos[i].name);
				return 0;
			}
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
	bool addressIsPointer = true;
	bool physical = false;
	bool searchMappings = false;
	int32 index = 1;

	while (index < argc) {
		if (argv[index][0] != '-')
			break;

		if (!strcmp(argv[index], "-p")) {
			addressIsPointer = false;
			physical = true;
		} else if (!strcmp(argv[index], "-v")) {
			addressIsPointer = false;
		} else if (!strcmp(argv[index], "-m")) {
			searchMappings = true;
		} else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}

		index++;
	}

	if (index + 1 != argc) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	uint64 value;
	if (!evaluate_debug_expression(argv[index], &value, false))
		return 0;

	addr_t pageAddress = (addr_t)value;
	struct vm_page* page;

	if (addressIsPointer) {
		page = (struct vm_page *)pageAddress;
	} else {
		if (!physical) {
			VMAddressSpace *addressSpace = VMAddressSpace::Kernel();

			if (debug_get_debugged_thread()->team->address_space != NULL)
				addressSpace = debug_get_debugged_thread()->team->address_space;

			uint32 flags = 0;
			if (addressSpace->TranslationMap()->QueryInterrupt(pageAddress,
					&pageAddress, &flags) != B_OK
				|| (flags & PAGE_PRESENT) == 0) {
				kprintf("Virtual address not mapped to a physical page in this "
					"address space.\n");
				return 0;
			}
		}

		page = vm_lookup_page(pageAddress / B_PAGE_SIZE);
	}

	kprintf("PAGE: %p\n", page);
	kprintf("queue_next,prev: %p, %p\n", page->queue_link.next,
		page->queue_link.previous);
	kprintf("physical_number: %#lx\n", page->physical_page_number);
	kprintf("cache:           %p\n", page->Cache());
	kprintf("cache_offset:    %ld\n", page->cache_offset);
	kprintf("cache_next:      %p\n", page->cache_next);
	kprintf("is dummy:        %d\n", page->is_dummy);
	kprintf("state:           %s\n", page_state_to_string(page->state));
	kprintf("wired_count:     %d\n", page->wired_count);
	kprintf("usage_count:     %d\n", page->usage_count);
	kprintf("busy_writing:    %d\n", page->busy_writing);
	kprintf("accessed:        %d\n", page->accessed);
	kprintf("modified:        %d\n", page->modified);
	#if DEBUG_PAGE_QUEUE
		kprintf("queue:           %p\n", page->queue);
	#endif
	#if DEBUG_PAGE_ACCESS
		kprintf("accessor:        %" B_PRId32 "\n", page->accessing_thread);
	#endif
	kprintf("area mappings:\n");

	vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
	vm_page_mapping *mapping;
	while ((mapping = iterator.Next()) != NULL) {
		kprintf("  %p (%" B_PRId32 ")\n", mapping->area, mapping->area->id);
		mapping = mapping->page_link.next;
	}

	if (searchMappings) {
		kprintf("all mappings:\n");
		VMAddressSpace* addressSpace = VMAddressSpace::DebugFirst();
		while (addressSpace != NULL) {
			size_t pageCount = addressSpace->Size() / B_PAGE_SIZE;
			for (addr_t address = addressSpace->Base(); pageCount != 0;
					address += B_PAGE_SIZE, pageCount--) {
				addr_t physicalAddress;
				uint32 flags = 0;
				if (addressSpace->TranslationMap()->QueryInterrupt(address,
						&physicalAddress, &flags) == B_OK
					&& (flags & PAGE_PRESENT) != 0
					&& physicalAddress / B_PAGE_SIZE
						== page->physical_page_number) {
					VMArea* area = addressSpace->LookupArea(address);
					kprintf("  aspace %ld, area %ld: %#" B_PRIxADDR
						" (%c%c%s%s)\n", addressSpace->ID(),
						area != NULL ? area->id : -1, address,
						(flags & B_KERNEL_READ_AREA) != 0 ? 'r' : '-',
						(flags & B_KERNEL_WRITE_AREA) != 0 ? 'w' : '-',
						(flags & PAGE_MODIFIED) != 0 ? " modified" : "",
						(flags & PAGE_ACCESSED) != 0 ? " accessed" : "");
				}
			}
			addressSpace = VMAddressSpace::DebugNext(addressSpace);
		}
	}

	return 0;
}


static int
dump_page_queue(int argc, char **argv)
{
	struct VMPageQueue *queue;

	if (argc < 2) {
		kprintf("usage: page_queue <address/name> [list]\n");
		return 0;
	}

	if (strlen(argv[1]) >= 2 && argv[1][0] == '0' && argv[1][1] == 'x')
		queue = (VMPageQueue*)strtoul(argv[1], NULL, 16);
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
		queue, queue->Head(), queue->Tail(), queue->Count());

	if (argc == 3) {
		struct vm_page *page = queue->Head();
		const char *type = "none";
		int i;

		if (page->Cache() != NULL) {
			switch (page->Cache()->type) {
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
		for (i = 0; page; i++, page = queue->Next(page)) {
			kprintf("%p  %p  %-7s %8s  %5d  %5d\n", page, page->Cache(),
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

		if (sPages[i].state == PAGE_STATE_MODIFIED && sPages[i].Cache() != NULL
			&& sPages[i].Cache()->temporary && sPages[i].wired_count == 0) {
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
	kprintf("unreserved free pages: %" B_PRId32 "\n", sUnreservedFreePages);
	kprintf("system reserved pages: %" B_PRId32 "\n", sSystemReservedPages);
	kprintf("page deficit: %lu\n", sPageDeficit);
	kprintf("mapped pages: %lu\n", gMappedPagesCount);

	kprintf("\nfree queue: %p, count = %ld\n", &sFreePageQueue,
		sFreePageQueue.Count());
	kprintf("clear queue: %p, count = %ld\n", &sClearPageQueue,
		sClearPageQueue.Count());
	kprintf("modified queue: %p, count = %ld (%ld temporary, %lu swappable, "
		"inactive: %lu)\n", &sModifiedPageQueue, sModifiedPageQueue.Count(),
		sModifiedTemporaryPages, swappableModified, swappableModifiedInactive);
	kprintf("active queue: %p, count = %ld\n", &sActivePageQueue,
		sActivePageQueue.Count());
	kprintf("inactive queue: %p, count = %ld\n", &sInactivePageQueue,
		sInactivePageQueue.Count());
	return 0;
}


// #pragma mark -


static void
unreserve_page()
{
	int32 systemReserve = sSystemReservedPages;
	if (systemReserve >= (int32)kMinimumSystemReserve) {
		atomic_add(&sUnreservedFreePages, 1);
	} else {
		// Note: Due to the race condition, we might increment
		// sSystemReservedPages beyond its desired count. That doesn't matter
		// all that much, though, since its only about a single page and
		// vm_page_reserve_pages() will correct this when the general reserve
		// is running low.
		atomic_add(&sSystemReservedPages, 1);
	}

	if (sPageDeficit > 0) {
		MutexLocker pageDeficitLocker(sPageDeficitLock);
		if (sPageDeficit > 0)
			sFreePageCondition.NotifyAll();
	}
}


static void
free_page(vm_page* page, bool clear)
{
	DEBUG_PAGE_ACCESS_CHECK(page);

	VMPageQueue* fromQueue;

	switch (page->state) {
		case PAGE_STATE_BUSY:
		case PAGE_STATE_ACTIVE:
			fromQueue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			fromQueue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			fromQueue = &sModifiedPageQueue;
			break;
		case PAGE_STATE_FREE:
		case PAGE_STATE_CLEAR:
			panic("free_page(): page %p already free", page);
			return;
		case PAGE_STATE_WIRED:
		case PAGE_STATE_UNUSED:
			fromQueue = NULL;
			break;
		default:
			panic("free_page(): page %p in invalid state %d",
				page, page->state);
			return;
	}

	if (page->CacheRef() != NULL)
		panic("to be freed page %p has cache", page);
	if (!page->mappings.IsEmpty() || page->wired_count > 0)
		panic("to be freed page %p has mappings", page);

	if (sPageDeficit > 0) {
		MutexLocker pageDeficitLocker(sPageDeficitLock);
		if (sPageDeficit > 0)
			sFreePageCondition.NotifyOne();
	}

	if (fromQueue != NULL)
		fromQueue->RemoveUnlocked(page);

	T(FreePage());

	ReadLocker locker(sFreePageQueuesLock);

	DEBUG_PAGE_ACCESS_END(page);

	if (clear) {
		page->state = PAGE_STATE_CLEAR;
		sClearPageQueue.PrependUnlocked(page);
	} else {
		page->state = PAGE_STATE_FREE;
		sFreePageQueue.PrependUnlocked(page);
	}

	locker.Unlock();

	unreserve_page();
}


/*!	The caller must make sure that no-one else tries to change the page's state
	while the function is called. If the page has a cache, this can be done by
	locking the cache.
*/
static void
set_page_state(vm_page *page, int pageState)
{
	DEBUG_PAGE_ACCESS_CHECK(page);

	if (pageState == page->state)
		return;

	VMPageQueue* fromQueue;

	switch (page->state) {
		case PAGE_STATE_BUSY:
		case PAGE_STATE_ACTIVE:
			fromQueue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			fromQueue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			fromQueue = &sModifiedPageQueue;
			break;
		case PAGE_STATE_FREE:
		case PAGE_STATE_CLEAR:
			panic("set_page_state(): page %p is free/clear", page);
			return;
		case PAGE_STATE_WIRED:
		case PAGE_STATE_UNUSED:
			fromQueue = NULL;
			break;
		default:
			panic("set_page_state(): page %p in invalid state %d",
				page, page->state);
			return;
	}

	VMPageQueue* toQueue;

	switch (pageState) {
		case PAGE_STATE_BUSY:
		case PAGE_STATE_ACTIVE:
			toQueue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			toQueue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			toQueue = &sModifiedPageQueue;
			break;
		case PAGE_STATE_FREE:
		case PAGE_STATE_CLEAR:
			panic("set_page_state(): target state is free/clear");
			return;
		case PAGE_STATE_WIRED:
		case PAGE_STATE_UNUSED:
			toQueue = NULL;
			break;
		default:
			panic("set_page_state(): invalid target state %d", pageState);
			return;
	}

	if (pageState == PAGE_STATE_INACTIVE && sPageDeficit > 0) {
		MutexLocker pageDeficitLocker(sPageDeficitLock);
		if (sPageDeficit > 0)
			sFreePageCondition.NotifyOne();
	}

	VMCache* cache = page->Cache();
	if (cache != NULL && cache->temporary) {
		if (pageState == PAGE_STATE_MODIFIED)
			atomic_add(&sModifiedTemporaryPages, 1);
		else if (page->state == PAGE_STATE_MODIFIED)
			atomic_add(&sModifiedTemporaryPages, -1);
	}

	// move the page
	if (toQueue == fromQueue) {
		// Note: Theoretically we are required to lock when changing the page
		// state, even if we don't change the queue. We actually don't have to
		// do this, though, since only for the active queue there are different
		// page states and active pages have a cache that must be locked at
		// this point. So we rely on the fact that everyone must lock the cache
		// before trying to change/interpret the page state.
		ASSERT(cache != NULL);
		cache->AssertLocked();
		page->state = pageState;
	} else {
		if (fromQueue != NULL)
			fromQueue->RemoveUnlocked(page);

		page->state = pageState;

		if (toQueue != NULL)
			toQueue->AppendUnlocked(page);
	}
}


/*! Moves a modified page into either the active or inactive page queue
	depending on its usage count and wiring.
	The page queues must not be locked.
*/
static void
move_page_to_active_or_inactive_queue(vm_page *page, bool dequeued)
{
	DEBUG_PAGE_ACCESS_CHECK(page);

	// Note, this logic must be in sync with what the page daemon does
	int32 state;
	if (!page->mappings.IsEmpty() || page->usage_count >= 0
		|| page->wired_count)
		state = PAGE_STATE_ACTIVE;
	else
		state = PAGE_STATE_INACTIVE;

	if (dequeued) {
		page->state = state;
		VMPageQueue& queue = state == PAGE_STATE_ACTIVE
			? sActivePageQueue : sInactivePageQueue;
		queue.AppendUnlocked(page);
		if (page->Cache()->temporary)
			atomic_add(&sModifiedTemporaryPages, -1);
	} else
		set_page_state(page, state);
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

		if (sFreePageQueue.Count() == 0)
			continue;

		// Since we temporarily remove pages from the free pages reserve,
		// we must make sure we don't cause a violation of the page
		// reservation warranty. The following is usually stricter than
		// necessary, because we don't have information on how many of the
		// reserved pages have already been allocated.
		if (!vm_page_try_reserve_pages(SCRUB_SIZE, VM_PRIORITY_USER))
			continue;

		// get some pages from the free queue
		ReadLocker locker(sFreePageQueuesLock);

		vm_page *page[SCRUB_SIZE];
		int32 scrubCount = 0;
		for (int32 i = 0; i < SCRUB_SIZE; i++) {
			page[i] = sFreePageQueue.RemoveHeadUnlocked();
			if (page[i] == NULL)
				break;

			DEBUG_PAGE_ACCESS_START(page[i]);

			page[i]->state = PAGE_STATE_BUSY;
			scrubCount++;
		}

		locker.Unlock();

		if (scrubCount == 0) {
			vm_page_unreserve_pages(SCRUB_SIZE);
			continue;
		}

		T(ScrubbingPages(scrubCount));

		// clear them
		for (int32 i = 0; i < scrubCount; i++)
			clear_page(page[i]);

		locker.Lock();

		// and put them into the clear queue
		for (int32 i = 0; i < scrubCount; i++) {
			page[i]->state = PAGE_STATE_CLEAR;
			DEBUG_PAGE_ACCESS_END(page[i]);
			sClearPageQueue.PrependUnlocked(page[i]);
		}

		locker.Unlock();

		vm_page_unreserve_pages(SCRUB_SIZE);

		T(ScrubbedPages(scrubCount));
	}

	return 0;
}


static inline bool
is_marker_page(struct vm_page *page)
{
	return page->is_dummy;
}


static void
remove_page_marker(struct vm_page &marker)
{
	if (marker.state == PAGE_STATE_UNUSED)
		return;

	DEBUG_PAGE_ACCESS_CHECK(&marker);

	VMPageQueue *queue;

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

	queue->RemoveUnlocked(&marker);

	marker.state = PAGE_STATE_UNUSED;
}


static vm_page *
next_modified_page(struct vm_page &marker)
{
	InterruptsSpinLocker locker(sModifiedPageQueue.GetLock());
	vm_page *page;

	DEBUG_PAGE_ACCESS_CHECK(&marker);

	if (marker.state == PAGE_STATE_MODIFIED) {
		page = sModifiedPageQueue.Next(&marker);
		sModifiedPageQueue.Remove(&marker);
		marker.state = PAGE_STATE_UNUSED;
	} else
		page = sModifiedPageQueue.Head();

	for (; page != NULL; page = sModifiedPageQueue.Next(page)) {
		if (!is_marker_page(page) && page->state != PAGE_STATE_BUSY) {
			// insert marker
			marker.state = PAGE_STATE_MODIFIED;
			sModifiedPageQueue.InsertAfter(page, &marker);
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


/*!	The page's cache must be locked.
*/
void
PageWriteWrapper::SetTo(vm_page* page, bool dequeuedPage)
{
	DEBUG_PAGE_ACCESS_CHECK(page);

	if (page->state == PAGE_STATE_BUSY)
		panic("setting page write wrapper to busy page");

	if (fIsActive)
		panic("re-setting page write wrapper that isn't completed");

	fPage = page;
	fCache = page->Cache();
	fDequeuedPage = dequeuedPage;
	fIsActive = true;

	fOldPageState = fPage->state;
	fPage->state = PAGE_STATE_BUSY;
	fPage->busy_writing = true;
}


/*!	The page's cache must be locked.
*/
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


/*!	The page's cache must be locked.
*/
void
PageWriteWrapper::CheckRemoveFromShrunkenCache()
{
	if (fPage->busy_writing)
		return;

	vm_remove_all_page_mappings(fPage, NULL);
	fCache->RemovePage(fPage);
}


/*!	The page's cache must be locked.
	The page queues must not be locked.
*/
void
PageWriteWrapper::Done(status_t result)
{
	if (!fIsActive)
		panic("completing page write wrapper that is not active");

	DEBUG_PAGE_ACCESS_CHECK(fPage);

	if (result == B_OK) {
		// put it into the active/inactive queue
		move_page_to_active_or_inactive_queue(fPage, fDequeuedPage);
		fPage->busy_writing = false;
		DEBUG_PAGE_ACCESS_END(fPage);
	} else {
		// Writing the page failed -- move to the modified queue. If we dequeued
		// it from there, just enqueue it again, otherwise set the page state
		// explicitly, which will take care of moving between the queues.
		if (fDequeuedPage) {
			fPage->state = PAGE_STATE_MODIFIED;
			sModifiedPageQueue.AppendUnlocked(fPage);
		} else {
			fPage->state = fOldPageState;
			set_page_state(fPage, PAGE_STATE_MODIFIED);
		}

		if (!fPage->busy_writing) {
			// The busy_writing flag was cleared. That means the cache has been
			// shrunk while we were trying to write the page and we have to free
			// it now.

			// Adjust temporary modified pages count, if necessary.
			if (fDequeuedPage && fCache->temporary)
				atomic_add(&sModifiedTemporaryPages, -1);

			// free the page
			free_page(fPage, false);
		} else {
			fPage->busy_writing = false;
			DEBUG_PAGE_ACCESS_END(fPage);
		}
	}


	fCache->NotifyPageEvents(fPage, PAGE_EVENT_NOT_BUSY);
	fIsActive = false;
}


/*!	The page's cache must be locked.
*/
void
PageWriteTransfer::SetTo(PageWriterRun* run, vm_page* page, int32 maxPages)
{
	fRun = run;
	fCache = page->Cache();
	fOffset = page->cache_offset;
	fPageCount = 1;
	fMaxPages = maxPages;
	fStatus = B_OK;

	fVecs[0].iov_base = (void*)(page->physical_page_number << PAGE_SHIFT);
	fVecs[0].iov_len = B_PAGE_SIZE;
	fVecCount = 1;
}


/*!	The page's cache must be locked.
*/
bool
PageWriteTransfer::AddPage(vm_page* page)
{
	if (page->Cache() != fCache
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


/*!	The page's cache must be locked.
*/
void
PageWriterRun::AddPage(vm_page* page)
{
	fWrappers[fWrapperCount++].SetTo(page, true);

	if (fTransferCount == 0 || !fTransfers[fTransferCount - 1].AddPage(page)) {
		fTransfers[fTransferCount++].SetTo(this, page,
			page->Cache()->MaxPagesPerAsyncWrite());
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

		for (uint32 j = 0; j < transfer.PageCount(); j++)
			fWrappers[wrapperIndex++].Done(transfer.Status());

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
	marker.is_dummy = true;
	marker.SetCacheRef(NULL);
	marker.state = PAGE_STATE_UNUSED;
#if DEBUG_PAGE_QUEUE
	marker.queue = NULL;
#endif
#if DEBUG_PAGE_ACCESS
	marker.accessing_thread = thread_get_current_thread_id();
#endif

	while (true) {
		if ((int32)sModifiedPageQueue.Count() - sModifiedTemporaryPages
				< 1024) {
			int32 count = 0;
			get_sem_count(sWriterWaitSem, &count);
			if (count == 0)
				count = 1;

			acquire_sem_etc(sWriterWaitSem, count, B_RELATIVE_TIMEOUT, 3000000);
				// all 3 seconds when no one triggers us
		}

		int32 modifiedPages = sModifiedPageQueue.Count()
			- sModifiedTemporaryPages;
		if (modifiedPages <= 0)
			continue;

		// depending on how urgent it becomes to get pages to disk, we adjust
		// our I/O priority
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

			DEBUG_PAGE_ACCESS_START(page);

			VMCache *cache = page->Cache();

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
				DEBUG_PAGE_ACCESS_END(page);
				continue;
			}

			// we need our own reference to the store, as it might
			// currently be destructed
			if (cache->AcquireUnreferencedStoreRef() != B_OK) {
				DEBUG_PAGE_ACCESS_END(page);
				cacheLocker.Unlock();
				thread_yield(true);
				continue;
			}

			// state might have changed while we were locking the cache
			if (page->state != PAGE_STATE_MODIFIED) {
				// release the cache reference
				DEBUG_PAGE_ACCESS_END(page);
				cache->ReleaseStoreRef();
				continue;
			}

			sModifiedPageQueue.RemoveUnlocked(page);
			run.AddPage(page);

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
find_page_candidate(struct vm_page &marker)
{
	DEBUG_PAGE_ACCESS_CHECK(&marker);

	InterruptsSpinLocker locker(sInactivePageQueue.GetLock());
	vm_page *page;

	if (marker.state == PAGE_STATE_UNUSED) {
		// Get the first free pages of the (in)active queue
		page = sInactivePageQueue.Head();
	} else {
		// Get the next page of the current queue
		if (marker.state != PAGE_STATE_INACTIVE) {
			panic("invalid marker %p state", &marker);
			return NULL;
		}

		page = sInactivePageQueue.Next(&marker);
		sInactivePageQueue.Remove(&marker);
		marker.state = PAGE_STATE_UNUSED;
	}

	while (page != NULL) {
		if (!is_marker_page(page) && page->state == PAGE_STATE_INACTIVE) {
			// we found a candidate, insert marker
			marker.state = PAGE_STATE_INACTIVE;
			sInactivePageQueue.InsertAfter(page, &marker);
			return page;
		}

		page = sInactivePageQueue.Next(page);
	}

	return NULL;
}


static bool
steal_page(vm_page *page)
{
	// try to lock the page's cache
	if (vm_cache_acquire_locked_page_cache(page, false) == NULL)
		return false;
	VMCache* cache = page->Cache();

	AutoLocker<VMCache> cacheLocker(cache, true);
	MethodDeleter<VMCache> _2(cache, &VMCache::ReleaseRefLocked);

	// check again if that page is still a candidate
	if (page->state != PAGE_STATE_INACTIVE)
		return false;

	DEBUG_PAGE_ACCESS_START(page);

	// recheck eventual last minute changes
	uint32 flags;
	vm_remove_all_page_mappings(page, &flags);
	if ((flags & PAGE_MODIFIED) != 0) {
		// page was modified, don't steal it
		set_page_state(page, PAGE_STATE_MODIFIED);
		DEBUG_PAGE_ACCESS_END(page);
		return false;
	} else if ((flags & PAGE_ACCESSED) != 0) {
		// page is in active use, don't steal it
		set_page_state(page, PAGE_STATE_ACTIVE);
		DEBUG_PAGE_ACCESS_END(page);
		return false;
	}

	// we can now steal this page

	//dprintf("  steal page %p from cache %p%s\n", page, page->cache,
	//	page->state == PAGE_STATE_INACTIVE ? "" : " (ACTIVE)");

	cache->RemovePage(page);

	sInactivePageQueue.RemoveUnlocked(page);
	return true;
}


static size_t
steal_pages(vm_page **pages, size_t count)
{
	while (true) {
		vm_page marker;
		marker.is_dummy = true;
		marker.SetCacheRef(NULL);
		marker.state = PAGE_STATE_UNUSED;
#if DEBUG_PAGE_QUEUE
		marker.queue = NULL;
#endif
#if DEBUG_PAGE_ACCESS
		marker.accessing_thread = thread_get_current_thread_id();
#endif

		bool tried = false;
		size_t stolen = 0;

		while (count > 0) {
			vm_page *page = find_page_candidate(marker);
			if (page == NULL)
				break;

			if (steal_page(page)) {
				ReadLocker locker(sFreePageQueuesLock);
				page->state = PAGE_STATE_FREE;
				DEBUG_PAGE_ACCESS_END(page);
				sFreePageQueue.PrependUnlocked(page);
				locker.Unlock();

				unreserve_page();

				T(StolenPage());

				stolen++;
				count--;
			} else
				tried = true;
		}

		remove_page_marker(marker);

		if (count == 0 || sUnreservedFreePages >= 0)
			return stolen;

		if (stolen && !tried && sInactivePageQueue.Count() > 0) {
			count++;
			continue;
		}

		// we need to wait for pages to become inactive

		MutexLocker pageDeficitLocker(sPageDeficitLock);

		sPageDeficit++;
		if (sUnreservedFreePages >= 0) {
			// There are enough pages available now. No need to wait after all.
			sPageDeficit--;
			return stolen;
		}

		ConditionVariableEntry freeConditionEntry;
		freeConditionEntry.Add(&sFreePageQueue);
		pageDeficitLocker.Unlock();

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

		pageDeficitLocker.Lock();
		sPageDeficit--;

		if (sUnreservedFreePages >= 0)
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

	const uint32 allocationFlags = HEAP_DONT_WAIT_FOR_MEMORY
		| HEAP_DONT_LOCK_KERNEL_SPACE;

	PageWriteWrapper stackWrappers[2];
	PageWriteWrapper* wrapperPool
		= new(malloc_flags(allocationFlags)) PageWriteWrapper[maxPages + 1];
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
				DEBUG_PAGE_ACCESS_START(page);
				sModifiedPageQueue.RemoveUnlocked(page);
				dequeuedPage = true;
			} else if (page->state == PAGE_STATE_BUSY
					|| !vm_test_map_modification(page)) {
				page = NULL;
			} else
				DEBUG_PAGE_ACCESS_START(page);
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

		for (int32 i = 0; i < usedWrappers; i++)
			wrappers[i]->Done(status);

		usedWrappers = 0;

		if (page != NULL) {
			transfer.SetTo(NULL, page, maxPages);
			wrappers[usedWrappers++] = wrapper;
		} else
			transferEmpty = true;
	}

	if (wrapperPool != stackWrappers)
		delete[] wrapperPool;

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
			DEBUG_PAGE_ACCESS_START(page);
			vm_page_requeue(page, false);
			modified++;
			DEBUG_PAGE_ACCESS_END(page);
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

	// init page queues
	sModifiedPageQueue.Init("modified pages queue");
	sInactivePageQueue.Init("inactive pages queue");
	sActivePageQueue.Init("active pages queue");
	sFreePageQueue.Init("free pages queue");
	sClearPageQueue.Init("clear pages queue");

	// map in the new free page table
	sPages = (vm_page *)vm_allocate_early(args, sNumPages * sizeof(vm_page),
		~0L, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, false);

	TRACE(("vm_init: putting free_page_table @ %p, # ents %ld (size 0x%x)\n",
		sPages, sNumPages, (unsigned int)(sNumPages * sizeof(vm_page))));

	// initialize the free page table
	for (uint32 i = 0; i < sNumPages; i++) {
		sPages[i].physical_page_number = sPhysicalPageOffset + i;
		sPages[i].is_dummy = false;
		sPages[i].state = PAGE_STATE_FREE;
		new(&sPages[i].mappings) vm_page_mappings();
		sPages[i].wired_count = 0;
		sPages[i].usage_count = 0;
		sPages[i].busy_writing = false;
		sPages[i].SetCacheRef(NULL);
		#if DEBUG_PAGE_QUEUE
			sPages[i].queue = NULL;
		#endif
		#if DEBUG_PAGE_ACCESS
			sPages[i].accessing_thread = -1;
		#endif
		sFreePageQueue.Append(&sPages[i]);
	}

	sUnreservedFreePages = sNumPages;

	TRACE(("initialized table\n"));

	// mark some of the page ranges inuse
	for (uint32 i = 0; i < args->num_physical_allocated_ranges; i++) {
		vm_mark_page_range_inuse(args->physical_allocated_range[i].start / B_PAGE_SIZE,
			args->physical_allocated_range[i].size / B_PAGE_SIZE);
	}

	TRACE(("vm_page_init: exit\n"));

	// reserve pages for the system, that user allocations will not touch
	if (sUnreservedFreePages < (int32)kMinimumSystemReserve) {
		panic("Less pages than the system reserve!");
		sSystemReservedPages = sUnreservedFreePages;
	} else
		sSystemReservedPages = kMinimumSystemReserve;
	sUnreservedFreePages -= sSystemReservedPages;

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

	add_debugger_command("page_stats", &dump_page_stats,
		"Dump statistics about page usage");
	add_debugger_command_etc("page", &dump_page,
		"Dump page info",
		"[ \"-p\" | \"-v\" ] [ \"-m\" ] <address>\n"
		"Prints information for the physical page. If neither \"-p\" nor\n"
		"\"-v\" are given, the provided address is interpreted as address of\n"
		"the vm_page data structure for the page in question. If \"-p\" is\n"
		"given, the address is the physical address of the page. If \"-v\" is\n"
		"given, the address is interpreted as virtual address in the current\n"
		"thread's address space and for the page it is mapped to (if any)\n"
		"information are printed. If \"-m\" is specified, the command will\n"
		"search all known address spaces for mappings to that page and print\n"
		"them.\n", 0);
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

	WriteLocker locker(sFreePageQueuesLock);

	for (addr_t i = 0; i < length; i++) {
		vm_page *page = &sPages[startPage + i];
		switch (page->state) {
			case PAGE_STATE_FREE:
			case PAGE_STATE_CLEAR:
			{
// TODO: This violates the page reservation policy, since we remove pages from
// the free/clear queues without having reserved them before. This should happen
// in the early boot process only, though.
				DEBUG_PAGE_ACCESS_START(page);
				VMPageQueue& queue = page->state == PAGE_STATE_FREE
					? sFreePageQueue : sClearPageQueue;
				queue.Remove(page);
				page->state = PAGE_STATE_UNUSED;
				atomic_add(&sUnreservedFreePages, -1);
				DEBUG_PAGE_ACCESS_END(page);
				break;
			}
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

	T(UnreservePages(count));

	while (true) {
		int32 systemReserve = sSystemReservedPages;
		if (systemReserve >= (int32)kMinimumSystemReserve)
			break;

		int32 toUnreserve = std::min((int32)count,
			(int32)kMinimumSystemReserve - systemReserve);
		if (atomic_test_and_set(&sSystemReservedPages,
					systemReserve + toUnreserve, systemReserve)
				== systemReserve) {
			count -= toUnreserve;
			if (count == 0)
				return;
			break;
		}

		// the count changed in the meantime -- retry
	}

	atomic_add(&sUnreservedFreePages, count);

	if (sPageDeficit > 0) {
		MutexLocker pageDeficitLocker(sPageDeficitLock);
		if (sPageDeficit > 0)
			sFreePageCondition.NotifyAll();
	}
}


/*!	With this call, you can reserve a number of free pages in the system.
	They will only be handed out to someone who has actually reserved them.
	This call returns as soon as the number of requested pages has been
	reached.
	The caller must not hold any cache lock or the function might deadlock.
*/
void
vm_page_reserve_pages(uint32 count, int priority)
{
	if (count == 0)
		return;

	T(ReservePages(count));

	while (true) {
		// Of the requested count reserve as many pages as possible from the
		// general reserve.
		int32 freePages = sUnreservedFreePages;
		if (freePages <= 0)
			break;

		uint32 toReserve = std::min((int32)count, freePages);
		if (atomic_test_and_set(&sUnreservedFreePages,
					freePages - toReserve, freePages)
				!= freePages) {
			// the count changed in the meantime -- retry
			continue;
		}

		count -= toReserve;
		if (count == 0)
			return;

		break;
	}

	// Try to get the remaining pages from the system reserve.
	uint32 systemReserve = kPageReserveForPriority[priority];
	while (true) {
		int32 systemFreePages = sSystemReservedPages;
		uint32 toReserve = 0;
		if (systemFreePages > (int32)systemReserve) {
			toReserve = std::min(count, systemFreePages - systemReserve);
			if (atomic_test_and_set(&sSystemReservedPages,
						systemFreePages - toReserve, systemFreePages)
					!= systemFreePages) {
				// the count changed in the meantime -- retry
				continue;
			}
		}

		count -= toReserve;
		if (count == 0)
			return;

		break;
	}

	// subtract the remaining pages
	int32 oldFreePages = atomic_add(&sUnreservedFreePages, -(int32)count);
	if (oldFreePages > 0) {
		if ((int32)count <= oldFreePages)
			return;
		count -= oldFreePages;
// TODO: Activate low-memory handling/page daemon!
	}

	steal_pages(NULL, count + 1);
		// we get one more, just in case we can do something someone
		// else can't
}


bool
vm_page_try_reserve_pages(uint32 count, int priority)
{
	if (count == 0)
		return true;

	T(ReservePages(count));

	while (true) {
		// From the requested count reserve as many pages as possible from the
		// general reserve.
		int32 freePages = sUnreservedFreePages;
		uint32 reserved = 0;
		if (freePages > 0) {
			reserved = std::min((int32)count, freePages);
			if (atomic_test_and_set(&sUnreservedFreePages,
						freePages - reserved, freePages)
					!= freePages) {
				// the count changed in the meantime -- retry
				continue;
			}
		}

		if (reserved == count)
			return true;

		// Try to get the remaining pages from the system reserve.
		uint32 systemReserve = kPageReserveForPriority[priority];
		uint32 leftToReserve = count - reserved;
		while (true) {
			int32 systemFreePages = sSystemReservedPages;
			if ((uint32)systemFreePages < leftToReserve + systemReserve) {
				// no dice
				vm_page_unreserve_pages(reserved);
				return false;
			}

			if (atomic_test_and_set(&sSystemReservedPages,
						systemFreePages - leftToReserve, systemFreePages)
					== systemFreePages) {
				return true;
			}

			// the count changed in the meantime -- retry
			continue;
		}
	}
}


vm_page *
vm_page_allocate_page(int pageState)
{
	VMPageQueue* queue;
	VMPageQueue* otherQueue;

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

	atomic_add(&sUnreservedFreePages, -1);

	T(AllocatePage());

	ReadLocker locker(sFreePageQueuesLock);

	vm_page* page = queue->RemoveHeadUnlocked();
	if (page == NULL) {
		// if the primary queue was empty, grab the page from the
		// secondary queue
		page = otherQueue->RemoveHeadUnlocked();

		if (page == NULL) {
			// Unlikely, but possible: the page we have reserved has moved
			// between the queues after we checked the first queue. Grab the
			// write locker to make sure this doesn't happen again.
			locker.Unlock();
			WriteLocker writeLocker(sFreePageQueuesLock);

			page = queue->RemoveHead();
			if (page == NULL)
				otherQueue->RemoveHead();

			if (page == NULL) {
				panic("Had reserved page, but there is none!");
				return NULL;
			}

			// downgrade to read lock
			locker.Lock();
		}
	}

	if (page->CacheRef() != NULL)
		panic("supposed to be free page %p has cache\n", page);

	DEBUG_PAGE_ACCESS_START(page);

	int oldPageState = page->state;
	page->state = PAGE_STATE_BUSY;
	page->usage_count = 2;
	page->accessed = false;
	page->modified = false;

	locker.Unlock();

	sActivePageQueue.AppendUnlocked(page);

	// clear the page, if we had to take it from the free queue and a clear
	// page was requested
	if (pageState == PAGE_STATE_CLEAR && oldPageState != PAGE_STATE_CLEAR)
		clear_page(page);

	return page;
}


static vm_page*
allocate_page_run(page_num_t start, page_num_t length, int pageState,
	WriteLocker& freeClearQueueLocker)
{
	T(AllocatePageRun(length));

	// pull the pages out of the appropriate queues
	VMPageQueue::PageList freePages;
	VMPageQueue::PageList clearPages;
	for (page_num_t i = 0; i < length; i++) {
		vm_page& page = sPages[start + i];
		DEBUG_PAGE_ACCESS_START(&page);
		if (page.state == PAGE_STATE_CLEAR) {
			sClearPageQueue.Remove(&page);
			clearPages.Add(&page);
		} else {
			sFreePageQueue.Remove(&page);
			freePages.Add(&page);
		}

		page.state = PAGE_STATE_BUSY;
		page.usage_count = 1;
		page.accessed = false;
		page.modified = false;
	}

	freeClearQueueLocker.Unlock();

	// clear pages, if requested
	if (pageState == PAGE_STATE_CLEAR) {
		for (VMPageQueue::PageList::Iterator it = freePages.GetIterator();
				vm_page* page = it.Next();) {
 			clear_page(page);
		}
	}

	// add pages to active queue
	freePages.MoveFrom(&clearPages);
	sActivePageQueue.AppendUnlocked(freePages, length);

	// Note: We don't unreserve the pages since we pulled them out of the
	// free/clear queues without adjusting sUnreservedFreePages.

	return &sPages[start];
}


vm_page *
vm_page_allocate_page_run(int pageState, addr_t base, addr_t length,
	int priority)
{
	uint32 start = base >> PAGE_SHIFT;

	if (!vm_page_try_reserve_pages(length, priority))
		return NULL;
		// TODO: add more tries, ie. free some inactive, ...
		// no free space

	WriteLocker freeClearQueueLocker(sFreePageQueuesLock);

	for (;;) {
		bool foundRun = true;
		if (start + length > sNumPages) {
			freeClearQueueLocker.Unlock();
			vm_page_unreserve_pages(length);
			return NULL;
		}

		uint32 i;
		for (i = 0; i < length; i++) {
			if (sPages[start + i].state != PAGE_STATE_FREE
				&& sPages[start + i].state != PAGE_STATE_CLEAR) {
				foundRun = false;
				i++;
				break;
			}
		}

		if (foundRun)
			return allocate_page_run(start, length, pageState,
				freeClearQueueLocker);

		start += i;
	}
}


vm_page *
vm_page_allocate_page_run_no_base(int pageState, addr_t count, int priority)
{
	VMPageQueue* queue;
	VMPageQueue* otherQueue;
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

	if (!vm_page_try_reserve_pages(count, priority))
		return NULL;
		// TODO: add more tries, ie. free some inactive, ...
		// no free space

	WriteLocker freeClearQueueLocker(sFreePageQueuesLock);

	for (;;) {
		VMPageQueue::Iterator it = queue->GetIterator();
		while (vm_page *page = it.Next()) {
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
				return allocate_page_run(page - sPages, count, pageState,
					freeClearQueueLocker);
			}
		}

		if (queue == otherQueue) {
			freeClearQueueLocker.Unlock();
			vm_page_unreserve_pages(count);
		}

		queue = otherQueue;
	}
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
	ASSERT(page->state != PAGE_STATE_FREE && page->state != PAGE_STATE_CLEAR);

	if (page->state == PAGE_STATE_MODIFIED && cache->temporary)
		atomic_add(&sModifiedTemporaryPages, -1);

	free_page(page, false);
}


void
vm_page_set_state(vm_page *page, int pageState)
{
	ASSERT(page->state != PAGE_STATE_FREE && page->state != PAGE_STATE_CLEAR);

	if (pageState == PAGE_STATE_FREE || pageState == PAGE_STATE_CLEAR)
		free_page(page, pageState == PAGE_STATE_CLEAR);
	else
		set_page_state(page, pageState);
}


/*!	Moves a page to either the tail of the head of its current queue,
	depending on \a tail.
	The page must have a cache and the cache must be locked!
*/
void
vm_page_requeue(struct vm_page *page, bool tail)
{
	ASSERT(page->Cache() != NULL);
	DEBUG_PAGE_ACCESS_CHECK(page);

	VMPageQueue *queue = NULL;

	switch (page->state) {
		case PAGE_STATE_BUSY:
		case PAGE_STATE_ACTIVE:
			queue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			queue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			queue = &sModifiedPageQueue;
			break;
		case PAGE_STATE_FREE:
		case PAGE_STATE_CLEAR:
			panic("vm_page_requeue() called for free/clear page %p", page);
			return;
		case PAGE_STATE_WIRED:
		case PAGE_STATE_UNUSED:
			return;
		default:
			panic("vm_page_touch: vm_page %p in invalid state %d\n",
				page, page->state);
			break;
	}

	queue->RequeueUnlocked(page, tail);
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
	int32 count = sUnreservedFreePages + sInactivePageQueue.Count();
	return count > 0 ? count : 0;
}


size_t
vm_page_num_unused_pages(void)
{
	int32 count = sUnreservedFreePages;
	return count > 0 ? count : 0;
}


void
vm_page_get_stats(system_info *info)
{
	// Get free pages count -- not really exact, since we don't know how many
	// of the reserved pages have already been allocated, but good citizens
	// unreserve chunk-wise as they are allocating the pages, if they have
	// reserved a larger quantity.
	int32 free = sUnreservedFreePages;
	if (free < 0)
		free = 0;

	// The pages used for the block cache buffers. Those should not be counted
	// as used but as cached pages.
	// TODO: We should subtract the blocks that are in use ATM, since those
	// can't really be freed in a low memory situation.
	page_num_t blockCachePages = block_cache_used_memory() / B_PAGE_SIZE;

	info->max_pages = sNumPages;
	info->used_pages = gMappedPagesCount - blockCachePages;
	info->cached_pages = sNumPages >= (uint32)free + info->used_pages
		? sNumPages - free - info->used_pages : 0;
	info->page_faults = vm_num_page_faults();

	// TODO: We don't consider pages used for page directories/tables yet.
}
