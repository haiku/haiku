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
#include <load_tracking.h>
#include <scheduler_defs.h>
#include <smp.h>
#include <thread.h>
#include <timer.h>
#include <util/Heap.h>
#include <util/MinMaxHeap.h>

#include <cpufreq.h>

#include "RunQueue.h"
#include "scheduler_common.h"
#include "scheduler_tracing.h"


//#define TRACE_SCHEDULER
#ifdef TRACE_SCHEDULER
#	define TRACE(...) dprintf_no_syslog(__VA_ARGS__)
#else
#	define TRACE(...) do { } while (false)
#endif


#define CACHE_LINE_ALIGN	 __attribute__((aligned(64)))


SchedulerListenerList gSchedulerListeners;
spinlock gSchedulerListenersLock = B_SPINLOCK_INITIALIZER;

static bool sSchedulerEnabled;

const bigtime_t kThreadQuantum = 1000;
const bigtime_t kMinThreadQuantum = 3000;
const bigtime_t kMaxThreadQuantum = 10000;

const bigtime_t kMinimalWaitTime = kThreadQuantum / 4;

const bigtime_t kCacheExpire = 100000;

const int kTargetLoad = kMaxLoad * 55 / 100;
const int kHighLoad = kMaxLoad * 70 / 100;
const int kVeryHighLoad = (kMaxLoad + kHighLoad) / 2;
const int kLoadDifference = kMaxLoad * 20 / 100;
const int kLowLoad = kLoadDifference / 2;

static bigtime_t sDisableSmallTaskPacking;
static int32 sSmallTaskCore;

static bool sSingleCore;

static scheduler_mode sSchedulerMode;
static rw_spinlock sSchedulerModeLock = B_RW_SPINLOCK_INITIALIZER;

static int32 (*sChooseCore)(Thread* thread);
static bool (*sShouldRebalance)(Thread* thread);
static void (*sRebalanceIRQs)(void);


// Heaps in sCPUPriorityHeaps are used for load balancing on a core the logical
// processors in the heap belong to. Since there are no cache affinity issues
// at this level and the run queue is shared among all logical processors on
// the core the only real concern is to make lower priority threads give way to
// the higher priority threads.
struct CPUEntry : public MinMaxHeapLinkImpl<CPUEntry, int32> {
				CPUEntry();

	int32		fCPUNumber;

	int32		fPriority;

	bigtime_t	fMeasureActiveTime;
	bigtime_t	fMeasureTime;

	int32		fLoad;
} CACHE_LINE_ALIGN;
typedef MinMaxHeap<CPUEntry, int32> CPUHeap CACHE_LINE_ALIGN;

static CPUEntry* sCPUEntries;
static CPUHeap* sCPUPriorityHeaps;

struct CoreEntry : public MinMaxHeapLinkImpl<CoreEntry, int32>,
	DoublyLinkedListLinkImpl<CoreEntry> {
				CoreEntry();

	int32		fCoreID;

	spinlock	fLock;

	bigtime_t	fStartedBottom;
	bigtime_t	fReachedBottom;
	bigtime_t	fStartedIdle;
	bigtime_t	fReachedIdle;

	bigtime_t	fActiveTime;

	int32		fLoad;
} CACHE_LINE_ALIGN;
typedef MinMaxHeap<CoreEntry, int32> CoreLoadHeap;

static CoreEntry* sCoreEntries;
static CoreLoadHeap* sCoreLoadHeap;
static CoreLoadHeap* sCoreHighLoadHeap;
static spinlock sCoreHeapsLock = B_SPINLOCK_INITIALIZER;

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
								PackageEntry();

	int32						fPackageID;

	DoublyLinkedList<CoreEntry>	fIdleCores;
	int32						fIdleCoreCount;

	int32						fCoreCount;
} CACHE_LINE_ALIGN;
typedef MinMaxHeap<PackageEntry, int32> PackageHeap;
typedef DoublyLinkedList<PackageEntry> IdlePackageList;

static PackageEntry* sPackageEntries;
static PackageHeap* sPackageUsageHeap;
static IdlePackageList* sIdlePackageList;
static spinlock sIdlePackageLock = B_SPINLOCK_INITIALIZER;

// The run queues. Holds the threads ready to run ordered by priority.
// One queue per schedulable target per core. Additionally, each
// logical processor has its sPinnedRunQueues used for scheduling
// pinned threads.
typedef RunQueue<Thread, THREAD_MAX_SET_PRIORITY> ThreadRunQueue;

static ThreadRunQueue* sRunQueues CACHE_LINE_ALIGN;
static ThreadRunQueue* sPinnedRunQueues CACHE_LINE_ALIGN;
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

			bigtime_t	measure_active_time;
			bigtime_t	measure_time;
			int32		load;

			bigtime_t	went_sleep;
			bigtime_t	went_sleep_active;

			int32		previous_core;

			bool		enqueued;
};


CPUEntry::CPUEntry()
	:
	fPriority(B_IDLE_PRIORITY),
	fMeasureActiveTime(0),
	fMeasureTime(0),
	fLoad(0)
{
}


CoreEntry::CoreEntry()
	:
	fActiveTime(0),
	fLoad(0)
{
	B_INITIALIZE_SPINLOCK(&fLock);
}


PackageEntry::PackageEntry()
	:
	fIdleCoreCount(0),
	fCoreCount(0)
{
}


void
scheduler_thread_data::Init()
{
	priority_penalty = 0;
	additional_penalty = 0;

	time_left = 0;
	stolen_time = 0;

	measure_active_time = 0;
	measure_time = 0;
	load = 0;

	went_sleep = 0;
	went_sleep_active = 0;

	lost_cpu = false;
	cpu_bound = true;

	previous_core = -1;
	enqueued = false;
}


static inline int
get_minimal_priority(Thread* thread)
{
	return max_c(min_c(thread->priority, 25) / 5, 1);
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
		kprintf("%3" B_PRId32 " %8" B_PRId32 " %3" B_PRId32 "%%\n", cpu, key,
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
dump_core_load_heap(CoreLoadHeap* heap)
{
	CoreLoadHeap temp(sRunQueueCount);
	int32 cpuPerCore = smp_get_num_cpus() / sRunQueueCount;

	CoreEntry* entry = heap->PeekMinimum();
	while (entry) {
		int32 key = CoreLoadHeap::GetKey(entry);
		kprintf("%4" B_PRId32 " %3" B_PRId32 "%%\n", entry->fCoreID,
			entry->fLoad / cpuPerCore / 10);

		heap->RemoveMinimum();
		temp.Insert(entry, key);

		entry = heap->PeekMinimum();
	}

	entry = temp.PeekMinimum();
	while (entry) {
		int32 key = CoreLoadHeap::GetKey(entry);
		temp.RemoveMinimum();
		heap->Insert(entry, key);
		entry = temp.PeekMinimum();
	}
}


static int
dump_cpu_heap(int argc, char** argv)
{
	kprintf("\ncore load\n");
	dump_core_load_heap(sCoreLoadHeap);
	kprintf("---------\n");
	dump_core_load_heap(sCoreHighLoadHeap);

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
		case SCHEDULER_MODE_LOW_LATENCY:
			return atomic_get64(&coreEntry->fActiveTime)
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
	kprintf("\tload:\t\t\t%" B_PRId32 "%%\n", schedulerThreadData->load / 10);
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
update_load_heaps(int32 core)
{
	ASSERT(!sSingleCore);

	CoreEntry* entry = &sCoreEntries[core];

	SpinLocker coreLocker(sCoreHeapsLock);

	int32 cpuPerCore = smp_get_num_cpus() / sRunQueueCount;
	int32 newKey = entry->fLoad / cpuPerCore;
	int32 oldKey = CoreLoadHeap::GetKey(entry);

	ASSERT(oldKey >= 0 && oldKey <= kMaxLoad);
	ASSERT(newKey >= 0 && newKey <= kMaxLoad);

	if (oldKey == newKey)
		return;

	if (newKey > kHighLoad) {
		if (oldKey <= kHighLoad) {
			sCoreLoadHeap->ModifyKey(entry, -1);
			ASSERT(sCoreLoadHeap->PeekMinimum() == entry);
			sCoreLoadHeap->RemoveMinimum();

			sCoreHighLoadHeap->Insert(entry, newKey);
		} else
			sCoreHighLoadHeap->ModifyKey(entry, newKey);
	} else {
		if (oldKey > kHighLoad) {
			sCoreHighLoadHeap->ModifyKey(entry, -1);
			ASSERT(sCoreHighLoadHeap->PeekMinimum() == entry);
			sCoreHighLoadHeap->RemoveMinimum();

			sCoreLoadHeap->Insert(entry, newKey);
		} else
			sCoreLoadHeap->ModifyKey(entry, newKey);
	}
}


static inline bool
is_small_task_packing_enabled(void)
{
	if (sDisableSmallTaskPacking == -1)
		return false;
	return sDisableSmallTaskPacking < system_time();
}


static inline void
disable_small_task_packing(void)
{
	ASSERT(!sSingleCore);

	ASSERT(is_small_task_packing_enabled());
	ASSERT(sSmallTaskCore == sCPUToCore[smp_get_current_cpu()]);

	sDisableSmallTaskPacking = system_time() + kThreadQuantum * 100;
	sSmallTaskCore = -1;
}


static inline void
increase_penalty(Thread* thread)
{
	if (thread->priority < B_LOWEST_ACTIVE_PRIORITY)
		return;
	if (thread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return;

	TRACE("increasing thread %ld penalty\n", thread->id);

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	int32 oldPenalty = schedulerThreadData->priority_penalty++;

	ASSERT(thread->priority - oldPenalty >= B_LOWEST_ACTIVE_PRIORITY);

	const int kMinimalPriority = get_minimal_priority(thread);
	if (thread->priority - oldPenalty <= kMinimalPriority) {
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
update_cpu_priority(int32 cpu, int32 priority)
{
	int32 core = sCPUToCore[cpu];

	int32 corePriority = CPUHeap::GetKey(sCPUPriorityHeaps[core].PeekMaximum());

	sCPUEntries[cpu].fPriority = priority;
	sCPUPriorityHeaps[core].ModifyKey(&sCPUEntries[cpu], priority);

	if (sSingleCore)
		return;

	int32 maxPriority
		= CPUHeap::GetKey(sCPUPriorityHeaps[core].PeekMaximum());

	if (corePriority == maxPriority)
		return;

	int32 package = sCPUToPackage[cpu];
	PackageEntry* packageEntry = &sPackageEntries[package];
	if (maxPriority == B_IDLE_PRIORITY) {
		SpinLocker _(sIdlePackageLock);

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
		SpinLocker _(sIdlePackageLock);

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


static int32
choose_core_low_latency(Thread* thread)
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
		entry = sCoreLoadHeap->PeekMinimum();
		if (entry == NULL)
			entry = sCoreHighLoadHeap->PeekMinimum();
	}

	ASSERT(entry != NULL);
	return entry->fCoreID;
}


static inline bool
is_task_small(Thread* thread)
{
	return thread->scheduler_data->load <= 200;
}


static int32
choose_core_power_saving(Thread* thread)
{
	CoreEntry* entry;

	if (is_small_task_packing_enabled() && is_task_small(thread)
		&& sCoreLoadHeap->PeekMaximum() != NULL) {
		// try to pack all threads on one core
		if (sSmallTaskCore < 0)
			sSmallTaskCore = sCoreLoadHeap->PeekMaximum()->fCoreID;
		entry = &sCoreEntries[sSmallTaskCore];
	} else if (sCoreLoadHeap->PeekMinimum() != NULL) {
		// run immediately on already woken core
		entry = sCoreLoadHeap->PeekMinimum();
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
		entry = sCoreLoadHeap->PeekMinimum();
		if (entry == NULL)
			entry = sCoreHighLoadHeap->PeekMinimum();
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
choose_core_and_cpu(Thread* thread, int32& targetCore, int32& targetCPU)
{
	SpinLocker coreLocker(sCoreHeapsLock);

	if (targetCore == -1 && targetCPU != -1)
		targetCore = sCPUToCore[targetCPU];
	else if (targetCore != -1 && targetCPU == -1)
		targetCPU = choose_cpu(targetCore);
	else if (targetCore == -1 && targetCPU == -1) {
		targetCore = choose_core(thread);
		targetCPU = choose_cpu(targetCore);
	}

	ASSERT(targetCore >= 0 && targetCore < sRunQueueCount);
	ASSERT(targetCPU >= 0 && targetCPU < smp_get_num_cpus());

	int32 targetPriority = sCPUEntries[targetCPU].fPriority;
	int32 threadPriority = get_effective_priority(thread);

	if (threadPriority > targetPriority) {
		// It is possible that another CPU schedules the thread before the
		// target CPU. However, since the target CPU is sent an ICI it will
		// reschedule anyway and update its heap key to the correct value.
		update_cpu_priority(targetCPU, threadPriority);
		return true;
	}

	return false;
}


static bool
should_rebalance_low_latency(Thread* thread)
{
	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	ASSERT(schedulerThreadData->previous_core >= 0);

	CoreEntry* coreEntry = &sCoreEntries[schedulerThreadData->previous_core];

	// If the thread produces more than 50% of the load, leave it here. In
	// such situation it is better to move other threads away.
	if (schedulerThreadData->load >= coreEntry->fLoad / 2)
		return false;

	// If there is high load on this core but this thread does not contribute
	// significantly consider giving it to someone less busy.
	if (coreEntry->fLoad > kHighLoad) {
		SpinLocker coreLocker(sCoreHeapsLock);

		CoreEntry* other = sCoreLoadHeap->PeekMinimum();
		if (other != NULL && coreEntry->fLoad - other->fLoad >= kLoadDifference)
			return true;
	}

	// No cpu bound threads - the situation is quite good. Make sure it
	// won't get much worse...
	SpinLocker coreLocker(sCoreHeapsLock);

	CoreEntry* other = sCoreLoadHeap->PeekMinimum();
	if (other == NULL)
		other = sCoreHighLoadHeap->PeekMinimum();
	return coreEntry->fLoad - other->fLoad >= kLoadDifference * 2;
}


static bool
should_rebalance_power_saving(Thread* thread)
{
	ASSERT(!sSingleCore);

	if (thread_is_idle_thread(thread))
		return false;

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	ASSERT(schedulerThreadData->previous_core >= 0);

	int32 core = schedulerThreadData->previous_core;
	CoreEntry* coreEntry = &sCoreEntries[core];

	// If the thread produces more than 50% of the load, leave it here. In
	// such situation it is better to move other threads away.
	// Unless we are trying to pack small tasks here, in such case get rid
	// of CPU hungry thread and continue packing.
	if (schedulerThreadData->load >= coreEntry->fLoad / 2)
		return is_small_task_packing_enabled() && sSmallTaskCore == core;

	// All cores try to give us small tasks, check whether we have enough.
	if (is_small_task_packing_enabled() && sSmallTaskCore == core) {
		if (coreEntry->fLoad > kHighLoad) {
			if (!is_task_small(thread))
				return true;
		} else if (coreEntry->fLoad > kVeryHighLoad)
			disable_small_task_packing();
	}

	// Try small task packing.
	if (is_small_task_packing_enabled() && is_task_small(thread))
		return sSmallTaskCore != core;

	// No cpu bound threads - the situation is quite good. Make sure it
	// won't get much worse...
	SpinLocker coreLocker(sCoreHeapsLock);

	CoreEntry* other = sCoreLoadHeap->PeekMinimum();
	if (other == NULL)
		other = sCoreHighLoadHeap->PeekMinimum();
	return coreEntry->fLoad - other->fLoad >= kLoadDifference;
}


static bool
should_rebalance(Thread* thread)
{
	ASSERT(!sSingleCore);

	if (thread_is_idle_thread(thread))
		return false;

	return sShouldRebalance(thread);
}


static void
rebalance_irqs_low_latency(void)
{
	cpu_ent* cpu = get_cpu_struct();
	SpinLocker locker(cpu->irqs_lock);

	irq_assignment* chosen = NULL;
	irq_assignment* irq = (irq_assignment*)list_get_first_item(&cpu->irqs);

	int32 totalLoad = 0;
	while (irq != NULL) {
		if (chosen == NULL || chosen->load < irq->load)
			chosen = irq;
		totalLoad += irq->load;
		irq = (irq_assignment*)list_get_next_item(&cpu->irqs, irq);
	}

	locker.Unlock();

	if (chosen == NULL || totalLoad < kLowLoad)
		return;

	SpinLocker coreLocker(sCoreHeapsLock);
	CoreEntry* other = sCoreLoadHeap->PeekMinimum();
	if (other == NULL)
		other = sCoreHighLoadHeap->PeekMinimum();
	coreLocker.Unlock();

	ASSERT(other != NULL);

	int32 thisCore = sCPUToCore[smp_get_current_cpu()];
	if (other->fCoreID == thisCore)
		return;

	if (other->fLoad + kLoadDifference >= sCoreEntries[thisCore].fLoad)
		return;

	int32 newCPU = choose_cpu(other->fCoreID);
	assign_io_interrupt_to_cpu(chosen->irq, newCPU);
}


static inline void
compute_cpu_load(int32 cpu)
{
	ASSERT(!sSingleCore);

	int oldLoad = compute_load(sCPUEntries[cpu].fMeasureTime,
		sCPUEntries[cpu].fMeasureActiveTime, sCPUEntries[cpu].fLoad);
	if (oldLoad < 0)
		return;

	if (sCPUEntries[cpu].fLoad > kVeryHighLoad)
		sRebalanceIRQs();

	if (oldLoad != sCPUEntries[cpu].fLoad) {
		int32 core = sCPUToCore[cpu];

		int32 delta = sCPUEntries[cpu].fLoad - oldLoad;
		atomic_add(&sCoreEntries[core].fLoad, delta);

		update_load_heaps(core);
	}
}


static inline void
compute_thread_load(Thread* thread)
{
	compute_load(thread->scheduler_data->measure_time,
		thread->scheduler_data->measure_active_time,
		thread->scheduler_data->load);
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
	schedulerThreadData->went_sleep_active
		= atomic_get64(&sCoreEntries[core].fActiveTime);
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

	if (wentSleep < atomic_get64(&sCoreEntries[core].fReachedIdle))
		return true;

	bigtime_t startedIdle = atomic_get64(&sCoreEntries[core].fStartedIdle);
	if (startedIdle != 0) {
		if (wentSleep < startedIdle && now - startedIdle >= kMinimalWaitTime)
			return true;

		if (wentSleep - startedIdle >= kMinimalWaitTime)
			return true;
	}

	if (get_effective_priority(thread) == B_LOWEST_ACTIVE_PRIORITY)
		return false;

	if (wentSleep < atomic_get64(&sCoreEntries[core].fReachedBottom))
		return true;

	bigtime_t startedBottom = atomic_get64(&sCoreEntries[core].fStartedBottom);
	if (sCoreEntries[core].fStartedBottom != 0) {
		if (wentSleep < startedBottom
			&& now - startedBottom >= kMinimalWaitTime) {
			return true;
		}

		if (wentSleep - startedBottom >= kMinimalWaitTime)
			return true;
	}

	return false;
}


static void
enqueue(Thread* thread, bool newOne)
{
	ASSERT(thread != NULL);

	thread->state = thread->next_state = B_THREAD_READY;

	compute_thread_load(thread);

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;
	schedulerThreadData->cpu_bound = true;
	schedulerThreadData->time_left = 0;
	int32 threadPriority = get_effective_priority(thread);

	T(EnqueueThread(thread, threadPriority));

	bool pinned = thread->pinned_to_cpu > 0;
	int32 targetCPU = -1;
	int32 targetCore = -1;
	if (pinned)
		targetCPU = thread->previous_cpu->cpu_num;
	else if (sSingleCore)
		targetCore = 0;
	else if (schedulerThreadData->previous_core >= 0
		&& (!newOne || !has_cache_expired(thread))
		&& !should_rebalance(thread)) {
		targetCore = schedulerThreadData->previous_core;
	}

	bool shouldReschedule = choose_core_and_cpu(thread, targetCore, targetCPU);
	schedulerThreadData->previous_core = targetCore;

	TRACE("enqueueing thread %ld with priority %ld on CPU %ld (core %ld)\n",
		thread->id, threadPriority, targetCPU, targetCore);

	SpinLocker runQueueLocker(sCoreEntries[targetCore].fLock);
	thread->scheduler_data->enqueued = true;
	if (pinned)
		sPinnedRunQueues[targetCPU].PushBack(thread, threadPriority);
	else
		sRunQueues[targetCore].PushBack(thread, threadPriority);
	runQueueLocker.Unlock();

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadEnqueuedInRunQueue,
		thread);

	if (shouldReschedule) {
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
	InterruptsReadSpinLocker modeLocker(sSchedulerModeLock);

	TRACE("enqueueing new thread %ld with static priority %ld\n", thread->id,
		thread->priority);

	scheduler_thread_data* schedulerThreadData = thread->scheduler_data;

	int32 core = schedulerThreadData->previous_core;
	if (core >= 0) {
		if (should_cancel_penalty(thread))
			cancel_penalty(thread);
	}

	enqueue(thread, true);
}


static inline void
put_back(Thread* thread)
{
	compute_thread_load(thread);

	int32 core = sCPUToCore[smp_get_current_cpu()];

	SpinLocker runQueueLocker(sCoreEntries[core].fLock);
	thread->scheduler_data->enqueued = true;
	if (thread->pinned_to_cpu > 0) {
		int32 pinnedCPU = thread->previous_cpu->cpu_num;

		ASSERT(pinnedCPU == smp_get_current_cpu());
		sPinnedRunQueues[pinnedCPU].PushFront(thread,
			get_effective_priority(thread));
	} else {
		int32 previousCore = thread->scheduler_data->previous_core;
		ASSERT(previousCore >= 0);

		ASSERT(previousCore == core);
		sRunQueues[previousCore].PushFront(thread,
			get_effective_priority(thread));
	}
}


/*!	Sets the priority of a thread.
*/
int32
scheduler_set_thread_priority(Thread *thread, int32 priority)
{
	InterruptsSpinLocker _(thread->scheduler_lock);
	InterruptsReadSpinLocker modeLocker(sSchedulerModeLock);

	int32 oldPriority = thread->priority;

	TRACE("changing thread %ld priority to %ld (old: %ld, effective: %ld)\n",
		thread->id, priority, oldPriority, get_effective_priority(thread));

	cancel_penalty(thread);

	if (priority == thread->priority)
		return thread->priority;

	thread->priority = priority;

	if (thread->state != B_THREAD_READY) {
		cancel_penalty(thread);
		thread->priority = priority;

		if (thread->state == B_THREAD_RUNNING) {
			SpinLocker coreLocker(sCoreHeapsLock);
			update_cpu_priority(thread->cpu->cpu_num, priority);
		}
		return oldPriority;
	}

	// The thread is in the run queue. We need to remove it and re-insert it at
	// a new position.

	bool pinned = thread->pinned_to_cpu > 0;
	int32 previousCPU = thread->previous_cpu->cpu_num;
	int32 previousCore = thread->scheduler_data->previous_core;
	ASSERT(previousCore >= 0);

	SpinLocker runQueueLocker(sCoreEntries[previousCore].fLock);

	// the thread might have been already dequeued and is about to start
	// running once we release its scheduler_lock, in such case we can not
	// attempt to dequeue it
	if (thread->scheduler_data->enqueued) {
		T(RemoveThread(thread));

		// notify listeners
		NotifySchedulerListeners(&SchedulerListener::ThreadRemovedFromRunQueue,
			thread);

		thread->scheduler_data->enqueued = false;
		if (pinned)
			sPinnedRunQueues[previousCPU].Remove(thread);
		else
			sRunQueues[previousCore].Remove(thread);
		runQueueLocker.Unlock();

		enqueue(thread, true);
	}

	return oldPriority;
}


static inline void
reschedule_needed()
{
	// This function is called as a result of either the timer event set by the
	// scheduler or an incoming ICI. Make sure the reschedule() is invoked.
	thread_get_current_thread()->scheduler_data->lost_cpu = true;
	get_cpu_struct()->invoke_scheduler = true;
}


void
scheduler_reschedule_ici()
{
	reschedule_needed();
}


static int32
reschedule_event(timer* /* unused */)
{
	reschedule_needed();
	get_cpu_struct()->preempted = true;
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
choose_next_thread(int32 thisCPU, Thread* oldThread, bool putAtBack)
{
	int32 thisCore = sCPUToCore[thisCPU];

	SpinLocker runQueueLocker(sCoreEntries[thisCore].fLock);

	Thread* sharedThread = sRunQueues[thisCore].PeekMaximum();
	Thread* pinnedThread = sPinnedRunQueues[thisCPU].PeekMaximum();

	ASSERT(sharedThread != NULL || pinnedThread != NULL || oldThread != NULL);

	int32 pinnedPriority = -1;
	if (pinnedThread != NULL)
		pinnedPriority = get_effective_priority(pinnedThread);

	int32 sharedPriority = -1;
	if (sharedThread != NULL)
		sharedPriority = get_effective_priority(sharedThread);

	int32 oldPriority = -1;
	if (oldThread != NULL)
		oldPriority = get_effective_priority(oldThread);

	int32 rest = max_c(pinnedPriority, sharedPriority);
	if (oldPriority > rest || (!putAtBack && oldPriority == rest)) {
		ASSERT(!oldThread->scheduler_data->enqueued);
		return oldThread;
	}

	if (sharedPriority > pinnedPriority) {
		ASSERT(sharedThread->scheduler_data->enqueued);
		sharedThread->scheduler_data->enqueued = false;

		sRunQueues[thisCore].Remove(sharedThread);
		return sharedThread;
	}

	ASSERT(pinnedThread->scheduler_data->enqueued);
	pinnedThread->scheduler_data->enqueued = false;

	sPinnedRunQueues[thisCPU].Remove(pinnedThread);
	return pinnedThread;
}


static inline void
track_cpu_activity(Thread* oldThread, Thread* nextThread, int32 thisCore)
{
	bigtime_t now = system_time();
	bigtime_t usedTime = now - oldThread->scheduler_data->quantum_start;

	if (thread_is_idle_thread(oldThread) && usedTime >= kMinimalWaitTime) {
		atomic_set64(&sCoreEntries[thisCore].fReachedBottom,
			now - kMinimalWaitTime);
		atomic_set64(&sCoreEntries[thisCore].fReachedIdle,
			now - kMinimalWaitTime);
	}

	if (get_effective_priority(oldThread) == B_LOWEST_ACTIVE_PRIORITY
		&& usedTime >= kMinimalWaitTime) {
		atomic_set64(&sCoreEntries[thisCore].fReachedBottom,
			now - kMinimalWaitTime);
	}

	if (!thread_is_idle_thread(oldThread)) {
		bigtime_t active
			= (oldThread->kernel_time - oldThread->cpu->last_kernel_time)
				+ (oldThread->user_time - oldThread->cpu->last_user_time);

		atomic_add64(&oldThread->cpu->active_time, active);
		oldThread->scheduler_data->measure_active_time += active;

		sCPUEntries[smp_get_current_cpu()].fMeasureActiveTime += active;
		atomic_add64(&sCoreEntries[thisCore].fActiveTime, active);
	}

	if (!sSingleCore)
		compute_cpu_load(smp_get_current_cpu());

	int32 oldPriority = get_effective_priority(oldThread);
	int32 nextPriority = get_effective_priority(nextThread);

	if (thread_is_idle_thread(nextThread)) {
		if (!thread_is_idle_thread(oldThread))
			atomic_set64(&sCoreEntries[thisCore].fStartedIdle, now);
		if (oldPriority > B_LOWEST_ACTIVE_PRIORITY)
			atomic_set64(&sCoreEntries[thisCore].fStartedBottom, now);
	} else if (nextPriority == B_LOWEST_ACTIVE_PRIORITY) {
		atomic_set64(&sCoreEntries[thisCore].fStartedIdle, 0);
		if (oldPriority > B_LOWEST_ACTIVE_PRIORITY)
			atomic_set64(&sCoreEntries[thisCore].fStartedBottom, now);
	} else {
		atomic_set64(&sCoreEntries[thisCore].fStartedBottom, 0);
		atomic_set64(&sCoreEntries[thisCore].fStartedIdle, 0);
	}

	if (!thread_is_idle_thread(nextThread)) {
		oldThread->cpu->last_kernel_time = nextThread->kernel_time;
		oldThread->cpu->last_user_time = nextThread->user_time;
	}
}


static inline void
update_cpu_performance(Thread* thread, int32 thisCore)
{
	int32 load = max_c(thread->scheduler_data->load,
			sCoreEntries[thisCore].fLoad);
	load = min_c(max_c(load, 0), kMaxLoad);

	if (load < kTargetLoad) {
		int32 delta = kTargetLoad - load;

		delta *= kTargetLoad;
		delta /= kCPUPerformanceScaleMax;

		decrease_cpu_performance(delta);
	} else {
		bool allowBoost = sSchedulerMode != SCHEDULER_MODE_POWER_SAVING;
		allowBoost = allowBoost || thread->scheduler_data->priority_penalty > 0;

		int32 delta = load - kTargetLoad;
		delta *= kMaxLoad - kTargetLoad;
		delta /= kCPUPerformanceScaleMax;

		increase_cpu_performance(delta, allowBoost);
	}
}


static void
_scheduler_reschedule(void)
{
	InterruptsReadSpinLocker modeLocker(sSchedulerModeLock);

	Thread* oldThread = thread_get_current_thread();

	int32 thisCPU = smp_get_current_cpu();
	int32 thisCore = sCPUToCore[thisCPU];

	TRACE("reschedule(): cpu %ld, current thread = %ld\n", thisCPU,
		oldThread->id);

	oldThread->state = oldThread->next_state;
	scheduler_thread_data* schedulerOldThreadData = oldThread->scheduler_data;

	bool enqueueOldThread = false;
	bool putOldThreadAtBack = false;
	switch (oldThread->next_state) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			enqueueOldThread = true;

			if (!schedulerOldThreadData->lost_cpu)
				schedulerOldThreadData->cpu_bound = false;

			if (quantum_ended(oldThread, oldThread->cpu->preempted,
					oldThread->has_yielded)) {
				if (schedulerOldThreadData->cpu_bound)
					increase_penalty(oldThread);

				TRACE("enqueueing thread %ld into run queue priority = %ld\n",
					oldThread->id, get_effective_priority(oldThread));
				putOldThreadAtBack = true;
			} else {
				TRACE("putting thread %ld back in run queue priority = %ld\n",
					oldThread->id, get_effective_priority(oldThread));
				putOldThreadAtBack = false;
			}

			break;
		case B_THREAD_SUSPENDED:
			increase_penalty(oldThread);
			thread_goes_away(oldThread);
			TRACE("reschedule(): suspending thread %ld\n", oldThread->id);
			break;
		case THREAD_STATE_FREE_ON_RESCHED:
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

	// select thread with the biggest priority and enqueue back the old thread
	Thread* nextThread
		= choose_next_thread(thisCPU, enqueueOldThread ? oldThread : NULL,
			putOldThreadAtBack);
	if (nextThread != oldThread) {
		if (enqueueOldThread) {
			if (putOldThreadAtBack)
				enqueue(oldThread, false);
			else
				put_back(oldThread);
		}

		acquire_spinlock(&nextThread->scheduler_lock);
	}

	TRACE("reschedule(): cpu %ld, next thread = %ld\n", thisCPU,
		nextThread->id);

	T(ScheduleThread(nextThread, oldThread));

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadScheduled,
		oldThread, nextThread);

	// update CPU heap
	{
		SpinLocker coreLocker(sCoreHeapsLock);
		update_cpu_priority(thisCPU, get_effective_priority(nextThread));
	}

	nextThread->state = B_THREAD_RUNNING;
	nextThread->next_state = B_THREAD_READY;

	ASSERT(nextThread->scheduler_data->previous_core == thisCore);

	compute_thread_load(nextThread);

	// track kernel time (user time is tracked in thread_at_kernel_entry())
	scheduler_update_thread_times(oldThread, nextThread);

	// track CPU activity
	track_cpu_activity(oldThread, nextThread, thisCore);

	if (nextThread != oldThread || oldThread->cpu->preempted) {
		timer* quantumTimer = &oldThread->cpu->quantum_timer;
		if (!oldThread->cpu->preempted)
			cancel_timer(quantumTimer);

		oldThread->cpu->preempted = false;
		if (!thread_is_idle_thread(nextThread)) {
			bigtime_t quantum = compute_quantum(oldThread);
			add_timer(quantumTimer, &reschedule_event, quantum,
				B_ONE_SHOT_RELATIVE_TIMER);

			update_cpu_performance(nextThread, thisCore);
		} else
			nextThread->scheduler_data->quantum_start = system_time();

		modeLocker.Unlock();
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

	if (thread_is_idle_thread(thread)) {
		static int32 sIdleThreadsID;
		int32 cpu = atomic_add(&sIdleThreadsID, 1);

		thread->previous_cpu = &gCPU[cpu];
		thread->pinned_to_cpu = 1;

		thread->scheduler_data->previous_core = sCPUToCore[cpu];
	}
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
	InterruptsSpinLocker _(thread_get_current_thread()->scheduler_lock);

	_scheduler_reschedule();
}


status_t
scheduler_set_operation_mode(scheduler_mode mode)
{
	if (mode != SCHEDULER_MODE_LOW_LATENCY
		&& mode != SCHEDULER_MODE_POWER_SAVING) {
		return B_BAD_VALUE;
	}

	const char* modeNames[] = { "low latency", "power saving" };
	dprintf("scheduler: switching to %s mode\n", modeNames[mode]);

	InterruptsWriteSpinLocker _(sSchedulerModeLock);

	sSchedulerMode = mode;
	switch (mode) {
		case SCHEDULER_MODE_LOW_LATENCY:
			sDisableSmallTaskPacking = -1;
			sSmallTaskCore = -1;

			sChooseCore = choose_core_low_latency;
			sShouldRebalance = should_rebalance_low_latency;
			sRebalanceIRQs = rebalance_irqs_low_latency;
			break;

		case SCHEDULER_MODE_POWER_SAVING:
			sDisableSmallTaskPacking = 0;
			sSmallTaskCore = -1;

			sChooseCore = choose_core_power_saving;
			sShouldRebalance = should_rebalance_power_saving;
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

	sCoreLoadHeap = new CoreLoadHeap;
	if (sCoreLoadHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<CoreLoadHeap> coreLoadHeapDeleter(sCoreLoadHeap);

	sCoreHighLoadHeap = new CoreLoadHeap(coreCount);
	if (sCoreHighLoadHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<CoreLoadHeap> coreHighLoadHeapDeleter(sCoreHighLoadHeap);

	for (int32 i = 0; i < coreCount; i++) {
		sCoreEntries[i].fCoreID = i;

		status_t result = sCoreLoadHeap->Insert(&sCoreEntries[i], 0);
		if (result != B_OK)
			return result;
	}

	sCPUPriorityHeaps = new CPUHeap[coreCount];
	if (sCPUPriorityHeaps == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<CPUHeap> cpuPriorityHeapDeleter(sCPUPriorityHeaps);

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

#if 1
	scheduler_set_operation_mode(SCHEDULER_MODE_LOW_LATENCY);
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
	coreHighLoadHeapDeleter.Detach();
	coreLoadHeapDeleter.Detach();
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
	InterruptsSpinLocker _(gSchedulerListenersLock);
	gSchedulerListeners.Add(listener);
}


/*!	Remove the given scheduler listener. Thread lock must be held.
*/
void
scheduler_remove_listener(struct SchedulerListener* listener)
{
	InterruptsSpinLocker _(gSchedulerListenersLock);
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
