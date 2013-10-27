/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2009, Rene Gollent, rene@gollent.com.
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Angelo Mottola, a.mottola@libero.it.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*! The thread scheduler */


#include <OS.h>

#include <AutoDeleter.h>
#include <cpu.h>
#include <debug.h>
#include <int.h>
#include <kernel.h>
#include <kscheduler.h>
#include <listeners.h>
#include <scheduler_defs.h>
#include <smp.h>
#include <thread.h>
#include <timer.h>
#include <util/Heap.h>
#include <util/MinMaxHeap.h>

#include "RunQueue.h"
#include "scheduler_common.h"
#include "scheduler_tracing.h"


//#define TRACE_SCHEDULER
#ifdef TRACE_SCHEDULER
#	define TRACE(...) dprintf_no_syslog(__VA_ARGS__)
#else
#	define TRACE(...) do { } while (false)
#endif


spinlock gSchedulerLock = B_SPINLOCK_INITIALIZER;
SchedulerListenerList gSchedulerListeners;

bool sSchedulerEnabled;

const bigtime_t kThreadQuantum = 1000;
const bigtime_t kMinThreadQuantum = 3000;
const bigtime_t kMaxThreadQuantum = 10000;

const bigtime_t kMinimalWaitTime = kThreadQuantum / 4;

const bigtime_t kCacheExpire = 100000;

static int sDisableSmallTaskPacking;
static int32 sSmallTaskCore;

static bool sSingleCore;

static scheduler_mode sSchedulerMode;

static int32 (*sChooseCore)(Thread* thread);


// Heaps in sCPUPriorityHeaps are used for load balancing on a core the logical
// processors in the heap belong to. Since there are no cache affinity issues
// at this level and the run queue is shared among all logical processors on
// the core the only real concern is to make lower priority threads give way to
// the higher priority threads.
struct CPUEntry : public MinMaxHeapLinkImpl<CPUEntry, int32> {
	int32		fCPUNumber;

	bigtime_t	fMeasureActiveTime;
	bigtime_t	fMeasureTime;

	int			fLoad;
};
typedef MinMaxHeap<CPUEntry, int32> CPUHeap;
static CPUEntry* sCPUEntries;
static CPUHeap* sCPUPriorityHeaps;

struct CoreEntry : public DoublyLinkedListLinkImpl<CoreEntry> {
	HeapLink<CoreEntry, int32>	fPriorityHeapLink;
	MinMaxHeapLink<CoreEntry, int32>	fThreadHeapLink;

	int32		fCoreID;

	bigtime_t	fStartedBottom;
	bigtime_t	fReachedBottom;
	bigtime_t	fStartedIdle;
	bigtime_t	fReachedIdle;

	bigtime_t	fActiveTime;

	int32		fCPUBoundThreads;
	int32		fThreads;

	int			fLoad;
};

static CoreEntry* sCoreEntries;
typedef Heap<CoreEntry, int32, HeapLesserCompare<int32>,
		HeapMemberGetLink<CoreEntry, int32, &CoreEntry::fPriorityHeapLink> >
	CorePriorityHeap;
static CorePriorityHeap* sCorePriorityHeap;

typedef MinMaxHeap<CoreEntry, int32, MinMaxHeapCompare<int32>,
		MinMaxHeapMemberGetLink<CoreEntry, int32, &CoreEntry::fThreadHeapLink> >
	CoreThreadHeap;
static CoreThreadHeap* sCoreThreadHeap;
static CoreThreadHeap* sCoreCPUBoundThreadHeap;

static int32 sCPUBoundThreads;
static int32 sAssignedThreads;

// sPackageUsageHeap is used to decide which core should be woken up from the
// idle state. When aiming for performance we should use as many packages as
// possible with as little cores active in each package as possible (so that the
// package can enter any boost mode if it has one and the active core have more
// of the shared cache for themselves. If power saving is the main priority we
// should keep active cores on as little packages as possible (so that other
// packages can go to the deep state of sleep). The heap stores only packages
// with at least one core active and one core idle. The packages with all cores
// idle are stored in sPackageIdleList (in LIFO manner).
struct PackageEntry : public MinMaxHeapLinkImpl<PackageEntry, int32>,
	DoublyLinkedListLinkImpl<PackageEntry> {
	int32						fPackageID;

	DoublyLinkedList<CoreEntry>	fIdleCores;
	int32						fIdleCoreCount;

	int32						fCoreCount;
};
typedef MinMaxHeap<PackageEntry, int32> PackageHeap;
typedef DoublyLinkedList<PackageEntry> IdlePackageList;

static PackageEntry* sPackageEntries;
static PackageHeap* sPackageUsageHeap;
static IdlePackageList* sIdlePackageList;

// The run queues. Holds the threads ready to run ordered by priority.
// One queue per schedulable target per core. Additionally, each
// logical processor has its sPinnedRunQueues used for scheduling
// pinned threads.
typedef RunQueue<Thread, THREAD_MAX_SET_PRIORITY> ThreadRunQueue;
static ThreadRunQueue* sRunQueues;
static ThreadRunQueue* sPinnedRunQueues;
static int32 sRunQueueCount;

// Since CPU IDs used internally by the kernel bear no relation to the actual
// CPU topology the following arrays are used to efficiently get the core
// and the package that CPU in question belongs to.
static int32* sCPUToCore;
static int32* sCPUToPackage;


struct scheduler_thread_data {
						scheduler_thread_data() { Init(); }
	inline	void		Init();

			int32		priority_penalty;
			int32		additional_penalty;

			bool		lost_cpu;
			bool		cpu_bound;

			bigtime_t	time_left;
			bigtime_t	stolen_time;
			bigtime_t	quantum_start;

			bigtime_t	went_sleep;
			bigtime_t	went_sleep_active;

			int32		previous_core;
};


void
scheduler_thread_data::Init()
{
	priority_penalty = 0;
	additional_penalty = 0;

	time_left = 0;
	stolen_time = 0;

	went_sleep = 0;
	went_sleep_active = 0;

	lost_cpu = false;
	cpu_bound = true;

	previous_core = -1;
}


static inline int
get_minimal_priority(Thread* thread)
{
	return min_c(thread->priority, 25) / 5;
}


static inline int32
get_thread_penalty(Thread* thread)
{
	int32 penalty = thread->scheduler_data->priority_penalty;

	const int kMinimalPriority = get_minimal_priority(thread);
	if (kMinimalPriority > 0) {
		penalty
			+= thread->scheduler_data->additional_penalty % kMinimalPriority;
	}

	return penalty;
}


static inline int32
get_effective_priority(Thread* thread)
{
	if (thread->priority == B_IDLE_PRIORITY)
		return thread->priority;
	if (thread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return thread->priority;

	int32 effectivePriority = thread->priority;
	effectivePriority -= get_thread_penalty(thread);

	ASSERT(effectivePriority < B_FIRST_REAL_TIME_PRIORITY);
	ASSERT(effectivePriority >= B_LOWEST_ACTIVE_PRIORITY);

	return effectivePriority;
}


static void
dump_queue(ThreadRunQueue::ConstIterator& iterator)
{
	if (!iterator.HasNext())
		kprintf("Run queue is empty.\n");
	else {
		kprintf("thread      id      priority penalty  name\n");
		while (iterator.HasNext()) {
			Thread* thread = iterator.Next();
			kprintf("%p  %-7" B_PRId32 " %-8" B_PRId32 " %-8" B_PRId32 " %s\n",
				thread, thread->id, thread->priority,
				get_thread_penalty(thread), thread->name);
		}
	}
}


static int
dump_run_queue(int argc, char **argv)
{
	int32 cpuCount = smp_get_num_cpus();
	int32 coreCount = 0;
	for (int32 i = 0; i < cpuCount; i++) {
		if (gCPU[i].topology_id[CPU_TOPOLOGY_SMT] == 0)
			sCPUToCore[i] = coreCount++;
	}

	ThreadRunQueue::ConstIterator iterator;
	for (int32 i = 0; i < coreCount; i++) {
		kprintf("\nCore %" B_PRId32 " run queue:\n", i);
		iterator = sRunQueues[i].GetConstIterator();
		dump_queue(iterator);
	}

	for (int32 i = 0; i < cpuCount; i++) {
		iterator = sPinnedRunQueues[i].GetConstIterator();

		if (iterator.HasNext()) {
			kprintf("\nCPU %" B_PRId32 " run queue:\n", i);
			dump_queue(iterator);
		}
	}

	return 0;
}


static void
dump_heap(CPUHeap* heap)
{
	CPUHeap temp(smp_get_num_cpus());

	kprintf("cpu priority load\n");
	CPUEntry* entry = heap->PeekMinimum();
	while (entry) {
		int32 cpu = entry->fCPUNumber;
		int32 key = CPUHeap::GetKey(entry);
		kprintf("%3" B_PRId32 " %8" B_PRId32 " %3d%%\n", cpu, key,
			sCPUEntries[cpu].fLoad / 10);

		heap->RemoveMinimum();
		temp.Insert(entry, key);

		entry = heap->PeekMinimum();
	}

	entry = temp.PeekMinimum();
	while (entry) {
		int32 key = CPUHeap::GetKey(entry);
		temp.RemoveMinimum();
		heap->Insert(entry, key);
		entry = temp.PeekMinimum();
	}
}


static void
dump_core_thread_heap(CoreThreadHeap* heap)
{
	CoreThreadHeap temp(sRunQueueCount);
	int32 cpuPerCore = smp_get_num_cpus() / sRunQueueCount;

	CoreEntry* entry = heap->PeekMinimum();
	while (entry) {
		int32 key = CoreThreadHeap::GetKey(entry);
		kprintf("%4" B_PRId32 " %6" B_PRId32 " %7" B_PRId32 " %9" B_PRId32
			" %3d%%\n", entry->fCoreID, key, entry->fThreads,
			entry->fCPUBoundThreads, entry->fLoad / cpuPerCore / 10);

		heap->RemoveMinimum();
		temp.Insert(entry, key);

		entry = heap->PeekMinimum();
	}

	entry = temp.PeekMinimum();
	while (entry) {
		int32 key = CoreThreadHeap::GetKey(entry);
		temp.RemoveMinimum();
		heap->Insert(entry, key);
		entry = temp.PeekMinimum();
	}
}


static int
dump_cpu_heap(int argc, char** argv)
{
	CorePriorityHeap temp(sRunQueueCount);

	CoreEntry* entry = sCorePriorityHeap->PeekRoot();
	if (entry != NULL)
		kprintf("core priority\n");
	else
		kprintf("No active cores.\n");

	while (entry) {
		int32 core = entry->fCoreID;
		int32 key = CorePriorityHeap::GetKey(entry);
		kprintf("%4" B_PRId32 " %8" B_PRId32 "\n", core, key);

		sCorePriorityHeap->RemoveRoot();
		temp.Insert(entry, key);

		entry = sCorePriorityHeap->PeekRoot();
	}

	entry = temp.PeekRoot();
	while (entry) {
		int32 key = CorePriorityHeap::GetKey(entry);
		temp.RemoveRoot();
		sCorePriorityHeap->Insert(entry, key);
		entry = temp.PeekRoot();
	}

	kprintf("\ncore    key threads cpu-bound load\n");
	dump_core_thread_heap(sCoreThreadHeap);
	dump_core_thread_heap(sCoreCPUBoundThreadHeap);

	for (int32 i = 0; i < sRunQueueCount; i++) {
		kprintf("\nCore %" B_PRId32 " heap:\n", i);
		dump_heap(&sCPUPriorityHeaps[i]);
	}

	return 0;
}


static int
dump_idle_cores(int argc, char** argv)
{
	kprintf("Idle packages:\n");
	IdlePackageList::ReverseIterator idleIterator
		= sIdlePackageList->GetReverseIterator();

	if (idleIterator.HasNext()) {
		kprintf("package cores\n");

		while (idleIterator.HasNext()) {
			PackageEntry* entry = idleIterator.Next();
			kprintf("%-7" B_PRId32 " ", entry->fPackageID);

			DoublyLinkedList<CoreEntry>::ReverseIterator iterator
				= entry->fIdleCores.GetReverseIterator();
			if (iterator.HasNext()) {
				while (iterator.HasNext()) {
					CoreEntry* coreEntry = iterator.Next();
					kprintf("%" B_PRId32 "%s", coreEntry->fCoreID,
						iterator.HasNext() ? ", " : "");
				}
			} else
				kprintf("-");
			kprintf("\n");
		}
	} else
		kprintf("No idle packages.\n");

	PackageHeap temp(smp_get_num_cpus());
	kprintf("\nPackages with idle cores:\n");

	PackageEntry* entry = sPackageUsageHeap->PeekMinimum();
	if (entry == NULL)
		kprintf("No packages.\n");
	else
		kprintf("package count cores\n");

	while (entry != NULL) {
		kprintf("%-7" B_PRId32 " %-5" B_PRId32 " ", entry->fPackageID,
			entry->fIdleCoreCount);

		DoublyLinkedList<CoreEntry>::ReverseIterator iterator
			= entry->fIdleCores.GetReverseIterator();
		if (iterator.HasNext()) {
			while (iterator.HasNext()) {
				CoreEntry* coreEntry = iterator.Next();
				kprintf("%" B_PRId32 "%s", coreEntry->fCoreID,
					iterator.HasNext() ? ", " : "");
			}
		} else
			kprintf("-");
		kprintf("\n");

		sPackageUsageHeap->RemoveMinimum();
		temp.Insert(entry, entry->fIdleCoreCount);

		entry = sPackageUsageHeap->PeekMinimum();
	}

	entry = temp.PeekMinimum();
	while (entry != NULL) {
		int32 key = PackageHeap::GetKey(entry);
		temp.RemoveMinimum();
		sPackageUsageHeap->Insert(entry, key);
		entry = temp.PeekMinimum();
	}

	return 0;
}


static inline bool
has_cache_expired(Thread* thread)
{
	ASSERT(!sSingleCore);

	if (thread_is_idle_thread(thread))
		return false;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	ASSERT(schedulerThreadData->previous_core >= 0);

	CoreEntry* coreEntry = &sCoreEntries[schedulerThreadData->previous_core];
	switch (sSchedulerMode) {
		case SCHEDULER_MODE_PERFORMANCE:
			return coreEntry->fActiveTime
					- schedulerThreadData->went_sleep_active > kCacheExpire;

		case SCHEDULER_MODE_POWER_SAVING:
			return system_time() - schedulerThreadData->went_sleep
				> kCacheExpire;

		default:
			return true;
	}
}


void
scheduler_dump_thread_data(Thread* thread)
{
	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	kprintf("\tpriority_penalty:\t%" B_PRId32 "\n",
		schedulerThreadData->priority_penalty);

	int32 additionalPenalty = 0;
	const int kMinimalPriority = get_minimal_priority(thread);
	if (kMinimalPriority > 0) {
		additionalPenalty
			= schedulerThreadData->additional_penalty % kMinimalPriority;
	}
	kprintf("\tadditional_penalty:\t%" B_PRId32 " (%" B_PRId32 ")\n",
		additionalPenalty, schedulerThreadData->additional_penalty);
	kprintf("\tstolen_time:\t\t%" B_PRId64 "\n",
		schedulerThreadData->stolen_time);
	kprintf("\twent_sleep:\t\t%" B_PRId64 "\n",
		schedulerThreadData->went_sleep);
	kprintf("\twent_sleep_active:\t%" B_PRId64 "\n",
		schedulerThreadData->went_sleep_active);
	kprintf("\tprevious_core:\t\t%" B_PRId32 "\n",
		schedulerThreadData->previous_core);
	if (schedulerThreadData->previous_core > 0
		&& has_cache_expired(thread)) {
		kprintf("\tcache affinity has expired\n");
	}
}


static void
update_thread_heaps(int32 core)
{
	ASSERT(!sSingleCore);

	CoreEntry* entry = &sCoreEntries[core];

	ASSERT(entry->fCPUBoundThreads >= 0
		&& entry->fCPUBoundThreads <= entry->fThreads);
	ASSERT(entry->fThreads >= 0
		&& entry->fThreads <= thread_max_threads());

	int32 newKey = entry->fCPUBoundThreads * thread_max_threads();
	newKey += entry->fThreads;

	int32 oldKey = CoreThreadHeap::GetKey(entry);

	if (oldKey == newKey)
		return;

	if (newKey > thread_max_threads()) {
		if (oldKey <= thread_max_threads()) {
			sCoreThreadHeap->ModifyKey(entry, -1);
			ASSERT(sCoreThreadHeap->PeekMinimum() == entry);
			sCoreThreadHeap->RemoveMinimum();

			sCoreCPUBoundThreadHeap->Insert(entry, newKey);
		} else
			sCoreCPUBoundThreadHeap->ModifyKey(entry, newKey);
	} else {
		if (oldKey > thread_max_threads()) {
			sCoreCPUBoundThreadHeap->ModifyKey(entry, -1);
			ASSERT(sCoreCPUBoundThreadHeap->PeekMinimum() == entry);
			sCoreCPUBoundThreadHeap->RemoveMinimum();

			sCoreThreadHeap->Insert(entry, newKey);
		} else
			sCoreThreadHeap->ModifyKey(entry, newKey);
	}
}


static inline void
disable_small_task_packing(void)
{
	ASSERT(!sSingleCore);

	ASSERT(sDisableSmallTaskPacking == 0);
	ASSERT(sSmallTaskCore == sCPUToCore[smp_get_current_cpu()]);

	ASSERT(sAssignedThreads > 0);
	sDisableSmallTaskPacking = sAssignedThreads * 64;
	sSmallTaskCore = -1;
}


static inline void
increase_penalty(Thread* thread)
{
	if (thread->priority <= B_LOWEST_ACTIVE_PRIORITY)
		return;
	if (thread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return;

	TRACE("increasing thread %ld penalty\n", thread->id);

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	int32 oldPenalty = schedulerThreadData->priority_penalty++;

	ASSERT(thread->priority - oldPenalty >= B_LOWEST_ACTIVE_PRIORITY);

	const int kMinimalPriority = get_minimal_priority(thread);
	if (thread->priority - oldPenalty <= kMinimalPriority) {
		int32 core = schedulerThreadData->previous_core;
		ASSERT(core >= 0);

		int32 additionalPenalty = schedulerThreadData->additional_penalty;
		if (additionalPenalty == 0 && !sSingleCore) {
			sCPUBoundThreads++;
			sCoreEntries[core].fCPUBoundThreads++;

			update_thread_heaps(core);
		}

		const int kSmallTaskThreshold = 50;
		if (additionalPenalty > kSmallTaskThreshold && !sSingleCore) {
			if (sSmallTaskCore == core)
				disable_small_task_packing();
		}

		schedulerThreadData->priority_penalty = oldPenalty;
		schedulerThreadData->additional_penalty++;
	}
}


static inline void
cancel_penalty(Thread* thread)
{
	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	if (schedulerThreadData->priority_penalty != 0)
		TRACE("cancelling thread %ld penalty\n", thread->id);

	schedulerThreadData->additional_penalty = 0;
	schedulerThreadData->priority_penalty = 0;
}


static inline void
update_priority_heaps(int32 cpu, int32 priority)
{
	int32 core = sCPUToCore[cpu];

	sCPUPriorityHeaps[core].ModifyKey(&sCPUEntries[cpu], priority);

	if (sSingleCore)
		return;

	int32 maxPriority
		= CPUHeap::GetKey(sCPUPriorityHeaps[core].PeekMaximum());
	int32 corePriority = CorePriorityHeap::GetKey(&sCoreEntries[core]);

	if (corePriority != maxPriority) {
		if (maxPriority == B_IDLE_PRIORITY) {
			sCorePriorityHeap->ModifyKey(&sCoreEntries[core], B_IDLE_PRIORITY);
			ASSERT(sCorePriorityHeap->PeekRoot() == &sCoreEntries[core]);
			sCorePriorityHeap->RemoveRoot();
		} else if (corePriority == B_IDLE_PRIORITY)
			sCorePriorityHeap->Insert(&sCoreEntries[core], maxPriority);
		else
			sCorePriorityHeap->ModifyKey(&sCoreEntries[core], maxPriority);

		int32 package = sCPUToPackage[cpu];
		PackageEntry* packageEntry = &sPackageEntries[package];
		if (maxPriority == B_IDLE_PRIORITY) {
			// core goes idle
			ASSERT(packageEntry->fIdleCoreCount >= 0);
			ASSERT(packageEntry->fIdleCoreCount < packageEntry->fCoreCount);

			packageEntry->fIdleCoreCount++;
			packageEntry->fIdleCores.Add(&sCoreEntries[core]);

			if (packageEntry->fIdleCoreCount == 1) {
				// first core on that package to go idle

				if (packageEntry->fCoreCount > 1)
					sPackageUsageHeap->Insert(packageEntry, 1);
				else
					sIdlePackageList->Add(packageEntry);
			} else if (packageEntry->fIdleCoreCount
				== packageEntry->fCoreCount) {
				// package goes idle
				sPackageUsageHeap->ModifyKey(packageEntry, 0);
				ASSERT(sPackageUsageHeap->PeekMinimum() == packageEntry);
				sPackageUsageHeap->RemoveMinimum();

				sIdlePackageList->Add(packageEntry);
			} else {
				sPackageUsageHeap->ModifyKey(packageEntry,
					packageEntry->fIdleCoreCount);
			}
		} else if (corePriority == B_IDLE_PRIORITY) {
			// core wakes up
			ASSERT(packageEntry->fIdleCoreCount > 0);
			ASSERT(packageEntry->fIdleCoreCount <= packageEntry->fCoreCount);

			packageEntry->fIdleCoreCount--;
			packageEntry->fIdleCores.Remove(&sCoreEntries[core]);

			if (packageEntry->fIdleCoreCount + 1 == packageEntry->fCoreCount) {
				// package wakes up
				sIdlePackageList->Remove(packageEntry);

				if (packageEntry->fIdleCoreCount > 0) {
					sPackageUsageHeap->Insert(packageEntry,
						packageEntry->fIdleCoreCount);
				}
			} else if (packageEntry->fIdleCoreCount == 0) {
				// no more idle cores in the package
				sPackageUsageHeap->ModifyKey(packageEntry, 0);
				ASSERT(sPackageUsageHeap->PeekMinimum() == packageEntry);
				sPackageUsageHeap->RemoveMinimum();
			} else {
				sPackageUsageHeap->ModifyKey(packageEntry,
					packageEntry->fIdleCoreCount);
			}
		}
	}
}


static int32
choose_core_performance(Thread* thread)
{
	CoreEntry* entry;

	if (sIdlePackageList->Last() != NULL) {
		// wake new package
		PackageEntry* package = sIdlePackageList->Last();
		entry = package->fIdleCores.Last();
	} else if (sPackageUsageHeap->PeekMaximum() != NULL) {
		// wake new core
		PackageEntry* package = sPackageUsageHeap->PeekMaximum();
		entry = package->fIdleCores.Last();
	} else {
		// no idle cores, use least occupied core
		entry = sCorePriorityHeap->PeekRoot();

		int32 priority = get_effective_priority(thread);
		if (CorePriorityHeap::GetKey(entry) >= priority) {
			entry = sCoreThreadHeap->PeekMinimum();
			if (entry == NULL)
				entry = sCoreCPUBoundThreadHeap->PeekMinimum();
		}
	}

	ASSERT(entry != NULL);
	return entry->fCoreID;
}


static inline bool
is_task_small(Thread* thread)
{
	int32 priority = get_effective_priority(thread);
	int32 penalty = thread->scheduler_data->priority_penalty;
	return penalty < 2 || priority >= B_DISPLAY_PRIORITY;
}


static int32
choose_core_power_saving(Thread* thread)
{
	CoreEntry* entry;

	int32 priority = get_effective_priority(thread);

	if (sDisableSmallTaskPacking > 0)
		sDisableSmallTaskPacking--;

	if (!sDisableSmallTaskPacking && is_task_small(thread)
		&& sCoreThreadHeap->PeekMaximum() != NULL) {
		// try to pack all threads on one core
		if (sSmallTaskCore < 0)
			sSmallTaskCore = sCoreThreadHeap->PeekMaximum()->fCoreID;
		entry = &sCoreEntries[sSmallTaskCore];
	} else if (sCorePriorityHeap->PeekRoot() != NULL
		&& CorePriorityHeap::GetKey(sCorePriorityHeap->PeekRoot())
			< priority) {
		// run immediately on already woken core
		entry = sCorePriorityHeap->PeekRoot();
	} else if (sPackageUsageHeap->PeekMinimum() != NULL) {
		// wake new core
		PackageEntry* package = sPackageUsageHeap->PeekMinimum();
		entry = package->fIdleCores.Last();
	} else if (sIdlePackageList->Last() != NULL) {
		// wake new package
		PackageEntry* package = sIdlePackageList->Last();
		entry = package->fIdleCores.Last();
	} else {
		// no idle cores, use least occupied core
		entry = sCoreThreadHeap->PeekMinimum();
		if (entry == NULL)
			entry = sCoreCPUBoundThreadHeap->PeekMinimum();
	}

	ASSERT(entry != NULL);
	return entry->fCoreID;
}


static inline int32
choose_core(Thread* thread)
{
	ASSERT(!sSingleCore);
	return sChooseCore(thread);
}


static inline int32
choose_cpu(int32 core)
{
	CPUEntry* entry = sCPUPriorityHeaps[core].PeekMinimum();
	ASSERT(entry != NULL);
	return entry->fCPUNumber;
}


static bool
should_rebalance(Thread* thread)
{
	ASSERT(!sSingleCore);

	if (thread_is_idle_thread(thread))
		return false;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	ASSERT(schedulerThreadData->previous_core >= 0);

	CoreEntry* coreEntry = &sCoreEntries[schedulerThreadData->previous_core];

	// If this is a cpu bound thread and we have significantly more such threads
	//  than the average get rid of this one.
	if (schedulerThreadData->additional_penalty != 0) {
		int32 averageCPUBound = sCPUBoundThreads / sRunQueueCount;
		return coreEntry->fCPUBoundThreads - averageCPUBound > 1;
	}

	// If this thread is not cpu bound but we have at least one consider giving
	// this one to someone less busy.
	int32 averageThread = sAssignedThreads / sRunQueueCount;
	if (coreEntry->fCPUBoundThreads > 0) {
		CoreEntry* other = sCoreThreadHeap->PeekMinimum();
		if (other != NULL
			&& CoreThreadHeap::GetKey(other) <= averageThread) {
			return true;
		}
	}

	int32 threadsAboveAverage = coreEntry->fThreads - averageThread;

	// All cores try to give us small tasks, check whether we have enough.
	const int kSmallTaskCountThreshold = 5;
	if (sDisableSmallTaskPacking == 0 && sSmallTaskCore == coreEntry->fCoreID) {
		if (threadsAboveAverage > kSmallTaskCountThreshold) {
			if (!is_task_small(thread))
				return true;
		} else if (threadsAboveAverage > 2 * kSmallTaskCountThreshold) {
			disable_small_task_packing();
		}
	}

	// Try our luck at small task packing.
	if (sDisableSmallTaskPacking == 0 && is_task_small(thread))
		return sSmallTaskCore != coreEntry->fCoreID;

	// No cpu bound threads - the situation is quite good. Make sure it
	// won't get much worse...
	const int32 kBalanceThreshold = 3;
	return threadsAboveAverage > kBalanceThreshold;
}


static void
assign_active_thread_to_core(Thread* thread)
{
	if (thread_is_idle_thread(thread) || sSingleCore)
		return;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	ASSERT(schedulerThreadData->previous_core >= 0);
	int32 core = schedulerThreadData->previous_core;

	sCoreEntries[core].fThreads++;
	sAssignedThreads++;

	if (schedulerThreadData->additional_penalty != 0) {
		sCoreEntries[core].fCPUBoundThreads++;
		sCPUBoundThreads++;
	}

	update_thread_heaps(core);
}


static inline void
thread_goes_away(Thread* thread)
{
	if (thread_is_idle_thread(thread))
		return;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	ASSERT(schedulerThreadData->previous_core >= 0);
	int32 core = schedulerThreadData->previous_core;

	schedulerThreadData->went_sleep = system_time();
	schedulerThreadData->went_sleep_active = sCoreEntries[core].fActiveTime;

	if (sSingleCore)
		return;

	ASSERT(sCoreEntries[core].fThreads > 0);
	ASSERT(sCoreEntries[core].fThreads > sCoreEntries[core].fCPUBoundThreads
		|| (sCoreEntries[core].fThreads == sCoreEntries[core].fCPUBoundThreads
			&& schedulerThreadData->additional_penalty != 0));
	sCoreEntries[core].fThreads--;
	sAssignedThreads--;

	if (schedulerThreadData->additional_penalty != 0) {
		ASSERT(sCoreEntries[core].fCPUBoundThreads > 0);
		sCoreEntries[core].fCPUBoundThreads--;
		sCPUBoundThreads--;
	}

	update_thread_heaps(core);
}


static inline bool
should_cancel_penalty(Thread* thread)
{
	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	int32 core = schedulerThreadData->previous_core;

	if (core < -1)
		return false;

	bigtime_t now = system_time();
	bigtime_t wentSleep = schedulerThreadData->went_sleep;

	if (wentSleep < sCoreEntries[core].fReachedIdle)
		return true;

	if (sCoreEntries[core].fStartedIdle != 0) {
		if (wentSleep < sCoreEntries[core].fStartedIdle
			&& now - sCoreEntries[core].fStartedIdle >= kMinimalWaitTime) {
			return true;
		}

		if (wentSleep - sCoreEntries[core].fStartedIdle >= kMinimalWaitTime)
			return true;
	}

	if (get_effective_priority(thread) == B_LOWEST_ACTIVE_PRIORITY)
		return false;

	if (wentSleep < sCoreEntries[core].fReachedIdle)
		return true;

	if (sCoreEntries[core].fStartedBottom != 0) {
		if (wentSleep < sCoreEntries[core].fStartedBottom
			&& now - sCoreEntries[core].fStartedBottom >= kMinimalWaitTime) {
			return true;
		}

		if (wentSleep - sCoreEntries[core].fStartedBottom >= kMinimalWaitTime)
			return true;
	}

	return false;
}


static void
enqueue(Thread* thread, bool newOne)
{
	ASSERT(thread != NULL);

	thread->state = thread->next_state = B_THREAD_READY;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	int32 core = schedulerThreadData->previous_core;
	if (newOne && core >= 0) {
		int32 priority = get_effective_priority(thread);

		if (priority == B_LOWEST_ACTIVE_PRIORITY) {
			if (schedulerThreadData->went_sleep
				< sCoreEntries[core].fReachedIdle) {
				cancel_penalty(thread);
			}
		} else {
			if (schedulerThreadData->went_sleep
				< sCoreEntries[core].fReachedBottom) {
				cancel_penalty(thread);
			}
		}
	}

	int32 threadPriority = get_effective_priority(thread);

	T(EnqueueThread(thread, threadPriority));

	bool pinned = thread->pinned_to_cpu > 0;
	int32 targetCPU = -1;
	int32 targetCore;
	if (pinned) {
		targetCPU = thread->previous_cpu->cpu_num;
		targetCore = sCPUToCore[targetCPU];
		ASSERT(targetCore == schedulerThreadData->previous_core);

		if (newOne)
			assign_active_thread_to_core(thread);
	} else if (sSingleCore) {
		targetCore = 0;
		targetCPU = choose_cpu(targetCore);

		schedulerThreadData->previous_core = targetCore;
	} else if (schedulerThreadData->previous_core < 0
		|| (newOne && has_cache_expired(thread))
		|| should_rebalance(thread)) {

		if (thread_is_idle_thread(thread)) {
			targetCPU = thread->previous_cpu->cpu_num;
			targetCore = sCPUToCore[targetCPU];
		} else {
			if (!newOne)
				thread_goes_away(thread);

			targetCore = choose_core(thread);
			targetCPU = choose_cpu(targetCore);
		}

		schedulerThreadData->previous_core = targetCore;
		assign_active_thread_to_core(thread);
	} else {
		targetCore = schedulerThreadData->previous_core;
		targetCPU = choose_cpu(targetCore);
		if (newOne)
			assign_active_thread_to_core(thread);
	}

	ASSERT(targetCore >= 0 && targetCore < sRunQueueCount);
	ASSERT(targetCPU >= 0 && targetCPU < smp_get_num_cpus());

	TRACE("enqueueing thread %ld with priority %ld %ld\n", thread->id,
		threadPriority, targetCore);
	if (pinned)
		sPinnedRunQueues[targetCPU].PushBack(thread, threadPriority);
	else
		sRunQueues[targetCore].PushBack(thread, threadPriority);

	schedulerThreadData->cpu_bound = true;
	schedulerThreadData->time_left = 0;
	schedulerThreadData->stolen_time = 0;

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadEnqueuedInRunQueue,
		thread);

	Thread* targetThread = gCPU[targetCPU].running_thread;
	int32 targetPriority = get_effective_priority(targetThread);

	TRACE("choosing CPU %ld with current priority %ld\n", targetCPU,
		targetPriority);

	if (threadPriority > targetPriority) {
		targetThread->scheduler_data->lost_cpu = true;

		// It is possible that another CPU schedules the thread before the
		// target CPU. However, since the target CPU is sent an ICI it will
		// reschedule anyway and update its heap key to the correct value.
		update_priority_heaps(targetCPU, threadPriority);

		if (targetCPU == smp_get_current_cpu())
			gCPU[targetCPU].invoke_scheduler = true;
		else {
			smp_send_ici(targetCPU, SMP_MSG_RESCHEDULE, 0, 0, 0, NULL,
				SMP_MSG_FLAG_ASYNC);
		}
	}
}


/*!	Enqueues the thread into the run queue.
	Note: thread lock must be held when entering this function
*/
void
scheduler_enqueue_in_run_queue(Thread *thread)
{
	TRACE("enqueueing new thread %ld with static priority %ld\n", thread->id,
		thread->priority);
	enqueue(thread, true);
}


static inline void
put_back(Thread* thread)
{
	bool pinned = sPinnedRunQueues != NULL && thread->pinned_to_cpu > 0;

	if (pinned) {
		int32 pinnedCPU = thread->previous_cpu->cpu_num;
		sPinnedRunQueues[pinnedCPU].PushFront(thread,
			get_effective_priority(thread));
	} else {
		int32 previousCore = thread->scheduler_data->previous_core;
		ASSERT(previousCore >= 0);
		sRunQueues[previousCore].PushFront(thread,
			get_effective_priority(thread));
	}
}


/*!	Sets the priority of a thread.
	Note: thread lock must be held when entering this function
*/
void
scheduler_set_thread_priority(Thread *thread, int32 priority)
{
	if (priority == thread->priority)
		return;

	TRACE("changing thread %ld priority to %ld (old: %ld, effective: %ld)\n",
		thread->id, priority, thread->priority,
		get_effective_priority(thread));

	if (thread->state == B_THREAD_RUNNING)
		thread_goes_away(thread);

	if (thread->state != B_THREAD_READY) {
		cancel_penalty(thread);
		thread->priority = priority;

		if (thread->state == B_THREAD_RUNNING) {
			assign_active_thread_to_core(thread);
			update_priority_heaps(thread->cpu->cpu_num, priority);
		}
		return;
	}

	// The thread is in the run queue. We need to remove it and re-insert it at
	// a new position.

	T(RemoveThread(thread));

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadRemovedFromRunQueue,
		thread);

	// remove thread from run queue
	int32 previousCore = thread->scheduler_data->previous_core;
	ASSERT(previousCore >= 0);
	sRunQueues[previousCore].Remove(thread);
	thread_goes_away(thread);

	// set priority and re-insert
	cancel_penalty(thread);
	thread->priority = priority;

	scheduler_enqueue_in_run_queue(thread);
}


static int32
reschedule_event(timer *unused)
{
	// This function is called as a result of the timer event set by the
	// scheduler. Make sure the reschedule() is invoked.
	Thread* thread= thread_get_current_thread();

	thread->scheduler_data->lost_cpu = true;
	thread->cpu->invoke_scheduler = true;
	thread->cpu->preempted = 1;
	return B_HANDLED_INTERRUPT;
}


static inline bool 
quantum_ended(Thread* thread, bool wasPreempted, bool hasYielded)
{
	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	if (hasYielded) {
		schedulerThreadData->time_left = 0;
		return true;
	}

	bigtime_t time_used = system_time() - schedulerThreadData->quantum_start;
	schedulerThreadData->time_left -= time_used;
	schedulerThreadData->time_left = max_c(0, schedulerThreadData->time_left);

	// too little time left, it's better make the next quantum a bit longer
	if (wasPreempted || schedulerThreadData->time_left <= kThreadQuantum / 50) {
		schedulerThreadData->stolen_time += schedulerThreadData->time_left;
		schedulerThreadData->time_left = 0;
	}

	return schedulerThreadData->time_left == 0;
}


static inline bigtime_t
quantum_linear_interpolation(bigtime_t maxQuantum, bigtime_t minQuantum,
	int32 maxPriority, int32 minPriority, int32 priority)
{
	ASSERT(priority <= maxPriority);
	ASSERT(priority >= minPriority);

	bigtime_t result = (maxQuantum - minQuantum) * (priority - minPriority);
	result /= maxPriority - minPriority;
	return maxQuantum - result;
}


static inline bigtime_t
get_base_quantum(Thread* thread)
{
	int32 priority = get_effective_priority(thread);

	if (priority >= B_URGENT_DISPLAY_PRIORITY)
		return kThreadQuantum;
	if (priority > B_NORMAL_PRIORITY) {
		return quantum_linear_interpolation(kThreadQuantum * 4,
			kThreadQuantum, B_URGENT_DISPLAY_PRIORITY, B_NORMAL_PRIORITY,
			priority);
	}
	return quantum_linear_interpolation(kThreadQuantum * 64,
		kThreadQuantum * 4, B_NORMAL_PRIORITY, B_IDLE_PRIORITY, priority);
}


static inline bigtime_t
compute_quantum(Thread* thread)
{
	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	bigtime_t quantum;
	if (schedulerThreadData->time_left != 0)
		quantum = schedulerThreadData->time_left;
	else
		quantum = get_base_quantum(thread);

	quantum += schedulerThreadData->stolen_time;
	schedulerThreadData->stolen_time = 0;

	schedulerThreadData->time_left = quantum;
	schedulerThreadData->quantum_start = system_time();

	return quantum;
}


static inline Thread*
dequeue_thread(int32 thisCPU)
{
	int32 thisCore = sCPUToCore[thisCPU];
	Thread* sharedThread = sRunQueues[thisCore].PeekMaximum();

	Thread* pinnedThread = NULL;
	if (sPinnedRunQueues != NULL)
		pinnedThread = sPinnedRunQueues[thisCPU].PeekMaximum();

	if (sharedThread == NULL && pinnedThread == NULL)
		return NULL;

	int32 pinnedPriority = -1;
	if (pinnedThread != NULL)
		pinnedPriority = get_effective_priority(pinnedThread);

	int32 sharedPriority = -1;
	if (sharedThread != NULL)
		sharedPriority = get_effective_priority(sharedThread);

	if (sharedPriority > pinnedPriority) {
		sRunQueues[thisCore].Remove(sharedThread);
		return sharedThread;
	}

	sPinnedRunQueues[thisCPU].Remove(pinnedThread);
	return pinnedThread;
}


static inline void
compute_cpu_load(int32 cpu)
{
	const bigtime_t kLoadMeasureInterval = 50000;
	const bigtime_t kIntervalInaccuracy = kLoadMeasureInterval / 4;

	int32 thisCPU = smp_get_current_cpu();

	bigtime_t now = system_time();
	bigtime_t deltaTime = now - sCPUEntries[cpu].fMeasureTime;

	if (deltaTime < kLoadMeasureInterval)
		return;

	int oldLoad = sCPUEntries[cpu].fLoad;

	int load = sCPUEntries[cpu].fMeasureActiveTime * 1000;
	load /= max_c(now - sCPUEntries[cpu].fMeasureTime, 1);

	sCPUEntries[cpu].fMeasureActiveTime = 0;
	sCPUEntries[cpu].fMeasureTime = now;

	deltaTime += kIntervalInaccuracy;
	int n = max_c(deltaTime / kLoadMeasureInterval, 1);
	if (n > 10)
		sCPUEntries[cpu].fLoad = load;
	else {
		load *= (1 << n) - 1;
		sCPUEntries[cpu].fLoad = (sCPUEntries[cpu].fLoad + load) / (1 << n);
	}

	if (oldLoad != load) {
		int32 core = sCPUToCore[cpu];
		sCoreEntries[core].fLoad -= oldLoad;
		sCoreEntries[core].fLoad += sCPUEntries[cpu].fLoad;
	}
}


static inline void
track_cpu_activity(Thread* oldThread, Thread* nextThread, int32 thisCore)
{
	bigtime_t now = system_time();
	bigtime_t usedTime = now - oldThread->scheduler_data->quantum_start;

	if (thread_is_idle_thread(oldThread) && usedTime >= kMinimalWaitTime) {
		sCoreEntries[thisCore].fReachedBottom = now - kMinimalWaitTime;
		sCoreEntries[thisCore].fReachedIdle = now - kMinimalWaitTime;
	}

	if (get_effective_priority(oldThread) == B_LOWEST_ACTIVE_PRIORITY
		&& usedTime >= kMinimalWaitTime) {
		sCoreEntries[thisCore].fReachedBottom = now - kMinimalWaitTime;
	}

	if (!thread_is_idle_thread(oldThread)) {
		bigtime_t active
			= (oldThread->kernel_time - oldThread->cpu->last_kernel_time)
				+ (oldThread->user_time - oldThread->cpu->last_user_time);

		atomic_add64(&oldThread->cpu->active_time, active);
		sCPUEntries[smp_get_current_cpu()].fMeasureActiveTime += active;
		sCoreEntries[thisCore].fActiveTime += active;
	}

	compute_cpu_load(smp_get_current_cpu());

	int32 oldPriority = get_effective_priority(oldThread);
	int32 nextPriority = get_effective_priority(nextThread);

	if (thread_is_idle_thread(nextThread)) {
		if (!thread_is_idle_thread(oldThread))
			sCoreEntries[thisCore].fStartedIdle = now;
		if (oldPriority > B_LOWEST_ACTIVE_PRIORITY)
			sCoreEntries[thisCore].fStartedBottom = now;
	} else if (nextPriority == B_LOWEST_ACTIVE_PRIORITY) {
		sCoreEntries[thisCore].fStartedIdle = 0;
		if (oldPriority > B_LOWEST_ACTIVE_PRIORITY)
			sCoreEntries[thisCore].fStartedBottom = now;
	} else {
		sCoreEntries[thisCore].fStartedBottom = 0;
		sCoreEntries[thisCore].fStartedIdle = 0;
	}

	if (!thread_is_idle_thread(nextThread)) {
		oldThread->cpu->last_kernel_time = nextThread->kernel_time;
		oldThread->cpu->last_user_time = nextThread->user_time;
	}
}


static void
_scheduler_reschedule(void)
{
	Thread* oldThread = thread_get_current_thread();

	int32 thisCPU = smp_get_current_cpu();
	int32 thisCore = sCPUToCore[thisCPU];

	TRACE("reschedule(): cpu %ld, current thread = %ld\n", thisCPU,
		oldThread->id);

	oldThread->state = oldThread->next_state;
	scheduler_thread_data* schedulerOldThreadData = oldThread->scheduler_data;

	// update CPU heap so that old thread would have CPU properly chosen
	Thread* nextThread = sRunQueues[thisCore].PeekMaximum();
	if (nextThread != NULL)
		update_priority_heaps(thisCPU, get_effective_priority(nextThread));

	switch (oldThread->next_state) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			if (!schedulerOldThreadData->lost_cpu)
				schedulerOldThreadData->cpu_bound = false;

			if (quantum_ended(oldThread, oldThread->cpu->preempted,
					oldThread->has_yielded)) {
				if (schedulerOldThreadData->cpu_bound)
					increase_penalty(oldThread);

				TRACE("enqueueing thread %ld into run queue priority = %ld\n",
					oldThread->id, get_effective_priority(oldThread));
				enqueue(oldThread, false);
			} else {
				TRACE("putting thread %ld back in run queue priority = %ld\n",
					oldThread->id, get_effective_priority(oldThread));
				put_back(oldThread);
			}

			break;
		case B_THREAD_SUSPENDED:
			increase_penalty(oldThread);
			thread_goes_away(oldThread);
			TRACE("reschedule(): suspending thread %ld\n", oldThread->id);
			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			thread_goes_away(oldThread);
			break;
		default:
			increase_penalty(oldThread);
			thread_goes_away(oldThread);
			TRACE("not enqueueing thread %ld into run queue next_state = %ld\n",
				oldThread->id, oldThread->next_state);
			break;
	}

	oldThread->has_yielded = false;
	schedulerOldThreadData->lost_cpu = false;

	// select thread with the biggest priority
	if (oldThread->cpu->disabled) {
		ASSERT(sPinnedRunQueues != NULL);
		nextThread = sPinnedRunQueues[thisCPU].PeekMaximum();
		if (nextThread != NULL)
			sPinnedRunQueues[thisCPU].Remove(nextThread);
		else {
			nextThread = sRunQueues[thisCore].GetHead(B_IDLE_PRIORITY);
			if (nextThread != NULL)
				sRunQueues[thisCore].Remove(nextThread);
		}
	} else
		nextThread = dequeue_thread(thisCPU);
	if (!nextThread)
		panic("reschedule(): run queues are empty!\n");

	TRACE("reschedule(): cpu %ld, next thread = %ld\n", thisCPU,
		nextThread->id);

	T(ScheduleThread(nextThread, oldThread));

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadScheduled,
		oldThread, nextThread);

	// update CPU heap
	update_priority_heaps(thisCPU,
		get_effective_priority(nextThread));

	nextThread->state = B_THREAD_RUNNING;
	nextThread->next_state = B_THREAD_READY;
	ASSERT(nextThread->scheduler_data->previous_core == thisCore);
	//nextThread->scheduler_data->previous_core = thisCore;

	// track kernel time (user time is tracked in thread_at_kernel_entry())
	scheduler_update_thread_times(oldThread, nextThread);

	// track CPU activity
	track_cpu_activity(oldThread, nextThread, thisCore);

	if (nextThread != oldThread || oldThread->cpu->preempted) {
		timer* quantumTimer = &oldThread->cpu->quantum_timer;
		if (!oldThread->cpu->preempted)
			cancel_timer(quantumTimer);

		oldThread->cpu->preempted = 0;
		if (!thread_is_idle_thread(nextThread)) {
			bigtime_t quantum = compute_quantum(oldThread);
			add_timer(quantumTimer, &reschedule_event, quantum,
				B_ONE_SHOT_RELATIVE_TIMER | B_TIMER_ACQUIRE_SCHEDULER_LOCK);
		} else
			nextThread->scheduler_data->quantum_start = system_time();

		if (nextThread != oldThread)
			scheduler_switch_thread(oldThread, nextThread);
	}
}


/*!	Runs the scheduler.
	Note: expects thread spinlock to be held
*/
void
scheduler_reschedule(void)
{
	if (!sSchedulerEnabled) {
		Thread* thread = thread_get_current_thread();
		if (thread != NULL && thread->next_state != B_THREAD_READY)
			panic("scheduler_reschedule_no_op() called in non-ready thread");
		return;
	}

	_scheduler_reschedule();
}


status_t
scheduler_on_thread_create(Thread* thread, bool idleThread)
{
	thread->scheduler_data = new (std::nothrow)scheduler_thread_data;
	if (thread->scheduler_data == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


void
scheduler_on_thread_init(Thread* thread)
{
	thread->scheduler_data->Init();
}


void
scheduler_on_thread_destroy(Thread* thread)
{
	delete thread->scheduler_data;
}


/*!	This starts the scheduler. Must be run in the context of the initial idle
	thread. Interrupts must be disabled and will be disabled when returning.
*/
void
scheduler_start(void)
{
	SpinLocker schedulerLocker(gSchedulerLock);

	_scheduler_reschedule();
}


status_t
scheduler_set_operation_mode(scheduler_mode mode)
{
	if (mode != SCHEDULER_MODE_PERFORMANCE
		&& mode != SCHEDULER_MODE_POWER_SAVING) {
		return B_BAD_VALUE;
	}

	const char* modeNames[] = { "performance", "power saving" };
	dprintf("scheduler: switching to %s mode\n", modeNames[mode]);

	InterruptsSpinLocker _(gSchedulerLock);

	sSchedulerMode = mode;
	switch (mode) {
		case SCHEDULER_MODE_PERFORMANCE:
			sDisableSmallTaskPacking = -1;
			sSmallTaskCore = -1;
			sChooseCore = choose_core_performance;
			break;

		case SCHEDULER_MODE_POWER_SAVING:
			sDisableSmallTaskPacking = 0;
			sSmallTaskCore = -1;
			sChooseCore = choose_core_power_saving;
			break;

		default:
			break;
	}

	return B_OK;
}


static void
traverse_topology_tree(cpu_topology_node* node, int packageID, int coreID)
{
	switch (node->level) {
		case CPU_TOPOLOGY_SMT:
			sCPUToCore[node->id] = coreID;
			sCPUToPackage[node->id] = packageID;
			return;

		case CPU_TOPOLOGY_CORE:
			coreID = node->id;
			break;

		case CPU_TOPOLOGY_PACKAGE:
			packageID = node->id;
			break;

		default:
			break;
	}

	for (int32 i = 0; i < node->children_count; i++)
		traverse_topology_tree(node->children[i], packageID, coreID);
}


static status_t
build_topology_mappings(int32& cpuCount, int32& coreCount, int32& packageCount)
{
	cpuCount = smp_get_num_cpus();

	sCPUToCore = new(std::nothrow) int32[cpuCount];
	if (sCPUToCore == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<int32> cpuToCoreDeleter(sCPUToCore);

	sCPUToPackage = new(std::nothrow) int32[cpuCount];
	if (sCPUToPackage == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<int32> cpuToPackageDeleter(sCPUToPackage);

	coreCount = 0;
	for (int32 i = 0; i < cpuCount; i++) {
		if (gCPU[i].topology_id[CPU_TOPOLOGY_SMT] == 0)
			coreCount++;
	}

	packageCount = 0;
	for (int32 i = 0; i < cpuCount; i++) {
		if (gCPU[i].topology_id[CPU_TOPOLOGY_SMT] == 0
			&& gCPU[i].topology_id[CPU_TOPOLOGY_CORE] == 0) {
			packageCount++;
		}
	}

	cpu_topology_node* root = get_cpu_topology();
	traverse_topology_tree(root, 0, 0);

	cpuToCoreDeleter.Detach();
	cpuToPackageDeleter.Detach();
	return B_OK;
}


static status_t
_scheduler_init()
{
	// create logical processor to core and package mappings
	int32 cpuCount, coreCount, packageCount;
	status_t result = build_topology_mappings(cpuCount, coreCount,
		packageCount);
	if (result != B_OK)
		return result;
	sRunQueueCount = coreCount;
	sSingleCore = coreCount == 1;

	// create package heap and idle package stack
	sPackageEntries = new(std::nothrow) PackageEntry[packageCount];
	if (sPackageEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<PackageEntry> packageEntriesDeleter(sPackageEntries);

	sPackageUsageHeap = new(std::nothrow) PackageHeap(packageCount);
	if (sPackageUsageHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<PackageHeap> packageHeapDeleter(sPackageUsageHeap);

	sIdlePackageList = new(std::nothrow) IdlePackageList;
	if (sIdlePackageList == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<IdlePackageList> packageListDeleter(sIdlePackageList);

	for (int32 i = 0; i < packageCount; i++) {
		sPackageEntries[i].fPackageID = i;
		sPackageEntries[i].fIdleCoreCount = coreCount / packageCount;
		sPackageEntries[i].fCoreCount = coreCount / packageCount;
		sIdlePackageList->Insert(&sPackageEntries[i]);
	}

	// create logical processor and core heaps
	sCPUEntries = new CPUEntry[cpuCount];
	if (sCPUEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<CPUEntry> cpuEntriesDeleter(sCPUEntries);

	sCoreEntries = new CoreEntry[coreCount];
	if (sCoreEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<CoreEntry> coreEntriesDeleter(sCoreEntries);

	sCorePriorityHeap = new CorePriorityHeap(coreCount);
	if (sCorePriorityHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<CorePriorityHeap> corePriorityHeapDeleter(sCorePriorityHeap);

	sCoreThreadHeap = new CoreThreadHeap;
	if (sCoreThreadHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<CoreThreadHeap> coreThreadHeapDeleter(sCoreThreadHeap);

	sCoreCPUBoundThreadHeap = new CoreThreadHeap(coreCount);
	if (sCoreCPUBoundThreadHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<CoreThreadHeap> coreCPUThreadHeapDeleter(
		sCoreCPUBoundThreadHeap);

	for (int32 i = 0; i < coreCount; i++) {
		sCoreEntries[i].fCoreID = i;
		sCoreEntries[i].fActiveTime = 0;
		sCoreEntries[i].fThreads = 0;
		sCoreEntries[i].fCPUBoundThreads = 0;
		sCoreEntries[i].fLoad = 0;

		status_t result = sCoreThreadHeap->Insert(&sCoreEntries[i], 0);
		if (result != B_OK)
			return result;

		result = sCorePriorityHeap->Insert(&sCoreEntries[i], B_IDLE_PRIORITY);
		if (result != B_OK)
			return result;
		sCorePriorityHeap->RemoveRoot();
	}

	sCPUPriorityHeaps = new CPUHeap[coreCount];
	if (sCPUPriorityHeaps == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<CPUHeap> cpuPriorityHeapDeleter(sCPUPriorityHeaps);

	for (int32 i = 0; i < cpuCount; i++) {
		sCPUEntries[i].fCPUNumber = i;

		sCPUEntries[i].fMeasureActiveTime = 0;
		sCPUEntries[i].fMeasureTime = 0;
		sCPUEntries[i].fLoad = 0;

		int32 core = sCPUToCore[i];

		int32 package = sCPUToPackage[i];
		if (sCPUPriorityHeaps[core].PeekMaximum() == NULL)
			sPackageEntries[package].fIdleCores.Insert(&sCoreEntries[core]);

		status_t result
			= sCPUPriorityHeaps[core].Insert(&sCPUEntries[i], B_IDLE_PRIORITY);
		if (result != B_OK)
			return result;
	}

	// create per-logical processor run queues for pinned threads
	TRACE("scheduler_init(): creating %" B_PRId32 " per-cpu queue%s\n",
		cpuCount, cpuCount != 1 ? "s" : "");

	sPinnedRunQueues = new(std::nothrow) ThreadRunQueue[cpuCount];
	if (sPinnedRunQueues == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<ThreadRunQueue> pinnedRunQueuesDeleter(sPinnedRunQueues);
	for (int i = 0; i < cpuCount; i++) {
		status_t result = sPinnedRunQueues[i].GetInitStatus();
		if (result != B_OK)
			return result;
	}

	// create per-core run queues
	TRACE("scheduler_init(): creating %" B_PRId32 " per-core queue%s\n",
		coreCount, coreCount != 1 ? "s" : "");

	sRunQueues = new(std::nothrow) ThreadRunQueue[coreCount];
	if (sRunQueues == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<ThreadRunQueue> runQueuesDeleter(sRunQueues);
	for (int i = 0; i < coreCount; i++) {
		status_t result = sRunQueues[i].GetInitStatus();
		if (result != B_OK)
			return result;
	}

#if 0
	scheduler_set_operation_mode(SCHEDULER_MODE_PERFORMANCE);
#else
	scheduler_set_operation_mode(SCHEDULER_MODE_POWER_SAVING);
#endif

	add_debugger_command_etc("run_queue", &dump_run_queue,
		"List threads in run queue", "\nLists threads in run queue", 0);
	add_debugger_command_etc("cpu_heap", &dump_cpu_heap,
		"List CPUs in CPU priority heap", "\nList CPUs in CPU priority heap",
		0);
	if (!sSingleCore) {
		add_debugger_command_etc("idle_cores", &dump_idle_cores,
			"List idle cores", "\nList idle cores", 0);
	}

	runQueuesDeleter.Detach();
	pinnedRunQueuesDeleter.Detach();
	coreCPUThreadHeapDeleter.Detach();
	coreThreadHeapDeleter.Detach();
	corePriorityHeapDeleter.Detach();
	cpuPriorityHeapDeleter.Detach();
	coreEntriesDeleter.Detach();
	cpuEntriesDeleter.Detach();
	packageEntriesDeleter.Detach();
	packageHeapDeleter.Detach();
	packageListDeleter.Detach();
	return B_OK;
}


void
scheduler_init(void)
{
	int32 cpuCount = smp_get_num_cpus();
	dprintf("scheduler_init: found %" B_PRId32 " logical cpu%s and %" B_PRId32
		" cache level%s\n", cpuCount, cpuCount != 1 ? "s" : "",
		gCPUCacheLevelCount, gCPUCacheLevelCount != 1 ? "s" : "");

	status_t result = _scheduler_init();
	if (result != B_OK)
		panic("scheduler_init: failed to initialize scheduler\n");

#if SCHEDULER_TRACING
	add_debugger_command_etc("scheduler", &cmd_scheduler,
		"Analyze scheduler tracing information",
		"<thread>\n"
		"Analyzes scheduler tracing information for a given thread.\n"
		"  <thread>  - ID of the thread.\n", 0);
#endif
}


void
scheduler_enable_scheduling(void)
{
	sSchedulerEnabled = true;
}


// #pragma mark - SchedulerListener


SchedulerListener::~SchedulerListener()
{
}


// #pragma mark - kernel private


/*!	Add the given scheduler listener. Thread lock must be held.
*/
void
scheduler_add_listener(struct SchedulerListener* listener)
{
	gSchedulerListeners.Add(listener);
}


/*!	Remove the given scheduler listener. Thread lock must be held.
*/
void
scheduler_remove_listener(struct SchedulerListener* listener)
{
	gSchedulerListeners.Remove(listener);
}


// #pragma mark - Syscalls


bigtime_t
_user_estimate_max_scheduling_latency(thread_id id)
{
	syscall_64_bit_return_value();

	// get the thread
	Thread* thread;
	if (id < 0) {
		thread = thread_get_current_thread();
		thread->AcquireReference();
	} else {
		thread = Thread::Get(id);
		if (thread == NULL)
			return 0;
	}
	BReference<Thread> threadReference(thread, true);

	// TODO: This is probably meant to be called periodically to return the
	// current estimate depending on the system usage; we return fixed estimates
	// per thread priority, though.

	if (thread->priority >= B_REAL_TIME_DISPLAY_PRIORITY)
		return kMinThreadQuantum / 4;
	if (thread->priority >= B_DISPLAY_PRIORITY)
		return kMinThreadQuantum;
	if (thread->priority < B_NORMAL_PRIORITY)
		return 2 * kMaxThreadQuantum;

	return 2 * kMinThreadQuantum;
}
