/*
 * Copyright 2010-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


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

//#define TRACE_VM_DAEMONS
#ifdef TRACE_VM_DAEMONS
#define TRACE_DAEMON(x...) dprintf(x)
#else
#define TRACE_DAEMON(x...) do {} while (false)
#endif

//#define TRACK_PAGE_USAGE_STATS	1

#define PAGE_ASSERT(page, condition)	\
	ASSERT_PRINT((condition), "page: %p", (page))

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

// Minimum number of free pages the page daemon will try to achieve.
static uint32 sFreePagesTarget;
static uint32 sFreeOrCachedPagesTarget;
static uint32 sInactivePagesTarget;

// Wait interval between page daemon runs.
static const bigtime_t kIdleScanWaitInterval = 1000000LL;	// 1 sec
static const bigtime_t kBusyScanWaitInterval = 500000LL;	// 0.5 sec

// Number of idle runs after which we want to have processed the full active
// queue.
static const uint32 kIdleRunsForFullQueue = 20;

// Maximum limit for the vm_page::usage_count.
static const int32 kPageUsageMax = 64;
// vm_page::usage_count buff an accessed page receives in a scan.
static const int32 kPageUsageAdvance = 3;
// vm_page::usage_count debuff an unaccessed page receives in a scan.
static const int32 kPageUsageDecline = 1;

int32 gMappedPagesCount;

static VMPageQueue sPageQueues[PAGE_STATE_COUNT];

static VMPageQueue& sFreePageQueue = sPageQueues[PAGE_STATE_FREE];
static VMPageQueue& sClearPageQueue = sPageQueues[PAGE_STATE_CLEAR];
static VMPageQueue& sModifiedPageQueue = sPageQueues[PAGE_STATE_MODIFIED];
static VMPageQueue& sInactivePageQueue = sPageQueues[PAGE_STATE_INACTIVE];
static VMPageQueue& sActivePageQueue = sPageQueues[PAGE_STATE_ACTIVE];
static VMPageQueue& sCachedPageQueue = sPageQueues[PAGE_STATE_CACHED];

static vm_page *sPages;
static page_num_t sPhysicalPageOffset;
static page_num_t sNumPages;
static page_num_t sNonExistingPages;
	// pages in the sPages array that aren't backed by physical memory
static uint64 sIgnoredPages;
	// pages of physical memory ignored by the boot loader (and thus not
	// available here)
static vint32 sUnreservedFreePages;
static vint32 sUnsatisfiedPageReservations;
static vint32 sModifiedTemporaryPages;

static ConditionVariable sFreePageCondition;
static mutex sPageDeficitLock = MUTEX_INITIALIZER("page deficit");

// This lock must be used whenever the free or clear page queues are changed.
// If you need to work on both queues at the same time, you need to hold a write
// lock, otherwise, a read lock suffices (each queue still has a spinlock to
// guard against concurrent changes).
static rw_lock sFreePageQueuesLock
	= RW_LOCK_INITIALIZER("free/clear page queues");

#ifdef TRACK_PAGE_USAGE_STATS
static page_num_t sPageUsageArrays[512];
static page_num_t* sPageUsage = sPageUsageArrays;
static page_num_t sPageUsagePageCount;
static page_num_t* sNextPageUsage = sPageUsageArrays + 256;
static page_num_t sNextPageUsagePageCount;
#endif


struct page_stats {
	int32	totalFreePages;
	int32	unsatisfiedReservations;
	int32	cachedPages;
};


struct PageReservationWaiter
		: public DoublyLinkedListLinkImpl<PageReservationWaiter> {
	Thread*	thread;
	uint32	dontTouch;		// reserve not to touch
	uint32	missing;		// pages missing for the reservation
	int32	threadPriority;

	bool operator<(const PageReservationWaiter& other) const
	{
		// Implies an order by descending VM priority (ascending dontTouch)
		// and (secondarily) descending thread priority.
		if (dontTouch != other.dontTouch)
			return dontTouch < other.dontTouch;
		return threadPriority > other.threadPriority;
	}
};

typedef DoublyLinkedList<PageReservationWaiter> PageReservationWaiterList;
static PageReservationWaiterList sPageReservationWaiters;


struct DaemonCondition {
	void Init(const char* name)
	{
		mutex_init(&fLock, "daemon condition");
		fCondition.Init(this, name);
		fActivated = false;
	}

	bool Lock()
	{
		return mutex_lock(&fLock) == B_OK;
	}

	void Unlock()
	{
		mutex_unlock(&fLock);
	}

	bool Wait(bigtime_t timeout, bool clearActivated)
	{
		MutexLocker locker(fLock);
		if (clearActivated)
			fActivated = false;
		else if (fActivated)
			return true;

		ConditionVariableEntry entry;
		fCondition.Add(&entry);

		locker.Unlock();

		return entry.Wait(B_RELATIVE_TIMEOUT, timeout) == B_OK;
	}

	void WakeUp()
	{
		if (fActivated)
			return;

		MutexLocker locker(fLock);
		fActivated = true;
		fCondition.NotifyOne();
	}

	void ClearActivated()
	{
		MutexLocker locker(fLock);
		fActivated = false;
	}

private:
	mutex				fLock;
	ConditionVariable	fCondition;
	bool				fActivated;
};


static DaemonCondition sPageWriterCondition;
static DaemonCondition sPageDaemonCondition;


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


class AllocatePage
	: public TRACE_ENTRY_SELECTOR(PAGE_ALLOCATION_TRACING_STACK_TRACE) {
public:
	AllocatePage()
		:
		TraceEntryBase(PAGE_ALLOCATION_TRACING_STACK_TRACE, 0, true)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("page alloc");
	}
};


class AllocatePageRun
	: public TRACE_ENTRY_SELECTOR(PAGE_ALLOCATION_TRACING_STACK_TRACE) {
public:
	AllocatePageRun(uint32 length)
		:
		TraceEntryBase(PAGE_ALLOCATION_TRACING_STACK_TRACE, 0, true),
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


class FreePage
	: public TRACE_ENTRY_SELECTOR(PAGE_ALLOCATION_TRACING_STACK_TRACE) {
public:
	FreePage()
		:
		TraceEntryBase(PAGE_ALLOCATION_TRACING_STACK_TRACE, 0, true)
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

#	define TA(x)	new(std::nothrow) PageAllocationTracing::x

#else
#	define TA(x)
#endif	// PAGE_ALLOCATION_TRACING


#if PAGE_DAEMON_TRACING

namespace PageDaemonTracing {

class ActivatePage : public AbstractTraceEntry {
	public:
		ActivatePage(vm_page* page)
			:
			fCache(page->cache),
			fPage(page)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page activated:   %p, cache: %p", fPage, fCache);
		}

	private:
		VMCache*	fCache;
		vm_page*	fPage;
};


class DeactivatePage : public AbstractTraceEntry {
	public:
		DeactivatePage(vm_page* page)
			:
			fCache(page->cache),
			fPage(page)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page deactivated: %p, cache: %p", fPage, fCache);
		}

	private:
		VMCache*	fCache;
		vm_page*	fPage;
};


class FreedPageSwap : public AbstractTraceEntry {
	public:
		FreedPageSwap(vm_page* page)
			:
			fCache(page->cache),
			fPage(page)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page swap freed:  %p, cache: %p", fPage, fCache);
		}

	private:
		VMCache*	fCache;
		vm_page*	fPage;
};

}	// namespace PageDaemonTracing

#	define TD(x)	new(std::nothrow) PageDaemonTracing::x

#else
#	define TD(x)
#endif	// PAGE_DAEMON_TRACING


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


#if PAGE_STATE_TRACING

namespace PageStateTracing {

class SetPageState : public AbstractTraceEntry {
	public:
		SetPageState(vm_page* page, uint8 newState)
			:
			fPage(page),
			fOldState(page->State()),
			fNewState(newState),
			fBusy(page->busy),
			fWired(page->WiredCount() > 0),
			fMapped(!page->mappings.IsEmpty()),
			fAccessed(page->accessed),
			fModified(page->modified)
		{
#if PAGE_STATE_TRACING_STACK_TRACE
			fStackTrace = capture_tracing_stack_trace(
				PAGE_STATE_TRACING_STACK_TRACE, 0, true);
				// Don't capture userland stack trace to avoid potential
				// deadlocks.
#endif
			Initialized();
		}

#if PAGE_STATE_TRACING_STACK_TRACE
		virtual void DumpStackTrace(TraceOutput& out)
		{
			out.PrintStackTrace(fStackTrace);
		}
#endif

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("page set state: %p (%c%c%c%c%c): %s -> %s", fPage,
				fBusy ? 'b' : '-',
				fWired ? 'w' : '-',
				fMapped ? 'm' : '-',
				fAccessed ? 'a' : '-',
				fModified ? 'm' : '-',
				page_state_to_string(fOldState),
				page_state_to_string(fNewState));
		}

	private:
		vm_page*	fPage;
#if PAGE_STATE_TRACING_STACK_TRACE
		tracing_stack_trace* fStackTrace;
#endif
		uint8		fOldState;
		uint8		fNewState;
		bool		fBusy : 1;
		bool		fWired : 1;
		bool		fMapped : 1;
		bool		fAccessed : 1;
		bool		fModified : 1;
};

}	// namespace PageStateTracing

#	define TPS(x)	new(std::nothrow) PageStateTracing::x

#else
#	define TPS(x)
#endif	// PAGE_STATE_TRACING


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
		{ "cached",		&sCachedPageQueue },
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
		case PAGE_STATE_MODIFIED:
			return "modified";
		case PAGE_STATE_CACHED:
			return "cached";
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

	uint64 pageAddress = value;
	struct vm_page* page;

	if (addressIsPointer) {
		page = (struct vm_page *)(addr_t)pageAddress;
	} else {
		if (!physical) {
			VMAddressSpace *addressSpace = VMAddressSpace::Kernel();

			if (debug_get_debugged_thread()->team->address_space != NULL)
				addressSpace = debug_get_debugged_thread()->team->address_space;

			uint32 flags = 0;
			phys_addr_t physicalAddress;
			if (addressSpace->TranslationMap()->QueryInterrupt(pageAddress,
					&physicalAddress, &flags) != B_OK
				|| (flags & PAGE_PRESENT) == 0) {
				kprintf("Virtual address not mapped to a physical page in this "
					"address space.\n");
				return 0;
			}
			pageAddress = physicalAddress;
		}

		page = vm_lookup_page(pageAddress / B_PAGE_SIZE);
	}

	kprintf("PAGE: %p\n", page);
	kprintf("queue_next,prev: %p, %p\n", page->queue_link.next,
		page->queue_link.previous);
	kprintf("physical_number: %#" B_PRIxPHYSADDR "\n",
		page->physical_page_number);
	kprintf("cache:           %p\n", page->Cache());
	kprintf("cache_offset:    %" B_PRIuPHYSADDR "\n", page->cache_offset);
	kprintf("cache_next:      %p\n", page->cache_next);
	kprintf("state:           %s\n", page_state_to_string(page->State()));
	kprintf("wired_count:     %d\n", page->WiredCount());
	kprintf("usage_count:     %d\n", page->usage_count);
	kprintf("busy:            %d\n", page->busy);
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
				phys_addr_t physicalAddress;
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

	set_debug_variable("_cache", (addr_t)page->Cache());
	#if DEBUG_PAGE_ACCESS
		set_debug_variable("_accessor", page->accessing_thread);
	#endif

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
	else if (!strcmp(argv[1], "cached"))
		queue = &sCachedPageQueue;
	else {
		kprintf("page_queue: unknown queue \"%s\".\n", argv[1]);
		return 0;
	}

	kprintf("queue = %p, queue->head = %p, queue->tail = %p, queue->count = %"
		B_PRIuPHYSADDR "\n", queue, queue->Head(), queue->Tail(),
		queue->Count());

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
				type, page_state_to_string(page->State()),
				page->WiredCount(), page->usage_count);
		}
	}
	return 0;
}


static int
dump_page_stats(int argc, char **argv)
{
	page_num_t swappableModified = 0;
	page_num_t swappableModifiedInactive = 0;

	size_t counter[8];
	size_t busyCounter[8];
	memset(counter, 0, sizeof(counter));
	memset(busyCounter, 0, sizeof(busyCounter));

	struct page_run {
		page_num_t	start;
		page_num_t	end;

		page_num_t Length() const	{ return end - start; }
	};

	page_run currentFreeRun = { 0, 0 };
	page_run currentCachedRun = { 0, 0 };
	page_run longestFreeRun = { 0, 0 };
	page_run longestCachedRun = { 0, 0 };

	for (page_num_t i = 0; i < sNumPages; i++) {
		if (sPages[i].State() > 7) {
			panic("page %" B_PRIuPHYSADDR " at %p has invalid state!\n", i,
				&sPages[i]);
		}

		uint32 pageState = sPages[i].State();

		counter[pageState]++;
		if (sPages[i].busy)
			busyCounter[pageState]++;

		if (pageState == PAGE_STATE_MODIFIED
			&& sPages[i].Cache() != NULL
			&& sPages[i].Cache()->temporary && sPages[i].WiredCount() == 0) {
			swappableModified++;
			if (sPages[i].usage_count == 0)
				swappableModifiedInactive++;
		}

		// track free and cached pages runs
		if (pageState == PAGE_STATE_FREE || pageState == PAGE_STATE_CLEAR) {
			currentFreeRun.end = i + 1;
			currentCachedRun.end = i + 1;
		} else {
			if (currentFreeRun.Length() > longestFreeRun.Length())
				longestFreeRun = currentFreeRun;
			currentFreeRun.start = currentFreeRun.end = i + 1;

			if (pageState == PAGE_STATE_CACHED) {
				currentCachedRun.end = i + 1;
			} else {
				if (currentCachedRun.Length() > longestCachedRun.Length())
					longestCachedRun = currentCachedRun;
				currentCachedRun.start = currentCachedRun.end = i + 1;
			}
		}
	}

	kprintf("page stats:\n");
	kprintf("total: %" B_PRIuPHYSADDR "\n", sNumPages);

	kprintf("active: %" B_PRIuSIZE " (busy: %" B_PRIuSIZE ")\n",
		counter[PAGE_STATE_ACTIVE], busyCounter[PAGE_STATE_ACTIVE]);
	kprintf("inactive: %" B_PRIuSIZE " (busy: %" B_PRIuSIZE ")\n",
		counter[PAGE_STATE_INACTIVE], busyCounter[PAGE_STATE_INACTIVE]);
	kprintf("cached: %" B_PRIuSIZE " (busy: %" B_PRIuSIZE ")\n",
		counter[PAGE_STATE_CACHED], busyCounter[PAGE_STATE_CACHED]);
	kprintf("unused: %" B_PRIuSIZE " (busy: %" B_PRIuSIZE ")\n",
		counter[PAGE_STATE_UNUSED], busyCounter[PAGE_STATE_UNUSED]);
	kprintf("wired: %" B_PRIuSIZE " (busy: %" B_PRIuSIZE ")\n",
		counter[PAGE_STATE_WIRED], busyCounter[PAGE_STATE_WIRED]);
	kprintf("modified: %" B_PRIuSIZE " (busy: %" B_PRIuSIZE ")\n",
		counter[PAGE_STATE_MODIFIED], busyCounter[PAGE_STATE_MODIFIED]);
	kprintf("free: %" B_PRIuSIZE "\n", counter[PAGE_STATE_FREE]);
	kprintf("clear: %" B_PRIuSIZE "\n", counter[PAGE_STATE_CLEAR]);

	kprintf("unreserved free pages: %" B_PRId32 "\n", sUnreservedFreePages);
	kprintf("unsatisfied page reservations: %" B_PRId32 "\n",
		sUnsatisfiedPageReservations);
	kprintf("mapped pages: %lu\n", gMappedPagesCount);
	kprintf("longest free pages run: %" B_PRIuPHYSADDR " pages (at %"
		B_PRIuPHYSADDR ")\n", longestFreeRun.Length(),
		sPages[longestFreeRun.start].physical_page_number);
	kprintf("longest free/cached pages run: %" B_PRIuPHYSADDR " pages (at %"
		B_PRIuPHYSADDR ")\n", longestCachedRun.Length(),
		sPages[longestCachedRun.start].physical_page_number);

	kprintf("waiting threads:\n");
	for (PageReservationWaiterList::Iterator it
			= sPageReservationWaiters.GetIterator();
		PageReservationWaiter* waiter = it.Next();) {
		kprintf("  %6" B_PRId32 ": missing: %6" B_PRIu32
			", don't touch: %6" B_PRIu32 "\n", waiter->thread->id,
			waiter->missing, waiter->dontTouch);
	}

	kprintf("\nfree queue: %p, count = %" B_PRIuPHYSADDR "\n", &sFreePageQueue,
		sFreePageQueue.Count());
	kprintf("clear queue: %p, count = %" B_PRIuPHYSADDR "\n", &sClearPageQueue,
		sClearPageQueue.Count());
	kprintf("modified queue: %p, count = %" B_PRIuPHYSADDR " (%" B_PRId32
		" temporary, %" B_PRIuPHYSADDR " swappable, " "inactive: %"
		B_PRIuPHYSADDR ")\n", &sModifiedPageQueue, sModifiedPageQueue.Count(),
		sModifiedTemporaryPages, swappableModified, swappableModifiedInactive);
	kprintf("active queue: %p, count = %" B_PRIuPHYSADDR "\n",
		&sActivePageQueue, sActivePageQueue.Count());
	kprintf("inactive queue: %p, count = %" B_PRIuPHYSADDR "\n",
		&sInactivePageQueue, sInactivePageQueue.Count());
	kprintf("cached queue: %p, count = %" B_PRIuPHYSADDR "\n",
		&sCachedPageQueue, sCachedPageQueue.Count());
	return 0;
}


#ifdef TRACK_PAGE_USAGE_STATS

static void
track_page_usage(vm_page* page)
{
	if (page->WiredCount() == 0) {
		sNextPageUsage[(int32)page->usage_count + 128]++;
		sNextPageUsagePageCount++;
	}
}


static void
update_page_usage_stats()
{
	std::swap(sPageUsage, sNextPageUsage);
	sPageUsagePageCount = sNextPageUsagePageCount;

	memset(sNextPageUsage, 0, sizeof(page_num_t) * 256);
	sNextPageUsagePageCount = 0;

	// compute average
	if (sPageUsagePageCount > 0) {
		int64 sum = 0;
		for (int32 i = 0; i < 256; i++)
			sum += (int64)sPageUsage[i] * (i - 128);

		TRACE_DAEMON("average page usage: %f (%lu pages)\n",
			(float)sum / sPageUsagePageCount, sPageUsagePageCount);
	}
}


static int
dump_page_usage_stats(int argc, char** argv)
{
	kprintf("distribution of page usage counts (%lu pages):",
		sPageUsagePageCount);

	int64 sum = 0;
	for (int32 i = 0; i < 256; i++) {
		if (i % 8 == 0)
			kprintf("\n%4ld:", i - 128);

		int64 count = sPageUsage[i];
		sum += count * (i - 128);

		kprintf("  %9llu", count);
	}

	kprintf("\n\n");

	kprintf("average usage count: %f\n",
		sPageUsagePageCount > 0 ? (float)sum / sPageUsagePageCount : 0);

	return 0;
}

#endif	// TRACK_PAGE_USAGE_STATS


// #pragma mark - vm_page


inline void
vm_page::InitState(uint8 newState)
{
	state = newState;
}


inline void
vm_page::SetState(uint8 newState)
{
	TPS(SetPageState(this, newState));

	state = newState;
}


// #pragma mark -


static void
get_page_stats(page_stats& _pageStats)
{
	_pageStats.totalFreePages = sUnreservedFreePages;
	_pageStats.cachedPages = sCachedPageQueue.Count();
	_pageStats.unsatisfiedReservations = sUnsatisfiedPageReservations;
	// TODO: We don't get an actual snapshot here!
}


static bool
do_active_paging(const page_stats& pageStats)
{
	return pageStats.totalFreePages + pageStats.cachedPages
		< pageStats.unsatisfiedReservations
			+ (int32)sFreeOrCachedPagesTarget;
}


/*!	Reserves as many pages as possible from \c sUnreservedFreePages up to
	\a count. Doesn't touch the last \a dontTouch pages of
	\c sUnreservedFreePages, though.
	\return The number of actually reserved pages.
*/
static uint32
reserve_some_pages(uint32 count, uint32 dontTouch)
{
	while (true) {
		int32 freePages = sUnreservedFreePages;
		if (freePages <= (int32)dontTouch)
			return 0;

		int32 toReserve = std::min(count, freePages - dontTouch);
		if (atomic_test_and_set(&sUnreservedFreePages,
					freePages - toReserve, freePages)
				== freePages) {
			return toReserve;
		}

		// the count changed in the meantime -- retry
	}
}


static void
wake_up_page_reservation_waiters()
{
	MutexLocker pageDeficitLocker(sPageDeficitLock);

	// TODO: If this is a low priority thread, we might want to disable
	// interrupts or otherwise ensure that we aren't unscheduled. Otherwise
	// high priority threads wait be kept waiting while a medium priority thread
	// prevents us from running.

	while (PageReservationWaiter* waiter = sPageReservationWaiters.Head()) {
		int32 reserved = reserve_some_pages(waiter->missing,
			waiter->dontTouch);
		if (reserved == 0)
			return;

		atomic_add(&sUnsatisfiedPageReservations, -reserved);
		waiter->missing -= reserved;

		if (waiter->missing > 0)
			return;

		sPageReservationWaiters.Remove(waiter);

		InterruptsSpinLocker schedulerLocker(gSchedulerLock);
		thread_unblock_locked(waiter->thread, B_OK);
	}
}


static inline void
unreserve_pages(uint32 count)
{
	atomic_add(&sUnreservedFreePages, count);
	if (sUnsatisfiedPageReservations != 0)
		wake_up_page_reservation_waiters();
}


static void
free_page(vm_page* page, bool clear)
{
	DEBUG_PAGE_ACCESS_CHECK(page);

	PAGE_ASSERT(page, !page->IsMapped());

	VMPageQueue* fromQueue;

	switch (page->State()) {
		case PAGE_STATE_ACTIVE:
			fromQueue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			fromQueue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			fromQueue = &sModifiedPageQueue;
			break;
		case PAGE_STATE_CACHED:
			fromQueue = &sCachedPageQueue;
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
				page, page->State());
			return;
	}

	if (page->CacheRef() != NULL)
		panic("to be freed page %p has cache", page);
	if (page->IsMapped())
		panic("to be freed page %p has mappings", page);

	if (fromQueue != NULL)
		fromQueue->RemoveUnlocked(page);

	TA(FreePage());

	ReadLocker locker(sFreePageQueuesLock);

	DEBUG_PAGE_ACCESS_END(page);

	if (clear) {
		page->SetState(PAGE_STATE_CLEAR);
		sClearPageQueue.PrependUnlocked(page);
	} else {
		page->SetState(PAGE_STATE_FREE);
		sFreePageQueue.PrependUnlocked(page);
	}

	locker.Unlock();

	unreserve_pages(1);
}


/*!	The caller must make sure that no-one else tries to change the page's state
	while the function is called. If the page has a cache, this can be done by
	locking the cache.
*/
static void
set_page_state(vm_page *page, int pageState)
{
	DEBUG_PAGE_ACCESS_CHECK(page);

	if (pageState == page->State())
		return;

	VMPageQueue* fromQueue;

	switch (page->State()) {
		case PAGE_STATE_ACTIVE:
			fromQueue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			fromQueue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			fromQueue = &sModifiedPageQueue;
			break;
		case PAGE_STATE_CACHED:
			fromQueue = &sCachedPageQueue;
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
				page, page->State());
			return;
	}

	VMPageQueue* toQueue;

	switch (pageState) {
		case PAGE_STATE_ACTIVE:
			toQueue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			toQueue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			toQueue = &sModifiedPageQueue;
			break;
		case PAGE_STATE_CACHED:
			PAGE_ASSERT(page, !page->IsMapped());
			PAGE_ASSERT(page, !page->modified);
			toQueue = &sCachedPageQueue;
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

	VMCache* cache = page->Cache();
	if (cache != NULL && cache->temporary) {
		if (pageState == PAGE_STATE_MODIFIED)
			atomic_add(&sModifiedTemporaryPages, 1);
		else if (page->State() == PAGE_STATE_MODIFIED)
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
		PAGE_ASSERT(page, cache != NULL);
		cache->AssertLocked();
		page->SetState(pageState);
	} else {
		if (fromQueue != NULL)
			fromQueue->RemoveUnlocked(page);

		page->SetState(pageState);

		if (toQueue != NULL)
			toQueue->AppendUnlocked(page);
	}
}


/*! Moves a previously modified page into a now appropriate queue.
	The page queues must not be locked.
*/
static void
move_page_to_appropriate_queue(vm_page *page)
{
	DEBUG_PAGE_ACCESS_CHECK(page);

	// Note, this logic must be in sync with what the page daemon does.
	int32 state;
	if (page->IsMapped())
		state = PAGE_STATE_ACTIVE;
	else if (page->modified)
		state = PAGE_STATE_MODIFIED;
	else
		state = PAGE_STATE_CACHED;

// TODO: If free + cached pages are low, we might directly want to free the
// page.
	set_page_state(page, state);
}


static void
clear_page(struct vm_page *page)
{
	vm_memset_physical(page->physical_page_number << PAGE_SHIFT, 0,
		B_PAGE_SIZE);
}


static status_t
mark_page_range_in_use(page_num_t startPage, page_num_t length, bool wired)
{
	TRACE(("mark_page_range_in_use: start %#" B_PRIxPHYSADDR ", len %#"
		B_PRIxPHYSADDR "\n", startPage, length));

	if (sPhysicalPageOffset > startPage) {
		dprintf("mark_page_range_in_use(%#" B_PRIxPHYSADDR ", %#" B_PRIxPHYSADDR
			"): start page is before free list\n", startPage, length);
		if (sPhysicalPageOffset - startPage >= length)
			return B_OK;
		length -= sPhysicalPageOffset - startPage;
		startPage = sPhysicalPageOffset;
	}

	startPage -= sPhysicalPageOffset;

	if (startPage + length > sNumPages) {
		dprintf("mark_page_range_in_use(%#" B_PRIxPHYSADDR ", %#" B_PRIxPHYSADDR
			"): range would extend past free list\n", startPage, length);
		if (startPage >= sNumPages)
			return B_OK;
		length = sNumPages - startPage;
	}

	WriteLocker locker(sFreePageQueuesLock);

	for (page_num_t i = 0; i < length; i++) {
		vm_page *page = &sPages[startPage + i];
		switch (page->State()) {
			case PAGE_STATE_FREE:
			case PAGE_STATE_CLEAR:
			{
// TODO: This violates the page reservation policy, since we remove pages from
// the free/clear queues without having reserved them before. This should happen
// in the early boot process only, though.
				DEBUG_PAGE_ACCESS_START(page);
				VMPageQueue& queue = page->State() == PAGE_STATE_FREE
					? sFreePageQueue : sClearPageQueue;
				queue.Remove(page);
				page->SetState(wired ? PAGE_STATE_UNUSED : PAGE_STATE_UNUSED);
				page->busy = false;
				atomic_add(&sUnreservedFreePages, -1);
				DEBUG_PAGE_ACCESS_END(page);
				break;
			}
			case PAGE_STATE_WIRED:
			case PAGE_STATE_UNUSED:
				break;
			case PAGE_STATE_ACTIVE:
			case PAGE_STATE_INACTIVE:
			case PAGE_STATE_MODIFIED:
			case PAGE_STATE_CACHED:
			default:
				// uh
				dprintf("mark_page_range_in_use: page %#" B_PRIxPHYSADDR
					" in non-free state %d!\n", startPage + i, page->State());
				break;
		}
	}

	return B_OK;
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

		if (sFreePageQueue.Count() == 0
				|| sUnreservedFreePages < (int32)sFreePagesTarget) {
			continue;
		}

		// Since we temporarily remove pages from the free pages reserve,
		// we must make sure we don't cause a violation of the page
		// reservation warranty. The following is usually stricter than
		// necessary, because we don't have information on how many of the
		// reserved pages have already been allocated.
		int32 reserved = reserve_some_pages(SCRUB_SIZE,
			kPageReserveForPriority[VM_PRIORITY_USER]);
		if (reserved == 0)
			continue;

		// get some pages from the free queue
		ReadLocker locker(sFreePageQueuesLock);

		vm_page *page[SCRUB_SIZE];
		int32 scrubCount = 0;
		for (int32 i = 0; i < reserved; i++) {
			page[i] = sFreePageQueue.RemoveHeadUnlocked();
			if (page[i] == NULL)
				break;

			DEBUG_PAGE_ACCESS_START(page[i]);

			page[i]->SetState(PAGE_STATE_ACTIVE);
			page[i]->busy = true;
			scrubCount++;
		}

		locker.Unlock();

		if (scrubCount == 0) {
			unreserve_pages(reserved);
			continue;
		}

		TA(ScrubbingPages(scrubCount));

		// clear them
		for (int32 i = 0; i < scrubCount; i++)
			clear_page(page[i]);

		locker.Lock();

		// and put them into the clear queue
		for (int32 i = 0; i < scrubCount; i++) {
			page[i]->SetState(PAGE_STATE_CLEAR);
			page[i]->busy = false;
			DEBUG_PAGE_ACCESS_END(page[i]);
			sClearPageQueue.PrependUnlocked(page[i]);
		}

		locker.Unlock();

		unreserve_pages(reserved);

		TA(ScrubbedPages(scrubCount));
	}

	return 0;
}


static void
init_page_marker(vm_page &marker)
{
	marker.SetCacheRef(NULL);
	marker.InitState(PAGE_STATE_UNUSED);
	marker.busy = true;
#if DEBUG_PAGE_QUEUE
	marker.queue = NULL;
#endif
#if DEBUG_PAGE_ACCESS
	marker.accessing_thread = thread_get_current_thread_id();
#endif
}


static void
remove_page_marker(struct vm_page &marker)
{
	DEBUG_PAGE_ACCESS_CHECK(&marker);

	if (marker.State() < PAGE_STATE_FIRST_UNQUEUED)
		sPageQueues[marker.State()].RemoveUnlocked(&marker);

	marker.SetState(PAGE_STATE_UNUSED);
}


static vm_page*
next_modified_page(page_num_t& maxPagesToSee)
{
	InterruptsSpinLocker locker(sModifiedPageQueue.GetLock());

	while (maxPagesToSee > 0) {
		vm_page* page = sModifiedPageQueue.Head();
		if (page == NULL)
			return NULL;

		sModifiedPageQueue.Requeue(page, true);

		maxPagesToSee--;

		if (!page->busy)
			return page;
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
	uint32 Go();

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
		generic_size_t bytesTransferred);
private:
	PageWriterRun*		fRun;
	struct VMCache*		fCache;
	off_t				fOffset;
	uint32				fPageCount;
	int32				fMaxPages;
	status_t			fStatus;
	uint32				fVecCount;
	generic_io_vec		fVecs[32]; // TODO: make dynamic/configurable
};


class PageWriteWrapper {
public:
	PageWriteWrapper();
	~PageWriteWrapper();
	void SetTo(vm_page* page);
	bool Done(status_t result);

private:
	vm_page*			fPage;
	struct VMCache*		fCache;
	bool				fIsActive;
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
PageWriteWrapper::SetTo(vm_page* page)
{
	DEBUG_PAGE_ACCESS_CHECK(page);

	if (page->busy)
		panic("setting page write wrapper to busy page");

	if (fIsActive)
		panic("re-setting page write wrapper that isn't completed");

	fPage = page;
	fCache = page->Cache();
	fIsActive = true;

	fPage->busy = true;
	fPage->busy_writing = true;

	// We have a modified page -- however, while we're writing it back,
	// the page might still be mapped. In order not to lose any changes to the
	// page, we mark it clean before actually writing it back; if
	// writing the page fails for some reason, we'll just keep it in the
	// modified page list, but that should happen only rarely.

	// If the page is changed after we cleared the dirty flag, but before we
	// had the chance to write it back, then we'll write it again later -- that
	// will probably not happen that often, though.

	vm_clear_map_flags(fPage, PAGE_MODIFIED);
}


/*!	The page's cache must be locked.
	The page queues must not be locked.
	\return \c true if the page was written successfully respectively could be
		handled somehow, \c false otherwise.
*/
bool
PageWriteWrapper::Done(status_t result)
{
	if (!fIsActive)
		panic("completing page write wrapper that is not active");

	DEBUG_PAGE_ACCESS_START(fPage);

	fPage->busy = false;
		// Set unbusy and notify later by hand, since we might free the page.

	bool success = true;

	if (result == B_OK) {
		// put it into the active/inactive queue
		move_page_to_appropriate_queue(fPage);
		fPage->busy_writing = false;
		DEBUG_PAGE_ACCESS_END(fPage);
	} else {
		// Writing the page failed. One reason would be that the cache has been
		// shrunk and the page does no longer belong to the file. Otherwise the
		// actual I/O failed, in which case we'll simply keep the page modified.

		if (!fPage->busy_writing) {
			// The busy_writing flag was cleared. That means the cache has been
			// shrunk while we were trying to write the page and we have to free
			// it now.
			vm_remove_all_page_mappings(fPage);
// TODO: Unmapping should already happen when resizing the cache!
			fCache->RemovePage(fPage);
			free_page(fPage, false);
		} else {
			// Writing the page failed -- mark the page modified and move it to
			// an appropriate queue other than the modified queue, so we don't
			// keep trying to write it over and over again. We keep
			// non-temporary pages in the modified queue, though, so they don't
			// get lost in the inactive queue.
			dprintf("PageWriteWrapper: Failed to write page %p: %s\n", fPage,
				strerror(result));

			fPage->modified = true;
			if (!fCache->temporary)
				set_page_state(fPage, PAGE_STATE_MODIFIED);
			else if (fPage->IsMapped())
				set_page_state(fPage, PAGE_STATE_ACTIVE);
			else
				set_page_state(fPage, PAGE_STATE_INACTIVE);

			fPage->busy_writing = false;
			DEBUG_PAGE_ACCESS_END(fPage);

			success = false;
		}
	}

	fCache->NotifyPageEvents(fPage, PAGE_EVENT_NOT_BUSY);
	fIsActive = false;

	return success;
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

	fVecs[0].base = (phys_addr_t)page->physical_page_number << PAGE_SHIFT;
	fVecs[0].length = B_PAGE_SIZE;
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

	phys_addr_t nextBase = fVecs[fVecCount - 1].base
		+ fVecs[fVecCount - 1].length;

	if ((phys_addr_t)page->physical_page_number << PAGE_SHIFT == nextBase
		&& (off_t)page->cache_offset == fOffset + fPageCount) {
		// append to last iovec
		fVecs[fVecCount - 1].length += B_PAGE_SIZE;
		fPageCount++;
		return true;
	}

	nextBase = fVecs[0].base - B_PAGE_SIZE;
	if ((phys_addr_t)page->physical_page_number << PAGE_SHIFT == nextBase
		&& (off_t)page->cache_offset == fOffset - 1) {
		// prepend to first iovec and adjust offset
		fVecs[0].base = nextBase;
		fVecs[0].length += B_PAGE_SIZE;
		fOffset = page->cache_offset;
		fPageCount++;
		return true;
	}

	if (((off_t)page->cache_offset == fOffset + fPageCount
			|| (off_t)page->cache_offset == fOffset - 1)
		&& fVecCount < sizeof(fVecs) / sizeof(fVecs[0])) {
		// not physically contiguous or not in the right order
		uint32 vectorIndex;
		if ((off_t)page->cache_offset < fOffset) {
			// we are pre-pending another vector, move the other vecs
			for (uint32 i = fVecCount; i > 0; i--)
				fVecs[i] = fVecs[i - 1];

			fOffset = page->cache_offset;
			vectorIndex = 0;
		} else
			vectorIndex = fVecCount;

		fVecs[vectorIndex].base
			= (phys_addr_t)page->physical_page_number << PAGE_SHIFT;
		fVecs[vectorIndex].length = B_PAGE_SIZE;

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
	generic_size_t writeLength = (phys_size_t)fPageCount << PAGE_SHIFT;

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
	generic_size_t bytesTransferred)
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
	fWrappers[fWrapperCount++].SetTo(page);

	if (fTransferCount == 0 || !fTransfers[fTransferCount - 1].AddPage(page)) {
		fTransfers[fTransferCount++].SetTo(this, page,
			page->Cache()->MaxPagesPerAsyncWrite());
	}
}


/*!	Writes all pages previously added.
	\return The number of pages that could not be written or otherwise handled.
*/
uint32
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

	uint32 failedPages = 0;
	uint32 wrapperIndex = 0;
	for (uint32 i = 0; i < fTransferCount; i++) {
		PageWriteTransfer& transfer = fTransfers[i];
		transfer.Cache()->Lock();

		for (uint32 j = 0; j < transfer.PageCount(); j++) {
			if (!fWrappers[wrapperIndex++].Done(transfer.Status()))
				failedPages++;
		}

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

	return failedPages;
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

	page_num_t pagesSinceLastSuccessfulWrite = 0;

	while (true) {
// TODO: Maybe wait shorter when memory is low!
		if (sModifiedPageQueue.Count() < kNumPages) {
			sPageWriterCondition.Wait(3000000, true);
				// all 3 seconds when no one triggers us
		}

		page_num_t modifiedPages = sModifiedPageQueue.Count();
		if (modifiedPages == 0)
			continue;

		if (modifiedPages <= pagesSinceLastSuccessfulWrite) {
			// We ran through the whole queue without being able to write a
			// single page. Take a break.
			snooze(500000);
			pagesSinceLastSuccessfulWrite = 0;
		}

#if ENABLE_SWAP_SUPPORT
		page_stats pageStats;
		get_page_stats(pageStats);
		bool activePaging = do_active_paging(pageStats);
#endif

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
		pageCollectionTime -= system_time();

		page_num_t maxPagesToSee = modifiedPages;

		while (numPages < kNumPages && maxPagesToSee > 0) {
			vm_page *page = next_modified_page(maxPagesToSee);
			if (page == NULL)
				break;

			PageCacheLocker cacheLocker(page, false);
			if (!cacheLocker.IsLocked())
				continue;

			VMCache *cache = page->Cache();

			// If the page is busy or its state has changed while we were
			// locking the cache, just ignore it.
			if (page->busy || page->State() != PAGE_STATE_MODIFIED)
				continue;

			DEBUG_PAGE_ACCESS_START(page);

			// Don't write back wired (locked) pages.
			if (page->WiredCount() > 0) {
				set_page_state(page, PAGE_STATE_ACTIVE);
				DEBUG_PAGE_ACCESS_END(page);
				continue;
			}

			// Write back temporary pages only when we're actively paging.
			if (cache->temporary
#if ENABLE_SWAP_SUPPORT
				&& (!activePaging
					|| !cache->CanWritePage(
							(off_t)page->cache_offset << PAGE_SHIFT))
#endif
				) {
				// We can't/don't want to do anything with this page, so move it
				// to one of the other queues.
				if (page->mappings.IsEmpty())
					set_page_state(page, PAGE_STATE_INACTIVE);
				else
					set_page_state(page, PAGE_STATE_ACTIVE);

				DEBUG_PAGE_ACCESS_END(page);
				continue;
			}

			// We need our own reference to the store, as it might currently be
			// destroyed.
			if (cache->AcquireUnreferencedStoreRef() != B_OK) {
				DEBUG_PAGE_ACCESS_END(page);
				cacheLocker.Unlock();
				thread_yield(true);
				continue;
			}

			run.AddPage(page);
				// TODO: We're possibly adding pages of different caches and
				// thus maybe of different underlying file systems here. This
				// is a potential problem for loop file systems/devices, since
				// we could mark a page busy that would need to be accessed
				// when writing back another page, thus causing a deadlock.

			DEBUG_PAGE_ACCESS_END(page);

			//dprintf("write page %p, cache %p (%ld)\n", page, page->cache, page->cache->ref_count);
			TPW(WritePage(page));

			cache->AcquireRefLocked();
			numPages++;
		}

		pageCollectionTime += system_time();

		if (numPages == 0)
			continue;

		// write pages to disk and do all the cleanup
		pageWritingTime -= system_time();
		uint32 failedPages = run.Go();
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

		if (failedPages == numPages)
			pagesSinceLastSuccessfulWrite += modifiedPages - maxPagesToSee;
		else
			pagesSinceLastSuccessfulWrite = 0;
	}

	return B_OK;
}


// #pragma mark -


// TODO: This should be done in the page daemon!
#if 0
#if ENABLE_SWAP_SUPPORT
static bool
free_page_swap_space(int32 index)
{
	vm_page *page = vm_page_at_index(index);
	PageCacheLocker locker(page);
	if (!locker.IsLocked())
		return false;

	DEBUG_PAGE_ACCESS_START(page);

	VMCache* cache = page->Cache();
	if (cache->temporary && page->WiredCount() == 0
			&& cache->HasPage(page->cache_offset << PAGE_SHIFT)
			&& page->usage_count > 0) {
		// TODO: how to judge a page is highly active?
		if (swap_free_page_swap_space(page)) {
			// We need to mark the page modified, since otherwise it could be
			// stolen and we'd lose its data.
			vm_page_set_state(page, PAGE_STATE_MODIFIED);
			TD(FreedPageSwap(page));
			DEBUG_PAGE_ACCESS_END(page);
			return true;
		}
	}
	DEBUG_PAGE_ACCESS_END(page);
	return false;
}
#endif
#endif	// 0


static vm_page *
find_cached_page_candidate(struct vm_page &marker)
{
	DEBUG_PAGE_ACCESS_CHECK(&marker);

	InterruptsSpinLocker locker(sCachedPageQueue.GetLock());
	vm_page *page;

	if (marker.State() == PAGE_STATE_UNUSED) {
		// Get the first free pages of the (in)active queue
		page = sCachedPageQueue.Head();
	} else {
		// Get the next page of the current queue
		if (marker.State() != PAGE_STATE_CACHED) {
			panic("invalid marker %p state", &marker);
			return NULL;
		}

		page = sCachedPageQueue.Next(&marker);
		sCachedPageQueue.Remove(&marker);
		marker.SetState(PAGE_STATE_UNUSED);
	}

	while (page != NULL) {
		if (!page->busy) {
			// we found a candidate, insert marker
			marker.SetState(PAGE_STATE_CACHED);
			sCachedPageQueue.InsertAfter(page, &marker);
			return page;
		}

		page = sCachedPageQueue.Next(page);
	}

	return NULL;
}


static bool
free_cached_page(vm_page *page, bool dontWait)
{
	// try to lock the page's cache
	if (vm_cache_acquire_locked_page_cache(page, dontWait) == NULL)
		return false;
	VMCache* cache = page->Cache();

	AutoLocker<VMCache> cacheLocker(cache, true);
	MethodDeleter<VMCache> _2(cache, &VMCache::ReleaseRefLocked);

	// check again if that page is still a candidate
	if (page->busy || page->State() != PAGE_STATE_CACHED)
		return false;

	DEBUG_PAGE_ACCESS_START(page);

	PAGE_ASSERT(page, !page->IsMapped());
	PAGE_ASSERT(page, !page->modified);

	// we can now steal this page

	cache->RemovePage(page);
		// Now the page doesn't have cache anymore, so no one else (e.g.
		// vm_page_allocate_page_run() can pick it up), since they would be
		// required to lock the cache first, which would fail.

	sCachedPageQueue.RemoveUnlocked(page);
	return true;
}


static uint32
free_cached_pages(uint32 pagesToFree, bool dontWait)
{
	vm_page marker;
	init_page_marker(marker);

	uint32 pagesFreed = 0;

	while (pagesFreed < pagesToFree) {
		vm_page *page = find_cached_page_candidate(marker);
		if (page == NULL)
			break;

		if (free_cached_page(page, dontWait)) {
			ReadLocker locker(sFreePageQueuesLock);
			page->SetState(PAGE_STATE_FREE);
			DEBUG_PAGE_ACCESS_END(page);
			sFreePageQueue.PrependUnlocked(page);
			locker.Unlock();

			TA(StolenPage());

			pagesFreed++;
		}
	}

	remove_page_marker(marker);

	return pagesFreed;
}


static void
idle_scan_active_pages(page_stats& pageStats)
{
	VMPageQueue& queue = sActivePageQueue;

	// We want to scan the whole queue in roughly kIdleRunsForFullQueue runs.
	uint32 maxToScan = queue.Count() / kIdleRunsForFullQueue + 1;

	while (maxToScan > 0) {
		maxToScan--;

		// Get the next page. Note that we don't bother to lock here. We go with
		// the assumption that on all architectures reading/writing pointers is
		// atomic. Beyond that it doesn't really matter. We have to unlock the
		// queue anyway to lock the page's cache, and we'll recheck afterwards.
		vm_page* page = queue.Head();
		if (page == NULL)
			break;

		// lock the page's cache
		VMCache* cache = vm_cache_acquire_locked_page_cache(page, true);
		if (cache == NULL)
			continue;

		if (page->State() != PAGE_STATE_ACTIVE) {
			// page is no longer in the cache or in this queue
			cache->ReleaseRefAndUnlock();
			continue;
		}

		if (page->busy) {
			// page is busy -- requeue at the end
			vm_page_requeue(page, true);
			cache->ReleaseRefAndUnlock();
			continue;
		}

		DEBUG_PAGE_ACCESS_START(page);

		// Get the page active/modified flags and update the page's usage count.
		// We completely unmap inactive temporary pages. This saves us to
		// iterate through the inactive list as well, since we'll be notified
		// via page fault whenever such an inactive page is used again.
		// We don't remove the mappings of non-temporary pages, since we
		// wouldn't notice when those would become unused and could thus be
		// moved to the cached list.
		int32 usageCount;
		if (page->WiredCount() > 0 || page->usage_count > 0
			|| !cache->temporary) {
			usageCount = vm_clear_page_mapping_accessed_flags(page);
		} else
			usageCount = vm_remove_all_page_mappings_if_unaccessed(page);

		if (usageCount > 0) {
			usageCount += page->usage_count + kPageUsageAdvance;
			if (usageCount > kPageUsageMax)
				usageCount = kPageUsageMax;
// TODO: This would probably also be the place to reclaim swap space.
		} else {
			usageCount += page->usage_count - (int32)kPageUsageDecline;
			if (usageCount < 0) {
				usageCount = 0;
				set_page_state(page, PAGE_STATE_INACTIVE);
			}
		}

		page->usage_count = usageCount;

		DEBUG_PAGE_ACCESS_END(page);

		cache->ReleaseRefAndUnlock();
	}
}


static void
full_scan_inactive_pages(page_stats& pageStats, int32 despairLevel)
{
	int32 pagesToFree = pageStats.unsatisfiedReservations
		+ sFreeOrCachedPagesTarget
		- (pageStats.totalFreePages + pageStats.cachedPages);
	if (pagesToFree <= 0)
		return;

	bigtime_t time = system_time();
	uint32 pagesScanned = 0;
	uint32 pagesToCached = 0;
	uint32 pagesToModified = 0;
	uint32 pagesToActive = 0;

	// Determine how many pages at maximum to send to the modified queue. Since
	// it is relatively expensive to page out pages, we do that on a grander
	// scale only when things get desperate.
	uint32 maxToFlush = despairLevel <= 1 ? 32 : 10000;

	vm_page marker;
	init_page_marker(marker);

	VMPageQueue& queue = sInactivePageQueue;
	InterruptsSpinLocker queueLocker(queue.GetLock());
	uint32 maxToScan = queue.Count();

	vm_page* nextPage = queue.Head();

	while (pagesToFree > 0 && maxToScan > 0) {
		maxToScan--;

		// get the next page
		vm_page* page = nextPage;
		if (page == NULL)
			break;
		nextPage = queue.Next(page);

		if (page->busy)
			continue;

		// mark the position
		queue.InsertAfter(page, &marker);
		queueLocker.Unlock();

		// lock the page's cache
		VMCache* cache = vm_cache_acquire_locked_page_cache(page, true);
		if (cache == NULL || page->busy
				|| page->State() != PAGE_STATE_INACTIVE) {
			if (cache != NULL)
				cache->ReleaseRefAndUnlock();
			queueLocker.Lock();
			nextPage = queue.Next(&marker);
			queue.Remove(&marker);
			continue;
		}

		pagesScanned++;

		DEBUG_PAGE_ACCESS_START(page);

		// Get the accessed count, clear the accessed/modified flags and
		// unmap the page, if it hasn't been accessed.
		int32 usageCount;
		if (page->WiredCount() > 0)
			usageCount = vm_clear_page_mapping_accessed_flags(page);
		else
			usageCount = vm_remove_all_page_mappings_if_unaccessed(page);

		// update usage count
		if (usageCount > 0) {
			usageCount += page->usage_count + kPageUsageAdvance;
			if (usageCount > kPageUsageMax)
				usageCount = kPageUsageMax;
		} else {
			usageCount += page->usage_count - (int32)kPageUsageDecline;
			if (usageCount < 0)
				usageCount = 0;
		}

		page->usage_count = usageCount;

		// Move to fitting queue or requeue:
		// * Active mapped pages go to the active queue.
		// * Inactive mapped (i.e. wired) pages are requeued.
		// * The remaining pages are cachable. Thus, if unmodified they go to
		//   the cached queue, otherwise to the modified queue (up to a limit).
		//   Note that until in the idle scanning we don't exempt pages of
		//   temporary caches. Apparently we really need memory, so we better
		//   page out memory as well.
		bool isMapped = page->IsMapped();
		if (usageCount > 0) {
			if (isMapped) {
				set_page_state(page, PAGE_STATE_ACTIVE);
				pagesToActive++;
			} else
				vm_page_requeue(page, true);
		} else if (isMapped) {
			vm_page_requeue(page, true);
		} else if (!page->modified) {
			set_page_state(page, PAGE_STATE_CACHED);
			pagesToFree--;
			pagesToCached++;
		} else if (maxToFlush > 0) {
			set_page_state(page, PAGE_STATE_MODIFIED);
			maxToFlush--;
			pagesToModified++;
		} else
			vm_page_requeue(page, true);

		DEBUG_PAGE_ACCESS_END(page);

		cache->ReleaseRefAndUnlock();

		// remove the marker
		queueLocker.Lock();
		nextPage = queue.Next(&marker);
		queue.Remove(&marker);
	}

	queueLocker.Unlock();

	time = system_time() - time;
	TRACE_DAEMON("  -> inactive scan (%7lld us): scanned: %7lu, "
		"moved: %lu -> cached, %lu -> modified, %lu -> active\n", time,
		pagesScanned, pagesToCached, pagesToModified, pagesToActive);

	// wake up the page writer, if we tossed it some pages
	if (pagesToModified > 0)
		sPageWriterCondition.WakeUp();
}


static void
full_scan_active_pages(page_stats& pageStats, int32 despairLevel)
{
	vm_page marker;
	init_page_marker(marker);

	VMPageQueue& queue = sActivePageQueue;
	InterruptsSpinLocker queueLocker(queue.GetLock());
	uint32 maxToScan = queue.Count();

	int32 pagesToDeactivate = pageStats.unsatisfiedReservations
		+ sFreeOrCachedPagesTarget
		- (pageStats.totalFreePages + pageStats.cachedPages)
		+ std::max((int32)sInactivePagesTarget - (int32)maxToScan, (int32)0);
	if (pagesToDeactivate <= 0)
		return;

	bigtime_t time = system_time();
	uint32 pagesAccessed = 0;
	uint32 pagesToInactive = 0;
	uint32 pagesScanned = 0;

	vm_page* nextPage = queue.Head();

	while (pagesToDeactivate > 0 && maxToScan > 0) {
		maxToScan--;

		// get the next page
		vm_page* page = nextPage;
		if (page == NULL)
			break;
		nextPage = queue.Next(page);

		if (page->busy)
			continue;

		// mark the position
		queue.InsertAfter(page, &marker);
		queueLocker.Unlock();

		// lock the page's cache
		VMCache* cache = vm_cache_acquire_locked_page_cache(page, true);
		if (cache == NULL || page->busy || page->State() != PAGE_STATE_ACTIVE) {
			if (cache != NULL)
				cache->ReleaseRefAndUnlock();
			queueLocker.Lock();
			nextPage = queue.Next(&marker);
			queue.Remove(&marker);
			continue;
		}

		pagesScanned++;

		DEBUG_PAGE_ACCESS_START(page);

		// Get the page active/modified flags and update the page's usage count.
		int32 usageCount = vm_clear_page_mapping_accessed_flags(page);

		if (usageCount > 0) {
			usageCount += page->usage_count + kPageUsageAdvance;
			if (usageCount > kPageUsageMax)
				usageCount = kPageUsageMax;
			pagesAccessed++;
// TODO: This would probably also be the place to reclaim swap space.
		} else {
			usageCount += page->usage_count - (int32)kPageUsageDecline;
			if (usageCount <= 0) {
				usageCount = 0;
				set_page_state(page, PAGE_STATE_INACTIVE);
				pagesToInactive++;
			}
		}

		page->usage_count = usageCount;

		DEBUG_PAGE_ACCESS_END(page);

		cache->ReleaseRefAndUnlock();

		// remove the marker
		queueLocker.Lock();
		nextPage = queue.Next(&marker);
		queue.Remove(&marker);
	}

	time = system_time() - time;
	TRACE_DAEMON("  ->   active scan (%7lld us): scanned: %7lu, "
		"moved: %lu -> inactive, encountered %lu accessed ones\n", time,
		pagesScanned, pagesToInactive, pagesAccessed);
}


static void
page_daemon_idle_scan(page_stats& pageStats)
{
	TRACE_DAEMON("page daemon: idle run\n");

	if (pageStats.totalFreePages < (int32)sFreePagesTarget) {
		// We want more actually free pages, so free some from the cached
		// ones.
		uint32 freed = free_cached_pages(
			sFreePagesTarget - pageStats.totalFreePages, false);
		if (freed > 0)
			unreserve_pages(freed);
		get_page_stats(pageStats);
	}

	// Walk the active list and move pages to the inactive queue.
	get_page_stats(pageStats);
	idle_scan_active_pages(pageStats);
}


static void
page_daemon_full_scan(page_stats& pageStats, int32 despairLevel)
{
	TRACE_DAEMON("page daemon: full run: free: %lu, cached: %lu, "
		"to free: %lu\n", pageStats.totalFreePages, pageStats.cachedPages,
		pageStats.unsatisfiedReservations + sFreeOrCachedPagesTarget
			- (pageStats.totalFreePages + pageStats.cachedPages));

	// Walk the inactive list and transfer pages to the cached and modified
	// queues.
	full_scan_inactive_pages(pageStats, despairLevel);

	// Free cached pages. Also wake up reservation waiters.
	get_page_stats(pageStats);
	int32 pagesToFree = pageStats.unsatisfiedReservations + sFreePagesTarget
		- (pageStats.totalFreePages);
	if (pagesToFree > 0) {
		uint32 freed = free_cached_pages(pagesToFree, true);
		if (freed > 0)
			unreserve_pages(freed);
	}

	// Walk the active list and move pages to the inactive queue.
	get_page_stats(pageStats);
	full_scan_active_pages(pageStats, despairLevel);
}


static status_t
page_daemon(void* /*unused*/)
{
	int32 despairLevel = 0;

	while (true) {
		sPageDaemonCondition.ClearActivated();

		// evaluate the free pages situation
		page_stats pageStats;
		get_page_stats(pageStats);

		if (!do_active_paging(pageStats)) {
			// Things look good -- just maintain statistics and keep the pool
			// of actually free pages full enough.
			despairLevel = 0;
			page_daemon_idle_scan(pageStats);
			sPageDaemonCondition.Wait(kIdleScanWaitInterval, false);
		} else {
			// Not enough free pages. We need to do some real work.
			despairLevel = std::max(despairLevel + 1, (int32)3);
			page_daemon_full_scan(pageStats, despairLevel);

			// Don't wait after the first full scan, but rather immediately
			// check whether we were successful in freeing enough pages and
			// re-run with increased despair level. The first scan is
			// conservative with respect to moving inactive modified pages to
			// the modified list to avoid thrashing. The second scan, however,
			// will not hold back.
			if (despairLevel > 1)
				snooze(kBusyScanWaitInterval);
		}
	}

	return B_OK;
}


/*!	Returns how many pages could *not* be reserved.
*/
static uint32
reserve_pages(uint32 count, int priority, bool dontWait)
{
	int32 dontTouch = kPageReserveForPriority[priority];

	while (true) {
		count -= reserve_some_pages(count, dontTouch);
		if (count == 0)
			return 0;

		if (sUnsatisfiedPageReservations == 0) {
			count -= free_cached_pages(count, dontWait);
			if (count == 0)
				return count;
		}

		if (dontWait)
			return count;

		// we need to wait for pages to become available

		MutexLocker pageDeficitLocker(sPageDeficitLock);

		bool notifyDaemon = sUnsatisfiedPageReservations == 0;
		sUnsatisfiedPageReservations += count;

		if (sUnreservedFreePages > dontTouch) {
			// the situation changed
			sUnsatisfiedPageReservations -= count;
			continue;
		}

		PageReservationWaiter waiter;
		waiter.dontTouch = dontTouch;
		waiter.missing = count;
		waiter.thread = thread_get_current_thread();
		waiter.threadPriority = waiter.thread->priority;

		// insert ordered (i.e. after all waiters with higher or equal priority)
		PageReservationWaiter* otherWaiter = NULL;
		for (PageReservationWaiterList::Iterator it
				= sPageReservationWaiters.GetIterator();
			(otherWaiter = it.Next()) != NULL;) {
			if (waiter < *otherWaiter)
				break;
		}

		sPageReservationWaiters.InsertBefore(otherWaiter, &waiter);

		thread_prepare_to_block(waiter.thread, 0, THREAD_BLOCK_TYPE_OTHER,
			"waiting for pages");

		if (notifyDaemon)
			sPageDaemonCondition.WakeUp();

		pageDeficitLocker.Unlock();

		low_resource(B_KERNEL_RESOURCE_PAGES, count, B_RELATIVE_TIMEOUT, 0);
		thread_block();

		pageDeficitLocker.Lock();

		return 0;
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

		if (page != NULL) {
			if (page->busy
				|| (page->State() != PAGE_STATE_MODIFIED
					&& !vm_test_map_modification(page))) {
				page = NULL;
			}
		}

		PageWriteWrapper* wrapper = NULL;
		if (page != NULL) {
			wrapper = &wrapperPool[nextWrapper++];
			if (nextWrapper > maxPages)
				nextWrapper = 0;

			DEBUG_PAGE_ACCESS_START(page);

			wrapper->SetTo(page);

			if (transferEmpty || transfer.AddPage(page)) {
				if (transferEmpty) {
					transfer.SetTo(NULL, page, maxPages);
					transferEmpty = false;
				}

				DEBUG_PAGE_ACCESS_END(page);

				wrappers[usedWrappers++] = wrapper;
				continue;
			}

			DEBUG_PAGE_ACCESS_END(page);
		}

		if (transferEmpty)
			continue;

		cache->Unlock();
		status_t status = transfer.Schedule(0);
		cache->Lock();

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
	PAGE_ASSERT(page, page->State() == PAGE_STATE_MODIFIED);

	vm_page_requeue(page, false);

	sPageWriterCondition.WakeUp();
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

		if (!page->busy && page->State() == PAGE_STATE_MODIFIED) {
			DEBUG_PAGE_ACCESS_START(page);
			vm_page_requeue(page, false);
			modified++;
			DEBUG_PAGE_ACCESS_END(page);
		}
	}

	if (modified > 0)
		sPageWriterCondition.WakeUp();
}


void
vm_page_init_num_pages(kernel_args *args)
{
	// calculate the size of memory by looking at the physical_memory_range array
	sPhysicalPageOffset = args->physical_memory_range[0].start / B_PAGE_SIZE;
	page_num_t physicalPagesEnd = sPhysicalPageOffset
		+ args->physical_memory_range[0].size / B_PAGE_SIZE;

	sNonExistingPages = 0;
	sIgnoredPages = args->ignored_physical_memory / B_PAGE_SIZE;

	for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
		page_num_t start = args->physical_memory_range[i].start / B_PAGE_SIZE;
		if (start > physicalPagesEnd)
			sNonExistingPages += start - physicalPagesEnd;
		physicalPagesEnd = start
			+ args->physical_memory_range[i].size / B_PAGE_SIZE;
	}

	TRACE(("first phys page = %#" B_PRIxPHYSADDR ", end %#" B_PRIxPHYSADDR "\n",
		sPhysicalPageOffset, physicalPagesEnd));

	sNumPages = physicalPagesEnd - sPhysicalPageOffset;

#ifdef LIMIT_AVAILABLE_MEMORY
	if (sNumPages > LIMIT_AVAILABLE_MEMORY * (1024 * 1024 / B_PAGE_SIZE))
		sNumPages = LIMIT_AVAILABLE_MEMORY * (1024 * 1024 / B_PAGE_SIZE);
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
	sCachedPageQueue.Init("cached pages queue");
	sFreePageQueue.Init("free pages queue");
	sClearPageQueue.Init("clear pages queue");

	new (&sPageReservationWaiters) PageReservationWaiterList;

	// map in the new free page table
	sPages = (vm_page *)vm_allocate_early(args, sNumPages * sizeof(vm_page),
		~0L, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0);

	TRACE(("vm_init: putting free_page_table @ %p, # ents %" B_PRIuPHYSADDR
		" (size %#" B_PRIxPHYSADDR ")\n", sPages, sNumPages,
		(phys_addr_t)(sNumPages * sizeof(vm_page))));

	// initialize the free page table
	for (uint32 i = 0; i < sNumPages; i++) {
		sPages[i].Init(sPhysicalPageOffset + i);
		sFreePageQueue.Append(&sPages[i]);
	}

	sUnreservedFreePages = sNumPages;

	TRACE(("initialized table\n"));

	// mark the ranges between usable physical memory unused
	phys_addr_t previousEnd = 0;
	for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
		phys_addr_t base = args->physical_memory_range[i].start;
		phys_size_t size = args->physical_memory_range[i].size;
		if (base > previousEnd) {
			mark_page_range_in_use(previousEnd / B_PAGE_SIZE,
				(base - previousEnd) / B_PAGE_SIZE, false);
		}
		previousEnd = base + size;
	}

	// mark the allocated physical page ranges wired
	for (uint32 i = 0; i < args->num_physical_allocated_ranges; i++) {
		mark_page_range_in_use(
			args->physical_allocated_range[i].start / B_PAGE_SIZE,
			args->physical_allocated_range[i].size / B_PAGE_SIZE, true);
	}

	// The target of actually free pages. This must be at least the system
	// reserve, but should be a few more pages, so we don't have to extract
	// a cached page with each allocation.
	sFreePagesTarget = VM_PAGE_RESERVE_USER
		+ std::max((page_num_t)32, (sNumPages - sNonExistingPages) / 1024);

	// The target of free + cached and inactive pages. On low-memory machines
	// keep things tight. free + cached is the pool of immediately allocatable
	// pages. We want a few inactive pages, so when we're actually paging, we
	// have a reasonably large set of pages to work with.
	if (sUnreservedFreePages < 16 * 1024) {
		sFreeOrCachedPagesTarget = sFreePagesTarget + 128;
		sInactivePagesTarget = sFreePagesTarget / 3;
	} else {
		sFreeOrCachedPagesTarget = 2 * sFreePagesTarget;
		sInactivePagesTarget = sFreePagesTarget / 2;
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

#ifdef TRACK_PAGE_USAGE_STATS
	add_debugger_command_etc("page_usage", &dump_page_usage_stats,
		"Dumps statistics about page usage counts",
		"\n"
		"Dumps statistics about page usage counts.\n",
		B_KDEBUG_DONT_PARSE_ARGUMENTS);
#endif

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
	resume_thread(thread);

	// start page writer

	sPageWriterCondition.Init("page writer");

	thread = spawn_kernel_thread(&page_writer, "page writer",
		B_NORMAL_PRIORITY + 1, NULL);
	resume_thread(thread);

	// start page daemon

	sPageDaemonCondition.Init("page daemon");

	thread = spawn_kernel_thread(&page_daemon, "page daemon",
		B_NORMAL_PRIORITY, NULL);
	resume_thread(thread);

	return B_OK;
}


status_t
vm_mark_page_inuse(page_num_t page)
{
	return vm_mark_page_range_inuse(page, 1);
}


status_t
vm_mark_page_range_inuse(page_num_t startPage, page_num_t length)
{
	return mark_page_range_in_use(startPage, length, false);
}


/*!	Unreserve pages previously reserved with vm_page_reserve_pages().
*/
void
vm_page_unreserve_pages(vm_page_reservation* reservation)
{
	uint32 count = reservation->count;
	reservation->count = 0;

	if (count == 0)
		return;

	TA(UnreservePages(count));

	unreserve_pages(count);
}


/*!	With this call, you can reserve a number of free pages in the system.
	They will only be handed out to someone who has actually reserved them.
	This call returns as soon as the number of requested pages has been
	reached.
	The caller must not hold any cache lock or the function might deadlock.
*/
void
vm_page_reserve_pages(vm_page_reservation* reservation, uint32 count,
	int priority)
{
	reservation->count = count;

	if (count == 0)
		return;

	TA(ReservePages(count));

	reserve_pages(count, priority, false);
}


bool
vm_page_try_reserve_pages(vm_page_reservation* reservation, uint32 count,
	int priority)
{
	if (count == 0) {
		reservation->count = count;
		return true;
	}

	uint32 remaining = reserve_pages(count, priority, true);
	if (remaining == 0) {
		TA(ReservePages(count));
		reservation->count = count;
		return true;
	}

	unreserve_pages(count - remaining);

	return false;
}


vm_page *
vm_page_allocate_page(vm_page_reservation* reservation, uint32 flags)
{
	uint32 pageState = flags & VM_PAGE_ALLOC_STATE;
	ASSERT(pageState != PAGE_STATE_FREE);
	ASSERT(pageState != PAGE_STATE_CLEAR);

	ASSERT(reservation->count > 0);
	reservation->count--;

	VMPageQueue* queue;
	VMPageQueue* otherQueue;

	if ((flags & VM_PAGE_ALLOC_CLEAR) != 0) {
		queue = &sClearPageQueue;
		otherQueue = &sFreePageQueue;
	} else {
		queue = &sFreePageQueue;
		otherQueue = &sClearPageQueue;
	}

	TA(AllocatePage());

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

	int oldPageState = page->State();
	page->SetState(pageState);
	page->busy = (flags & VM_PAGE_ALLOC_BUSY) != 0;
	page->usage_count = 0;
	page->accessed = false;
	page->modified = false;

	locker.Unlock();

	if (pageState < PAGE_STATE_FIRST_UNQUEUED)
		sPageQueues[pageState].AppendUnlocked(page);

	// clear the page, if we had to take it from the free queue and a clear
	// page was requested
	if ((flags & VM_PAGE_ALLOC_CLEAR) != 0 && oldPageState != PAGE_STATE_CLEAR)
		clear_page(page);

	return page;
}


static void
allocate_page_run_cleanup(VMPageQueue::PageList& freePages,
	VMPageQueue::PageList& clearPages)
{
	while (vm_page* page = freePages.RemoveHead()) {
		page->busy = false;
		page->SetState(PAGE_STATE_FREE);
		DEBUG_PAGE_ACCESS_END(page);
		sFreePageQueue.PrependUnlocked(page);
	}

	while (vm_page* page = clearPages.RemoveHead()) {
		page->busy = false;
		page->SetState(PAGE_STATE_CLEAR);
		DEBUG_PAGE_ACCESS_END(page);
		sClearPageQueue.PrependUnlocked(page);
	}
}


/*!	Tries to allocate the a contiguous run of \a length pages starting at
	index \a start.

	The caller must have write-locked the free/clear page queues. The function
	will unlock regardless of whether it succeeds or fails.

	If the function fails, it cleans up after itself, i.e. it will free all
	pages it managed to allocate.

	\param start The start index (into \c sPages) of the run.
	\param length The number of pages to allocate.
	\param flags Page allocation flags. Encodes the state the function shall
		set the allocated pages to, whether the pages shall be marked busy
		(VM_PAGE_ALLOC_BUSY), and whether the pages shall be cleared
		(VM_PAGE_ALLOC_CLEAR).
	\param freeClearQueueLocker Locked WriteLocker for the free/clear page
		queues in locked state. Will be unlocked by the function.
	\return The index of the first page that could not be allocated. \a length
		is returned when the function was successful.
*/
static page_num_t
allocate_page_run(page_num_t start, page_num_t length, uint32 flags,
	WriteLocker& freeClearQueueLocker)
{
	uint32 pageState = flags & VM_PAGE_ALLOC_STATE;
	ASSERT(pageState != PAGE_STATE_FREE);
	ASSERT(pageState != PAGE_STATE_CLEAR);
	ASSERT(start + length <= sNumPages);

	TA(AllocatePageRun(length));

	// Pull the free/clear pages out of their respective queues. Cached pages
	// are allocated later.
	page_num_t cachedPages = 0;
	VMPageQueue::PageList freePages;
	VMPageQueue::PageList clearPages;
	page_num_t i = 0;
	for (; i < length; i++) {
		bool pageAllocated = true;
		bool noPage = false;
		vm_page& page = sPages[start + i];
		switch (page.State()) {
			case PAGE_STATE_CLEAR:
				DEBUG_PAGE_ACCESS_START(&page);
				sClearPageQueue.Remove(&page);
				clearPages.Add(&page);
				break;
			case PAGE_STATE_FREE:
				DEBUG_PAGE_ACCESS_START(&page);
				sFreePageQueue.Remove(&page);
				freePages.Add(&page);
				break;
			case PAGE_STATE_CACHED:
				// We allocate cached pages later.
				cachedPages++;
				pageAllocated = false;
				break;

			default:
				// Probably a page was cached when our caller checked. Now it's
				// gone and we have to abort.
				noPage = true;
				break;
		}

		if (noPage)
			break;

		if (pageAllocated) {
			page.SetState(flags & VM_PAGE_ALLOC_STATE);
			page.busy = (flags & VM_PAGE_ALLOC_BUSY) != 0;
			page.usage_count = 0;
			page.accessed = false;
			page.modified = false;
		}
	}

	if (i < length) {
		// failed to allocate a page -- free all that we've got
		allocate_page_run_cleanup(freePages, clearPages);
		return i;
	}

	freeClearQueueLocker.Unlock();

	if (cachedPages > 0) {
		// allocate the pages that weren't free but cached
		page_num_t freedCachedPages = 0;
		page_num_t nextIndex = start;
		vm_page* freePage = freePages.Head();
		vm_page* clearPage = clearPages.Head();
		while (cachedPages > 0) {
			// skip, if we've already got the page
			if (freePage != NULL && size_t(freePage - sPages) == nextIndex) {
				freePage = freePages.GetNext(freePage);
				nextIndex++;
				continue;
			}
			if (clearPage != NULL && size_t(clearPage - sPages) == nextIndex) {
				clearPage = clearPages.GetNext(clearPage);
				nextIndex++;
				continue;
			}

			// free the page, if it is still cached
			vm_page& page = sPages[nextIndex];
			if (!free_cached_page(&page, false)) {
				// TODO: if the page turns out to have been freed already,
				// there would be no need to fail
				break;
			}

			page.SetState(flags & VM_PAGE_ALLOC_STATE);
			page.busy = (flags & VM_PAGE_ALLOC_BUSY) != 0;
			page.usage_count = 0;
			page.accessed = false;
			page.modified = false;

			freePages.InsertBefore(freePage, &page);
			freedCachedPages++;
			cachedPages--;
			nextIndex++;
		}

		// If we have freed cached pages, we need to balance things.
		if (freedCachedPages > 0)
			unreserve_pages(freedCachedPages);

		if (nextIndex - start < length) {
			// failed to allocate all cached pages -- free all that we've got
			freeClearQueueLocker.Lock();
			allocate_page_run_cleanup(freePages, clearPages);
			freeClearQueueLocker.Unlock();

			return nextIndex - start;
		}
	}

	// clear pages, if requested
	if ((flags & VM_PAGE_ALLOC_CLEAR) != 0) {
		for (VMPageQueue::PageList::Iterator it = freePages.GetIterator();
				vm_page* page = it.Next();) {
 			clear_page(page);
		}
	}

	// add pages to target queue
	if (pageState < PAGE_STATE_FIRST_UNQUEUED) {
		freePages.MoveFrom(&clearPages);
		sPageQueues[pageState].AppendUnlocked(freePages, length);
	}

	// Note: We don't unreserve the pages since we pulled them out of the
	// free/clear queues without adjusting sUnreservedFreePages.

	return length;
}


/*! Allocate a physically contiguous range of pages.

	\param flags Page allocation flags. Encodes the state the function shall
		set the allocated pages to, whether the pages shall be marked busy
		(VM_PAGE_ALLOC_BUSY), and whether the pages shall be cleared
		(VM_PAGE_ALLOC_CLEAR).
	\param length The number of contiguous pages to allocate.
	\param restrictions Restrictions to the physical addresses of the page run
		to allocate, including \c low_address, the first acceptable physical
		address where the page run may start, \c high_address, the last
		acceptable physical address where the page run may end (i.e. it must
		hold \code runStartAddress + length <= high_address \endcode),
		\c alignment, the alignment of the page run start address, and
		\c boundary, multiples of which the page run must not cross.
		Values set to \c 0 are ignored.
	\param priority The page reservation priority (as passed to
		vm_page_reserve_pages()).
	\return The first page of the allocated page run on success; \c NULL
		when the allocation failed.
*/
vm_page*
vm_page_allocate_page_run(uint32 flags, page_num_t length,
	const physical_address_restrictions* restrictions, int priority)
{
	// compute start and end page index
	page_num_t requestedStart
		= std::max(restrictions->low_address / B_PAGE_SIZE, sPhysicalPageOffset)
			- sPhysicalPageOffset;
	page_num_t start = requestedStart;
	page_num_t end;
	if (restrictions->high_address > 0) {
		end = std::max(restrictions->high_address / B_PAGE_SIZE,
				sPhysicalPageOffset)
			- sPhysicalPageOffset;
		end = std::min(end, sNumPages);
	} else
		end = sNumPages;

	// compute alignment mask
	page_num_t alignmentMask
		= std::max(restrictions->alignment / B_PAGE_SIZE, (phys_addr_t)1) - 1;
	ASSERT(((alignmentMask + 1) & alignmentMask) == 0);
		// alignment must be a power of 2

	// compute the boundary shift
	uint32 boundaryShift = 0;
	if (restrictions->boundary != 0) {
		page_num_t boundary = restrictions->boundary / B_PAGE_SIZE;
		// boundary must be a power of two and not less than alignment and
		// length
		ASSERT(((boundary - 1) & boundary) == 0);
		ASSERT(boundary >= alignmentMask + 1);
		ASSERT(boundary >= length);

		while ((boundary >>= 1) > 0)
			boundaryShift++;
	}

	vm_page_reservation reservation;
	vm_page_reserve_pages(&reservation, length, priority);

	WriteLocker freeClearQueueLocker(sFreePageQueuesLock);

	// First we try to get a run with free pages only. If that fails, we also
	// consider cached pages. If there are only few free pages and many cached
	// ones, the odds are that we won't find enough contiguous ones, so we skip
	// the first iteration in this case.
	int32 freePages = sUnreservedFreePages;
	int useCached = freePages > 0 && (page_num_t)freePages > 2 * length ? 0 : 1;

	for (;;) {
		if (alignmentMask != 0 || boundaryShift != 0) {
			page_num_t offsetStart = start + sPhysicalPageOffset;

			// enforce alignment
			if ((offsetStart & alignmentMask) != 0) {
				offsetStart = ((offsetStart + alignmentMask) & ~alignmentMask)
					- sPhysicalPageOffset;
			}

			// enforce boundary
			if (offsetStart << boundaryShift
					!= (offsetStart + length - 1) << boundaryShift) {
				offsetStart = (offsetStart + length - 1) << boundaryShift
					>> boundaryShift;
			}

			start = offsetStart - sPhysicalPageOffset;
		}

		if (start + length > end) {
			if (useCached == 0) {
				// The first iteration with free pages only was unsuccessful.
				// Try again also considering cached pages.
				useCached = 1;
				start = requestedStart;
				continue;
			}

			dprintf("vm_page_allocate_page_run(): Failed to allocate run of "
				"length %" B_PRIuPHYSADDR " in second iteration!", length);

			freeClearQueueLocker.Unlock();
			vm_page_unreserve_pages(&reservation);
			return NULL;
		}

		bool foundRun = true;
		page_num_t i;
		for (i = 0; i < length; i++) {
			uint32 pageState = sPages[start + i].State();
			if (pageState != PAGE_STATE_FREE
				&& pageState != PAGE_STATE_CLEAR
				&& (pageState != PAGE_STATE_CACHED || useCached == 0)) {
				foundRun = false;
				break;
			}
		}

		if (foundRun) {
			i = allocate_page_run(start, length, flags, freeClearQueueLocker);
			if (i == length)
				return &sPages[start];

			// apparently a cached page couldn't be allocated -- skip it and
			// continue
			freeClearQueueLocker.Lock();
		}

		start += i + 1;
	}
}


vm_page *
vm_page_at_index(int32 index)
{
	return &sPages[index];
}


vm_page *
vm_lookup_page(page_num_t pageNumber)
{
	if (pageNumber < sPhysicalPageOffset)
		return NULL;

	pageNumber -= sPhysicalPageOffset;
	if (pageNumber >= sNumPages)
		return NULL;

	return &sPages[pageNumber];
}


bool
vm_page_is_dummy(struct vm_page *page)
{
	return page < sPages || page >= sPages + sNumPages;
}


/*!	Free the page that belonged to a certain cache.
	You can use vm_page_set_state() manually if you prefer, but only
	if the page does not equal PAGE_STATE_MODIFIED.
*/
void
vm_page_free(VMCache *cache, vm_page *page)
{
	PAGE_ASSERT(page, page->State() != PAGE_STATE_FREE
		&& page->State() != PAGE_STATE_CLEAR);

	if (page->State() == PAGE_STATE_MODIFIED && cache->temporary)
		atomic_add(&sModifiedTemporaryPages, -1);

	free_page(page, false);
}


void
vm_page_set_state(vm_page *page, int pageState)
{
	PAGE_ASSERT(page, page->State() != PAGE_STATE_FREE
		&& page->State() != PAGE_STATE_CLEAR);

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
	PAGE_ASSERT(page, page->Cache() != NULL);
	page->Cache()->AssertLocked();
	// DEBUG_PAGE_ACCESS_CHECK(page);
		// TODO: This assertion cannot be satisfied by idle_scan_active_pages()
		// when it requeues busy pages. The reason is that vm_soft_fault()
		// (respectively fault_get_page()) and the file cache keep newly
		// allocated pages accessed while they are reading them from disk. It
		// would probably be better to change that code and reenable this
		// check.

	VMPageQueue *queue = NULL;

	switch (page->State()) {
		case PAGE_STATE_ACTIVE:
			queue = &sActivePageQueue;
			break;
		case PAGE_STATE_INACTIVE:
			queue = &sInactivePageQueue;
			break;
		case PAGE_STATE_MODIFIED:
			queue = &sModifiedPageQueue;
			break;
		case PAGE_STATE_CACHED:
			queue = &sCachedPageQueue;
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
				page, page->State());
			break;
	}

	queue->RequeueUnlocked(page, tail);
}


page_num_t
vm_page_num_pages(void)
{
	return sNumPages - sNonExistingPages;
}


/*! There is a subtle distinction between the page counts returned by
	this function and vm_page_num_free_pages():
	The latter returns the number of pages that are completely uncommitted,
	whereas this one returns the number of pages that are available for
	use by being reclaimed as well (IOW it factors in things like cache pages
	as available).
*/
page_num_t
vm_page_num_available_pages(void)
{
	return vm_available_memory() / B_PAGE_SIZE;
}


page_num_t
vm_page_num_free_pages(void)
{
	int32 count = sUnreservedFreePages + sCachedPageQueue.Count();
	return count > 0 ? count : 0;
}


page_num_t
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

	info->max_pages = sNumPages - sNonExistingPages;
	info->used_pages = gMappedPagesCount + sInactivePageQueue.Count()
		- blockCachePages;
	info->cached_pages = info->max_pages >= free + info->used_pages
		? info->max_pages - free - info->used_pages : 0;
	info->page_faults = vm_num_page_faults();
	info->ignored_pages = sIgnoredPages;

	// TODO: We don't consider pages used for page directories/tables yet.
}


/*!	Returns the greatest address within the last page of accessible physical
	memory.
	The value is inclusive, i.e. in case of a 32 bit phys_addr_t 0xffffffff
	means the that the last page ends at exactly 4 GB.
*/
phys_addr_t
vm_page_max_address()
{
	return ((phys_addr_t)sPhysicalPageOffset + sNumPages) * B_PAGE_SIZE - 1;
}
