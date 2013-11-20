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


#define CACHE_LINE_ALIGN	 __attribute__((aligned(64)))


//#define TRACE_SCHEDULER
#ifdef TRACE_SCHEDULER
#	define TRACE(...) dprintf_no_syslog(__VA_ARGS__)
#else
#	define TRACE(...) do { } while (false)
#endif


namespace Scheduler {


const bigtime_t kThreadQuantum = 1000;
const bigtime_t kMinThreadQuantum = 3000;
const bigtime_t kMaxThreadQuantum = 10000;

const bigtime_t kMinimalWaitTime = kThreadQuantum / 4;

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

	spinlock	fLock;

	bigtime_t	fStartedBottom;
	bigtime_t	fReachedBottom;
	bigtime_t	fStartedIdle;
	bigtime_t	fReachedIdle;

	bigtime_t	fActiveTime;

	int32		fLoad;
} CACHE_LINE_ALIGN;
typedef MinMaxHeap<CoreEntry, int32> CoreLoadHeap;

extern CoreEntry* gCoreEntries;
extern CoreLoadHeap* gCoreLoadHeap;
extern CoreLoadHeap* gCoreHighLoadHeap;
extern spinlock gCoreHeapsLock;

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

extern PackageEntry* gPackageEntries;
extern PackageHeap* gPackageUsageHeap;
extern IdlePackageList* gIdlePackageList;
extern spinlock gIdlePackageLock;

// The run queues. Holds the threads ready to run ordered by priority.
// One queue per schedulable target per core. Additionally, each
// logical processor has its sPinnedRunQueues used for scheduling
// pinned threads.
typedef RunQueue<Thread, THREAD_MAX_SET_PRIORITY> CACHE_LINE_ALIGN
	ThreadRunQueue;

extern ThreadRunQueue* gRunQueues;
extern ThreadRunQueue* gPinnedRunQueues;
extern int32 gRunQueueCount;

// Since CPU IDs used internally by the kernel bear no relation to the actual
// CPU topology the following arrays are used to efficiently get the core
// and the package that CPU in question belongs to.
extern int32* gCPUToCore;
extern int32* gCPUToPackage;


}	// namespace Scheduler


struct scheduler_thread_data {
	inline				scheduler_thread_data();
			void		Init();

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


/*!	Switches the currently running thread.
	This is a service function for scheduler implementations.

	\param fromThread The currently running thread.
	\param toThread The thread to switch to. Must be different from
		\a fromThread.
*/
static inline void
scheduler_switch_thread(Thread* fromThread, Thread* toThread)
{
	// notify the user debugger code
	if ((fromThread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_unscheduled(fromThread);

	// stop CPU time based user timers
	acquire_spinlock(&fromThread->team->time_lock);
	acquire_spinlock(&fromThread->time_lock);
	if (fromThread->HasActiveCPUTimeUserTimers()
		|| fromThread->team->HasActiveCPUTimeUserTimers()) {
		user_timer_stop_cpu_timers(fromThread, toThread);
	}
	release_spinlock(&fromThread->time_lock);
	release_spinlock(&fromThread->team->time_lock);

	// update CPU and Thread structures and perform the context switch
	cpu_ent* cpu = fromThread->cpu;
	toThread->previous_cpu = toThread->cpu = cpu;
	fromThread->cpu = NULL;
	cpu->running_thread = toThread;
	cpu->previous_thread = fromThread;

	arch_thread_set_current_thread(toThread);
	arch_thread_context_switch(fromThread, toThread);

	release_spinlock(&fromThread->cpu->previous_thread->scheduler_lock);

	// The use of fromThread below looks weird, but is correct. fromThread had
	// been unscheduled earlier, but is back now. For a thread scheduled the
	// first time the same is done in thread.cpp:common_thread_entry().

	// continue CPU time based user timers
	acquire_spinlock(&fromThread->team->time_lock);
	acquire_spinlock(&fromThread->time_lock);
	if (fromThread->HasActiveCPUTimeUserTimers()
		|| fromThread->team->HasActiveCPUTimeUserTimers()) {
		user_timer_continue_cpu_timers(fromThread, cpu->previous_thread);
	}
	release_spinlock(&fromThread->time_lock);
	release_spinlock(&fromThread->team->time_lock);

	// notify the user debugger code
	if ((fromThread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_scheduled(fromThread);
}


static inline void
scheduler_update_thread_times(Thread* oldThread, Thread* nextThread)
{
	bigtime_t now = system_time();
	if (oldThread == nextThread) {
		acquire_spinlock(&oldThread->time_lock);
		oldThread->kernel_time += now - oldThread->last_time;
		oldThread->last_time = now;
		release_spinlock(&oldThread->time_lock);
	} else {
		acquire_spinlock(&oldThread->time_lock);
		oldThread->kernel_time += now - oldThread->last_time;
		oldThread->last_time = 0;
		release_spinlock(&oldThread->time_lock);

		acquire_spinlock(&nextThread->time_lock);
		nextThread->last_time = now;
		release_spinlock(&nextThread->time_lock);
	}

	// If the old thread's team has user time timers, check them now.
	Team* team = oldThread->team;

	acquire_spinlock(&team->time_lock);
	if (team->HasActiveUserTimeUserTimers())
		user_timer_check_team_user_timers(team);
	release_spinlock(&team->time_lock);
}


#endif	// KERNEL_SCHEDULER_COMMON_H
