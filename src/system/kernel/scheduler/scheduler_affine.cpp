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
#include <util/Random.h>

#include "RunQueue.h"
#include "scheduler_common.h"
#include "scheduler_tracing.h"


//#define TRACE_SCHEDULER
#ifdef TRACE_SCHEDULER
#	define TRACE(...) dprintf_no_syslog(__VA_ARGS__)
#else
#	define TRACE(...) do { } while (false)
#endif


const bigtime_t kThreadQuantum = 1000;
const bigtime_t kMinThreadQuantum = 3000;
const bigtime_t kMaxThreadQuantum = 10000;

const bigtime_t kCacheExpire = 100000;

static bigtime_t sDisableSmallTaskPacking;

static scheduler_mode sSchedulerMode;

static int32 (*sChooseCore)(int32 priority);


// Heaps in sCPUPriorityHeaps are used for load balancing on a core the logical
// processors in the heap belong to. Since there are no cache affinity issues
// at this level and the run queue is shared among all logical processors on
// the core the only real concern is to make lower priority threads give way to
// the higher priority threads.
struct CPUEntry : public MinMaxHeapLinkImpl<CPUEntry, int32> {
	int32		fCPUNumber;
};
typedef MinMaxHeap<CPUEntry, int32> AffineCPUHeap;
static CPUEntry* sCPUEntries;
static AffineCPUHeap* sCPUPriorityHeaps;

struct CoreEntry : public DoublyLinkedListLinkImpl<CoreEntry> {
	HeapLink<CoreEntry, int32>	fPriorityHeapLink;
	MinMaxHeapLink<CoreEntry, int32>	fThreadHeapLink;

	int32		fCoreID;

	bigtime_t	fActiveTime;

	int32		fCPUBoundThreads;
	int32		fThreads;
};

static CoreEntry* sCoreEntries;
typedef Heap<CoreEntry, int32, HeapLesserCompare<int32>,
		HeapMemberGetLink<CoreEntry, int32, &CoreEntry::fPriorityHeapLink> >
	AffineCorePriorityHeap;
static AffineCorePriorityHeap* sCorePriorityHeap;

typedef MinMaxHeap<CoreEntry, int32, MinMaxHeapCompare<int32>,
		MinMaxHeapMemberGetLink<CoreEntry, int32, &CoreEntry::fThreadHeapLink> >
	AffineCoreThreadHeap;
static AffineCoreThreadHeap* sCoreThreadHeap;
static AffineCoreThreadHeap* sCoreCPUBoundThreadHeap;

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
typedef MinMaxHeap<PackageEntry, int32> AffinePackageHeap;
typedef DoublyLinkedList<PackageEntry> AffineIdlePackageList;

static PackageEntry* sPackageEntries;
static AffinePackageHeap* sPackageUsageHeap;
static AffineIdlePackageList* sIdlePackageList;

// The run queues. Holds the threads ready to run ordered by priority.
// One queue per schedulable target per core. Additionally, each
// logical processor has its sPinnedRunQueues used for scheduling
// pinned threads.
typedef RunQueue<Thread, THREAD_MAX_SET_PRIORITY> AffineRunQueue;
static AffineRunQueue* sRunQueues;
static AffineRunQueue* sPinnedRunQueues;
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
affine_get_minimal_priority(Thread* thread)
{
	return min_c(thread->priority, 25) / 5;
}


static inline int32
affine_get_thread_penalty(Thread* thread)
{
	int32 penalty = thread->scheduler_data->priority_penalty;

	const int kMinimalPriority = affine_get_minimal_priority(thread);
	if (kMinimalPriority > 0) {
		penalty
			+= thread->scheduler_data->additional_penalty % kMinimalPriority;
	}

	return penalty;
}


static inline int32
affine_get_effective_priority(Thread* thread)
{
	if (thread->priority == B_IDLE_PRIORITY)
		return thread->priority;
	if (thread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return thread->priority;

	int32 effectivePriority = thread->priority;
	effectivePriority -= affine_get_thread_penalty(thread);

	ASSERT(effectivePriority < B_FIRST_REAL_TIME_PRIORITY);
	ASSERT(effectivePriority >= B_LOWEST_ACTIVE_PRIORITY);

	return effectivePriority;
}


static void
dump_queue(AffineRunQueue::ConstIterator& iterator)
{
	if (!iterator.HasNext())
		kprintf("Run queue is empty.\n");
	else {
		kprintf("thread      id      priority penalty  name\n");
		while (iterator.HasNext()) {
			Thread* thread = iterator.Next();
			kprintf("%p  %-7" B_PRId32 " %-8" B_PRId32 " %-8" B_PRId32 " %s\n",
				thread, thread->id, thread->priority,
				affine_get_thread_penalty(thread), thread->name);
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

	AffineRunQueue::ConstIterator iterator;
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
dump_heap(AffineCPUHeap* heap)
{
	AffineCPUHeap temp(smp_get_num_cpus());

	kprintf("cpu priority actual priority\n");
	CPUEntry* entry = heap->PeekMinimum();
	while (entry) {
		int32 cpu = entry->fCPUNumber;
		int32 key = AffineCPUHeap::GetKey(entry);
		kprintf("%3" B_PRId32 " %8" B_PRId32 " %15" B_PRId32 "\n", cpu, key,
			affine_get_effective_priority(gCPU[cpu].running_thread));

		heap->RemoveMinimum();
		temp.Insert(entry, key);

		entry = heap->PeekMinimum();
	}

	entry = temp.PeekMinimum();
	while (entry) {
		int32 key = AffineCPUHeap::GetKey(entry);
		temp.RemoveMinimum();
		heap->Insert(entry, key);
		entry = temp.PeekMinimum();
	}
}


static void
dump_core_thread_heap(AffineCoreThreadHeap* heap)
{
	AffineCoreThreadHeap temp(sRunQueueCount);

	CoreEntry* entry = heap->PeekMinimum();
	while (entry) {
		int32 key = AffineCoreThreadHeap::GetKey(entry);
		kprintf("%4" B_PRId32 " %6" B_PRId32 " %7" B_PRId32 " %9" B_PRId32 "\n",
			entry->fCoreID, key, entry->fThreads, entry->fCPUBoundThreads);

		heap->RemoveMinimum();
		temp.Insert(entry, key);

		entry = heap->PeekMinimum();
	}

	entry = temp.PeekMinimum();
	while (entry) {
		int32 key = AffineCoreThreadHeap::GetKey(entry);
		temp.RemoveMinimum();
		heap->Insert(entry, key);
		entry = temp.PeekMinimum();
	}
}


static int
dump_cpu_heap(int argc, char** argv)
{
	AffineCorePriorityHeap temp(sRunQueueCount);

	CoreEntry* entry = sCorePriorityHeap->PeekRoot();
	if (entry != NULL)
		kprintf("core priority\n");
	else
		kprintf("No active cores.\n");

	while (entry) {
		int32 core = entry->fCoreID;
		int32 key = AffineCorePriorityHeap::GetKey(entry);
		kprintf("%4" B_PRId32 " %8" B_PRId32 "\n", core, key);

		sCorePriorityHeap->RemoveRoot();
		temp.Insert(entry, key);

		entry = sCorePriorityHeap->PeekRoot();
	}

	entry = temp.PeekRoot();
	while (entry) {
		int32 key = AffineCorePriorityHeap::GetKey(entry);
		temp.RemoveRoot();
		sCorePriorityHeap->Insert(entry, key);
		entry = temp.PeekRoot();
	}

	kprintf("\ncore    key threads cpu-bound\n");
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
	AffineIdlePackageList::ReverseIterator idleIterator
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

	AffinePackageHeap temp(smp_get_num_cpus());
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
		int32 key = AffinePackageHeap::GetKey(entry);
		temp.RemoveMinimum();
		sPackageUsageHeap->Insert(entry, key);
		entry = temp.PeekMinimum();
	}

	return 0;
}


static inline bool
affine_has_cache_expired(Thread* thread)
{
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


static void
affine_dump_thread_data(Thread* thread)
{
	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	kprintf("\tpriority_penalty:\t%" B_PRId32 "\n",
		schedulerThreadData->priority_penalty);

	int32 additionalPenalty = 0;
	const int kMinimalPriority = affine_get_minimal_priority(thread);
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
		&& affine_has_cache_expired(thread)) {
		kprintf("\tcache affinity has expired\n");
	}
}


static void
affine_update_thread_heaps(int32 core)
{
	CoreEntry* entry = &sCoreEntries[core];

	ASSERT(entry->fCPUBoundThreads >= 0
		&& entry->fCPUBoundThreads <= entry->fThreads);
	ASSERT(entry->fThreads >= 0
		&& entry->fThreads <= thread_max_threads());

	int32 newKey = entry->fCPUBoundThreads * thread_max_threads();
	newKey += entry->fThreads;

	int32 oldKey = AffineCoreThreadHeap::GetKey(entry);

	if (oldKey == newKey)
		return;

	if (newKey > thread_max_threads()) {
		if (oldKey <= thread_max_threads()) {
			sCoreThreadHeap->ModifyKey(entry, -1);
			ASSERT(sCoreThreadHeap->PeekMinimum() == entry);
			sCoreThreadHeap->RemoveMinimum();
			ASSERT(sCoreThreadHeap->PeekMinimum() != entry);

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
affine_increase_penalty(Thread* thread)
{
	if (thread->priority <= B_LOWEST_ACTIVE_PRIORITY)
		return;
	if (thread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return;

	TRACE("increasing thread %ld penalty\n", thread->id);

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	int32 oldPenalty = schedulerThreadData->priority_penalty++;

	ASSERT(thread->priority - oldPenalty >= B_LOWEST_ACTIVE_PRIORITY);
	const int kMinimalPriority = affine_get_minimal_priority(thread);
	if (thread->priority - oldPenalty <= kMinimalPriority) {
		int32 core = schedulerThreadData->previous_core;
		ASSERT(core >= 0);
		if (schedulerThreadData->additional_penalty == 0) {
			sCPUBoundThreads++;
			sCoreEntries[core].fCPUBoundThreads++;
			affine_update_thread_heaps(core);
		}

		schedulerThreadData->priority_penalty = oldPenalty;
		schedulerThreadData->additional_penalty++;
	}
}


static inline void
affine_cancel_penalty(Thread* thread)
{
	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	if (schedulerThreadData->priority_penalty != 0)
		TRACE("cancelling thread %ld penalty\n", thread->id);

	schedulerThreadData->priority_penalty = 0;
	schedulerThreadData->additional_penalty = 0;
}


static inline void
affine_update_priority_heaps(int32 cpu, int32 priority)
{
	int32 core = sCPUToCore[cpu];

	sCPUPriorityHeaps[core].ModifyKey(&sCPUEntries[cpu], priority);

	int32 maxPriority
		= AffineCPUHeap::GetKey(sCPUPriorityHeaps[core].PeekMaximum());
	int32 corePriority = AffineCorePriorityHeap::GetKey(&sCoreEntries[core]);

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
affine_choose_core_performance(int32 priority)
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
		if (AffineCorePriorityHeap::GetKey(entry) >= priority) {
			entry = sCoreThreadHeap->PeekMinimum();
			if (entry == NULL)
				entry = sCoreCPUBoundThreadHeap->PeekMinimum();
		}
	}

	ASSERT(entry != NULL);
	return entry->fCoreID;
}


static int32
affine_choose_core_power_saving(int32 priority)
{
	CoreEntry* entry;

	entry = sCorePriorityHeap->PeekRoot();
	if (entry != NULL && AffineCorePriorityHeap::GetKey(entry) < priority) {
		// run immediately on already woken core
	} else if (sDisableSmallTaskPacking < system_time()
		&& sCoreThreadHeap->PeekMaximum() != NULL) {
		// try to pack all threads on one core
		entry = sCoreThreadHeap->PeekMaximum();
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
affine_choose_core(int32 priority)
{
	return sChooseCore(priority);
}


static inline int32
affine_choose_cpu(int32 core)
{
	CPUEntry* entry = sCPUPriorityHeaps[core].PeekMinimum();
	ASSERT(entry != NULL);
	return entry->fCPUNumber;
}


static bool
affine_should_rebalance(Thread* thread)
{
	if (thread_is_idle_thread(thread))
		return false;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	ASSERT(schedulerThreadData->previous_core >= 0);

	CoreEntry* coreEntry = &sCoreEntries[schedulerThreadData->previous_core];

	// If this is a cpu bound thread and we have significantly more such threads
	//  than the average get rid of this one.
	if (schedulerThreadData->additional_penalty != 0) {
		int32 averageCPUBound = sCPUBoundThreads / sRunQueueCount;
		if (coreEntry->fCPUBoundThreads - averageCPUBound > 1)
			return true;
		return false;
	}

	// If this thread is not cpu bound but we have at least one consider giving
	// this one to someone less busy.
	int32 averageThread = sAssignedThreads / sRunQueueCount;
	if (coreEntry->fCPUBoundThreads > 0) {
		CoreEntry* other = sCoreThreadHeap->PeekMinimum();
		if (other != NULL
			&& AffineCoreThreadHeap::GetKey(other) <= averageThread) {
			return true;
		}
	}

	// No cpu bound threads - the situation is quite good. Make sure it
	// won't get much worse...
	const int32 kBalanceThreshold = 3;
	return coreEntry->fThreads - averageThread > kBalanceThreshold;
}


static void
affine_assign_active_thread_to_core(Thread* thread)
{
	if (thread_is_idle_thread(thread))
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

	affine_update_thread_heaps(core);
}


static inline void
affine_thread_goes_away(Thread* thread)
{
	if (thread_is_idle_thread(thread))
		return;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	ASSERT(schedulerThreadData->previous_core >= 0);
	int32 core = schedulerThreadData->previous_core;

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

	affine_update_thread_heaps(core);

	schedulerThreadData->went_sleep = system_time();
	schedulerThreadData->went_sleep_active = sCoreEntries[core].fActiveTime;
}


static void
affine_enqueue(Thread* thread, bool newOne)
{
	ASSERT(thread != NULL);

	thread->state = thread->next_state = B_THREAD_READY;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	bigtime_t hasSlept = system_time() - schedulerThreadData->went_sleep;
	if (newOne && hasSlept > kThreadQuantum)
		affine_cancel_penalty(thread);

	int32 threadPriority = affine_get_effective_priority(thread);

	T(EnqueueThread(thread, threadPriority));

	bool pinned = thread->pinned_to_cpu > 0;
	int32 targetCPU = -1;
	int32 targetCore;
	if (pinned) {
		targetCPU = thread->previous_cpu->cpu_num;
		targetCore = sCPUToCore[targetCPU];
		ASSERT(targetCore == schedulerThreadData->previous_core);

		if (newOne)
			affine_assign_active_thread_to_core(thread);
	} else if (schedulerThreadData->previous_core < 0
		|| (newOne && affine_has_cache_expired(thread))
		|| affine_should_rebalance(thread)) {

		if (thread_is_idle_thread(thread)) {
			targetCPU = thread->previous_cpu->cpu_num;
			targetCore = sCPUToCore[targetCPU];
		} else {
			if (!newOne)
				affine_thread_goes_away(thread);

			targetCore = affine_choose_core(threadPriority);
			targetCPU = affine_choose_cpu(targetCore);
		}

		schedulerThreadData->previous_core = targetCore;
		affine_assign_active_thread_to_core(thread);
	} else {
		targetCore = schedulerThreadData->previous_core;
		targetCPU = affine_choose_cpu(targetCore);
		if (newOne)
			affine_assign_active_thread_to_core(thread);
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
	int32 targetPriority = affine_get_effective_priority(targetThread);

	TRACE("choosing CPU %ld with current priority %ld\n", targetCPU,
		targetPriority);

	if (threadPriority > targetPriority) {
		targetThread->scheduler_data->lost_cpu = true;

		// It is possible that another CPU schedules the thread before the
		// target CPU. However, since the target CPU is sent an ICI it will
		// reschedule anyway and update its heap key to the correct value.
		affine_update_priority_heaps(targetCPU, threadPriority);

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
static void
affine_enqueue_in_run_queue(Thread *thread)
{
	TRACE("enqueueing new thread %ld with static priority %ld\n", thread->id,
		thread->priority);
	affine_enqueue(thread, true);
}


static inline void
affine_put_back(Thread* thread)
{
	bool pinned = sPinnedRunQueues != NULL && thread->pinned_to_cpu > 0;

	if (pinned) {
		int32 pinnedCPU = thread->previous_cpu->cpu_num;
		sPinnedRunQueues[pinnedCPU].PushFront(thread,
			affine_get_effective_priority(thread));
	} else {
		int32 previousCore = thread->scheduler_data->previous_core;
		ASSERT(previousCore >= 0);
		sRunQueues[previousCore].PushFront(thread,
			affine_get_effective_priority(thread));
	}
}


/*!	Sets the priority of a thread.
	Note: thread lock must be held when entering this function
*/
static void
affine_set_thread_priority(Thread *thread, int32 priority)
{
	if (priority == thread->priority)
		return;

	TRACE("changing thread %ld priority to %ld (old: %ld, effective: %ld)\n",
		thread->id, priority, thread->priority,
		affine_get_effective_priority(thread));

	if (thread->state == B_THREAD_RUNNING)
		affine_thread_goes_away(thread);

	if (thread->state != B_THREAD_READY) {
		affine_cancel_penalty(thread);
		thread->priority = priority;

		if (thread->state == B_THREAD_RUNNING) {
			affine_assign_active_thread_to_core(thread);
			affine_update_priority_heaps(thread->cpu->cpu_num, priority);
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
	affine_thread_goes_away(thread);

	// set priority and re-insert
	affine_cancel_penalty(thread);
	thread->priority = priority;

	affine_enqueue_in_run_queue(thread);
}


static bigtime_t
affine_estimate_max_scheduling_latency(Thread* thread)
{
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
affine_quantum_ended(Thread* thread, bool wasPreempted, bool hasYielded)
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
affine_quantum_linear_interpolation(bigtime_t maxQuantum, bigtime_t minQuantum,
	int32 maxPriority, int32 minPriority, int32 priority)
{
	ASSERT(priority <= maxPriority);
	ASSERT(priority >= minPriority);

	bigtime_t result = (maxQuantum - minQuantum) * (priority - minPriority);
	result /= maxPriority - minPriority;
	return maxQuantum - result;
}


static inline bigtime_t
affine_get_base_quantum(Thread* thread)
{
	int32 priority = affine_get_effective_priority(thread);

	if (priority >= B_URGENT_DISPLAY_PRIORITY)
		return kThreadQuantum;
	if (priority > B_NORMAL_PRIORITY) {
		return affine_quantum_linear_interpolation(kThreadQuantum * 4,
			kThreadQuantum, B_URGENT_DISPLAY_PRIORITY, B_NORMAL_PRIORITY,
			priority);
	}
	return affine_quantum_linear_interpolation(kThreadQuantum * 64,
		kThreadQuantum * 4, B_NORMAL_PRIORITY, B_IDLE_PRIORITY, priority);
}


static inline bigtime_t
affine_compute_quantum(Thread* thread)
{
	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	bigtime_t quantum;
	if (schedulerThreadData->time_left != 0)
		quantum = schedulerThreadData->time_left;
	else
		quantum = affine_get_base_quantum(thread);

	quantum += schedulerThreadData->stolen_time;
	schedulerThreadData->stolen_time = 0;

	schedulerThreadData->time_left = quantum;
	schedulerThreadData->quantum_start = system_time();

	return quantum;
}


static inline Thread*
affine_dequeue_thread(int32 thisCPU)
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
		pinnedPriority = affine_get_effective_priority(pinnedThread);

	int32 sharedPriority = -1;
	if (sharedThread != NULL)
		sharedPriority = affine_get_effective_priority(sharedThread);

	if (sharedPriority > pinnedPriority) {
		sRunQueues[thisCore].Remove(sharedThread);
		return sharedThread;
	}

	sPinnedRunQueues[thisCPU].Remove(pinnedThread);
	return pinnedThread;
}


static inline void
affine_track_cpu_activity(Thread* oldThread, Thread* nextThread, int32 thisCore)
{
	if (!thread_is_idle_thread(oldThread)) {
		bigtime_t active
			= (oldThread->kernel_time - oldThread->cpu->last_kernel_time)
				+ (oldThread->user_time - oldThread->cpu->last_user_time);

		oldThread->cpu->active_time += active;
		sCoreEntries[thisCore].fActiveTime += active;
	}

	if (!thread_is_idle_thread(nextThread)) {
		oldThread->cpu->last_kernel_time = nextThread->kernel_time;
		oldThread->cpu->last_user_time = nextThread->user_time;
	}
}


/*!	Runs the scheduler.
	Note: expects thread spinlock to be held
*/
static void
affine_reschedule(void)
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
	if (nextThread != NULL) {
		affine_update_priority_heaps(thisCPU,
			affine_get_effective_priority(nextThread));
	}

	switch (oldThread->next_state) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			if (!schedulerOldThreadData->lost_cpu)
				schedulerOldThreadData->cpu_bound = false;

			if (affine_quantum_ended(oldThread, oldThread->cpu->preempted,
					oldThread->has_yielded)) {
				if (schedulerOldThreadData->cpu_bound)
					affine_increase_penalty(oldThread);

				TRACE("enqueueing thread %ld into run queue priority = %ld\n",
					oldThread->id, affine_get_effective_priority(oldThread));
				affine_enqueue(oldThread, false);
			} else {
				TRACE("putting thread %ld back in run queue priority = %ld\n",
					oldThread->id, affine_get_effective_priority(oldThread));
				affine_put_back(oldThread);
			}

			break;
		case B_THREAD_SUSPENDED:
			affine_thread_goes_away(oldThread);
			TRACE("reschedule(): suspending thread %ld\n", oldThread->id);
			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			affine_thread_goes_away(oldThread);
			break;
		default:
			affine_thread_goes_away(oldThread);
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
		nextThread = affine_dequeue_thread(thisCPU);
	if (!nextThread)
		panic("reschedule(): run queues are empty!\n");

	TRACE("reschedule(): cpu %ld, next thread = %ld\n", thisCPU,
		nextThread->id);

	T(ScheduleThread(nextThread, oldThread));

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadScheduled,
		oldThread, nextThread);

	// update CPU heap
	affine_update_priority_heaps(thisCPU,
		affine_get_effective_priority(nextThread));

	nextThread->state = B_THREAD_RUNNING;
	nextThread->next_state = B_THREAD_READY;
	ASSERT(nextThread->scheduler_data->previous_core == thisCore);
	//nextThread->scheduler_data->previous_core = thisCore;

	// track kernel time (user time is tracked in thread_at_kernel_entry())
	scheduler_update_thread_times(oldThread, nextThread);

	// track CPU activity
	affine_track_cpu_activity(oldThread, nextThread, thisCore);

	if (nextThread != oldThread || oldThread->cpu->preempted) {
		timer* quantumTimer = &oldThread->cpu->quantum_timer;
		if (!oldThread->cpu->preempted)
			cancel_timer(quantumTimer);

		oldThread->cpu->preempted = 0;
		if (!thread_is_idle_thread(nextThread)) {
			bigtime_t quantum = affine_compute_quantum(oldThread);
			add_timer(quantumTimer, &reschedule_event, quantum,
				B_ONE_SHOT_RELATIVE_TIMER | B_TIMER_ACQUIRE_SCHEDULER_LOCK);
		}

		if (nextThread != oldThread)
			scheduler_switch_thread(oldThread, nextThread);
	}
}


static status_t
affine_on_thread_create(Thread* thread, bool idleThread)
{
	thread->scheduler_data = new (std::nothrow)scheduler_thread_data;
	if (thread->scheduler_data == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


static void
affine_on_thread_init(Thread* thread)
{
	thread->scheduler_data->Init();
}


static void
affine_on_thread_destroy(Thread* thread)
{
	delete thread->scheduler_data;
}


/*!	This starts the scheduler. Must be run in the context of the initial idle
	thread. Interrupts must be disabled and will be disabled when returning.
*/
static void
affine_start(void)
{
	SpinLocker schedulerLocker(gSchedulerLock);

	affine_reschedule();
}


static status_t
affine_set_operation_mode(scheduler_mode mode)
{
	if (mode != SCHEDULER_MODE_PERFORMANCE
		&& mode != SCHEDULER_MODE_POWER_SAVING) {
		return B_BAD_VALUE;
	}

#ifdef TRACE_SCHEDULER
	const char* modeNames = { "performance", "power saving" };
#endif
	TRACE("switching scheduler to %s mode\n", modeNames[mode]);

	sSchedulerMode = mode;
	switch (mode) {
		case SCHEDULER_MODE_PERFORMANCE:
			sChooseCore = affine_choose_core_performance;
			break;

		case SCHEDULER_MODE_POWER_SAVING:
			sChooseCore = affine_choose_core_power_saving;
			break;

		default:
			break;
	}

	return B_OK;
}


static scheduler_ops kAffineOps = {
	affine_enqueue_in_run_queue,
	affine_reschedule,
	affine_set_thread_priority,
	affine_estimate_max_scheduling_latency,
	affine_on_thread_create,
	affine_on_thread_init,
	affine_on_thread_destroy,
	affine_start,
	affine_set_operation_mode,
	affine_dump_thread_data
};


// #pragma mark -


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


status_t
scheduler_affine_init()
{
	// create logical processor to core and package mappings
	int32 cpuCount, coreCount, packageCount;
	status_t result = build_topology_mappings(cpuCount, coreCount,
		packageCount);
	if (result != B_OK)
		return result;
	sRunQueueCount = coreCount;

	// create package heap and idle package stack
	sPackageEntries = new(std::nothrow) PackageEntry[packageCount];
	if (sPackageEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<PackageEntry> packageEntriesDeleter(sPackageEntries);

	sPackageUsageHeap = new(std::nothrow) AffinePackageHeap(packageCount);
	if (sPackageUsageHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<AffinePackageHeap> packageHeapDeleter(sPackageUsageHeap);

	sIdlePackageList = new(std::nothrow) AffineIdlePackageList;
	if (sIdlePackageList == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<AffineIdlePackageList> packageListDeleter(sIdlePackageList);

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
	ArrayDeleter<CoreEntry> coreEntriesDeleter(
		sCoreEntries);

	sCorePriorityHeap = new AffineCorePriorityHeap(coreCount);
	if (sCorePriorityHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<AffineCorePriorityHeap> corePriorityHeapDeleter(
		sCorePriorityHeap);

	sCoreThreadHeap = new AffineCoreThreadHeap;
	if (sCoreThreadHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<AffineCoreThreadHeap> coreThreadHeapDeleter(sCoreThreadHeap);

	sCoreCPUBoundThreadHeap = new AffineCoreThreadHeap(coreCount);
	if (sCoreCPUBoundThreadHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<AffineCoreThreadHeap> coreCPUThreadHeapDeleter(
		sCoreCPUBoundThreadHeap);

	for (int32 i = 0; i < coreCount; i++) {
		sCoreEntries[i].fCoreID = i;
		sCoreEntries[i].fActiveTime = 0;
		sCoreEntries[i].fThreads = 0;
		sCoreEntries[i].fCPUBoundThreads = 0;

		status_t result = sCoreThreadHeap->Insert(&sCoreEntries[i], 0);
		if (result != B_OK)
			return result;

		result = sCorePriorityHeap->Insert(&sCoreEntries[i], B_IDLE_PRIORITY);
		if (result != B_OK)
			return result;
		sCorePriorityHeap->RemoveRoot();
	}

	sCPUPriorityHeaps = new AffineCPUHeap[coreCount];
	if (sCPUPriorityHeaps == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<AffineCPUHeap> cpuPriorityHeapDeleter(sCPUPriorityHeaps);

	for (int32 i = 0; i < cpuCount; i++) {
		sCPUEntries[i].fCPUNumber = i;
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
	TRACE("scheduler_affine_init(): creating %" B_PRId32 " per-cpu queue%s\n",
		cpuCount, cpuCount != 1 ? "s" : "");

	sPinnedRunQueues = new(std::nothrow) AffineRunQueue[cpuCount];
	if (sPinnedRunQueues == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<AffineRunQueue> pinnedRunQueuesDeleter(sPinnedRunQueues);
	for (int i = 0; i < cpuCount; i++) {
		status_t result = sPinnedRunQueues[i].GetInitStatus();
		if (result != B_OK)
			return result;
	}

	// create per-core run queues
	TRACE("scheduler_affine_init(): creating %" B_PRId32 " per-core queue%s\n",
		coreCount, coreCount != 1 ? "s" : "");

	sRunQueues = new(std::nothrow) AffineRunQueue[coreCount];
	if (sRunQueues == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<AffineRunQueue> runQueuesDeleter(sRunQueues);
	for (int i = 0; i < coreCount; i++) {
		status_t result = sRunQueues[i].GetInitStatus();
		if (result != B_OK)
			return result;
	}

#if 1
	affine_set_operation_mode(SCHEDULER_MODE_POWER_SAVING);
#else
	affine_set_operation_mode(SCHEDULER_MODE_PERFORMANCE);
#endif
	gScheduler = &kAffineOps;

	add_debugger_command_etc("run_queue", &dump_run_queue,
		"List threads in run queue", "\nLists threads in run queue", 0);
	add_debugger_command_etc("cpu_heap", &dump_cpu_heap,
		"List CPUs in CPU priority heap", "\nList CPUs in CPU priority heap",
		0);
	add_debugger_command_etc("idle_cores", &dump_idle_cores,
		"List idle cores", "\nList idle cores", 0);

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
