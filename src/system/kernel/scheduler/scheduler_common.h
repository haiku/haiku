/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_COMMON_H
#define KERNEL_SCHEDULER_COMMON_H


#include <debug.h>
#include <kscheduler.h>
#include <load_tracking.h>
#include <smp.h>
#include <thread.h>
#include <user_debugger.h>
#include <util/MinMaxHeap.h>

#include "RunQueue.h"


//#define TRACE_SCHEDULER
#ifdef TRACE_SCHEDULER
#	define TRACE(...) dprintf_no_syslog(__VA_ARGS__)
#else
#	define TRACE(...) do { } while (false)
#endif


namespace Scheduler {


const bigtime_t kThreadQuantum = 1000;

const bigtime_t kCacheExpire = 100000;

const int kLowLoad = kMaxLoad * 20 / 100;
const int kTargetLoad = kMaxLoad * 55 / 100;
const int kHighLoad = kMaxLoad * 70 / 100;
const int kVeryHighLoad = (kMaxLoad + kHighLoad) / 2;

const int kLoadDifference = kMaxLoad * 20 / 100;

extern bool gSingleCore;

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

	rw_spinlock fSchedulerModeLock;
} CACHE_LINE_ALIGN;
typedef MinMaxHeap<CPUEntry, int32> CPUHeap CACHE_LINE_ALIGN;

extern CPUEntry* gCPUEntries;
extern CPUHeap* gCPUPriorityHeaps;

struct CoreEntry : public MinMaxHeapLinkImpl<CoreEntry, int32>,
	DoublyLinkedListLinkImpl<CoreEntry> {
				CoreEntry();

	int32		fCoreID;

	int32		fCPUCount;

	spinlock	fCPULock;
	spinlock	fQueueLock;

	int32		fThreadCount;
	DoublyLinkedList<scheduler_thread_data>	fThreadList;

	bigtime_t	fActiveTime;

	int32		fLoad;
} CACHE_LINE_ALIGN;
typedef MinMaxHeap<CoreEntry, int32> CoreLoadHeap;

extern CoreEntry* gCoreEntries;
extern CoreLoadHeap* gCoreLoadHeap;
extern CoreLoadHeap* gCoreHighLoadHeap;
extern rw_spinlock gCoreHeapsLock;
extern int32 gCoreCount;

// gPackageEntries are used to decide which core should be woken up from the
// idle state. When aiming for performance we should use as many packages as
// possible with as little cores active in each package as possible (so that the
// package can enter any boost mode if it has one and the active core have more
// of the shared cache for themselves. If power saving is the main priority we
// should keep active cores on as little packages as possible (so that other
// packages can go to the deep state of sleep). The heap stores only packages
// with at least one core active and one core idle. The packages with all cores
// idle are stored in sPackageIdleList (in LIFO manner).
struct PackageEntry : public DoublyLinkedListLinkImpl<PackageEntry> {
								PackageEntry();

	int32						fPackageID;

	spinlock					fCoreLock;

	DoublyLinkedList<CoreEntry>	fIdleCores;
	int32						fIdleCoreCount;

	int32						fCoreCount;
} CACHE_LINE_ALIGN;
typedef DoublyLinkedList<PackageEntry> IdlePackageList;

extern PackageEntry* gPackageEntries;
extern IdlePackageList* gIdlePackageList;
extern spinlock gIdlePackageLock;
extern int32 gPackageCount;

// The run queues. Holds the threads ready to run ordered by priority.
// One queue per schedulable target per core. Additionally, each
// logical processor has its sPinnedRunQueues used for scheduling
// pinned threads.
typedef RunQueue<Thread, THREAD_MAX_SET_PRIORITY> CACHE_LINE_ALIGN
	ThreadRunQueue;

extern ThreadRunQueue* gRunQueues;
extern ThreadRunQueue* gPinnedRunQueues;

// Since CPU IDs used internally by the kernel bear no relation to the actual
// CPU topology the following arrays are used to efficiently get the core
// and the package that CPU in question belongs to.
extern int32* gCPUToCore;
extern int32* gCPUToPackage;


}	// namespace Scheduler


struct scheduler_thread_data :
	public DoublyLinkedListLinkImpl<scheduler_thread_data> {
	inline				scheduler_thread_data();
			void		Init();

			int32		priority_penalty;
			int32		additional_penalty;

			bigtime_t	time_left;
			bigtime_t	stolen_time;
			bigtime_t	quantum_start;
			bigtime_t	last_interrupt_time;

			bigtime_t	measure_active_time;
			bigtime_t	measure_time;
			int32		load;

			bigtime_t	went_sleep;
			bigtime_t	went_sleep_active;
			int32		went_sleep_count;

			int32		previous_core;

			bool		enqueued;
};


static inline int32
get_core_load(struct Scheduler::CoreEntry* core)
{
	return core->fLoad / core->fCPUCount;
}


static inline int32
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


#endif	// KERNEL_SCHEDULER_COMMON_H
