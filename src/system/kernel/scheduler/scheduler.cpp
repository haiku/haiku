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
#include <timer.h>

#include <cpufreq.h>

#include "scheduler_common.h"
#include "scheduler_modes.h"
#include "scheduler_tracing.h"


using namespace Scheduler;


static bool sSchedulerEnabled;

SchedulerListenerList gSchedulerListeners;
spinlock gSchedulerListenersLock = B_SPINLOCK_INITIALIZER;

static rw_spinlock sSchedulerModeLock = B_RW_SPINLOCK_INITIALIZER;
static struct scheduler_mode_operations* sCurrentMode;
static struct scheduler_mode_operations* sSchedulerModes[] = {
	&gSchedulerLowLatencyMode,
	&gSchedulerPowerSavingMode,
};

namespace Scheduler {

bool gSingleCore;

CPUEntry* gCPUEntries;
CPUHeap* gCPUPriorityHeaps;

CoreEntry* gCoreEntries;
CoreLoadHeap* gCoreLoadHeap;
CoreLoadHeap* gCoreHighLoadHeap;
spinlock gCoreHeapsLock = B_SPINLOCK_INITIALIZER;

PackageEntry* gPackageEntries;
PackageHeap* gPackageUsageHeap;
IdlePackageList* gIdlePackageList;
spinlock gIdlePackageLock = B_SPINLOCK_INITIALIZER;

ThreadRunQueue* gRunQueues;
ThreadRunQueue* gPinnedRunQueues;
int32 gRunQueueCount;

int32* gCPUToCore;
int32* gCPUToPackage;

}	// namespace Scheduler


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


scheduler_thread_data::scheduler_thread_data()
{
	Init();
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
			gCPUToCore[i] = coreCount++;
	}

	ThreadRunQueue::ConstIterator iterator;
	for (int32 i = 0; i < coreCount; i++) {
		kprintf("\nCore %" B_PRId32 " run queue:\n", i);
		iterator = gRunQueues[i].GetConstIterator();
		dump_queue(iterator);
	}

	for (int32 i = 0; i < cpuCount; i++) {
		iterator = gPinnedRunQueues[i].GetConstIterator();

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
			gCPUEntries[cpu].fLoad / 10);

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
	CoreLoadHeap temp(gRunQueueCount);
	int32 cpuPerCore = smp_get_num_cpus() / gRunQueueCount;

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
	dump_core_load_heap(gCoreLoadHeap);
	kprintf("---------\n");
	dump_core_load_heap(gCoreHighLoadHeap);

	for (int32 i = 0; i < gRunQueueCount; i++) {
		kprintf("\nCore %" B_PRId32 " heap:\n", i);
		dump_heap(&gCPUPriorityHeaps[i]);
	}

	return 0;
}


static int
dump_idle_cores(int argc, char** argv)
{
	kprintf("Idle packages:\n");
	IdlePackageList::ReverseIterator idleIterator
		= gIdlePackageList->GetReverseIterator();

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

	PackageEntry* entry = gPackageUsageHeap->PeekMinimum();
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

		gPackageUsageHeap->RemoveMinimum();
		temp.Insert(entry, entry->fIdleCoreCount);

		entry = gPackageUsageHeap->PeekMinimum();
	}

	entry = temp.PeekMinimum();
	while (entry != NULL) {
		int32 key = PackageHeap::GetKey(entry);
		temp.RemoveMinimum();
		gPackageUsageHeap->Insert(entry, key);
		entry = temp.PeekMinimum();
	}

	return 0;
}


static inline bool
has_cache_expired(Thread* thread)
{
	return sCurrentMode->has_cache_expired(thread);
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
	ASSERT(!gSingleCore);

	CoreEntry* entry = &gCoreEntries[core];

	SpinLocker coreLocker(gCoreHeapsLock);

	int32 cpuPerCore = smp_get_num_cpus() / gRunQueueCount;
	int32 newKey = entry->fLoad / cpuPerCore;
	int32 oldKey = CoreLoadHeap::GetKey(entry);

	ASSERT(oldKey >= 0 && oldKey <= kMaxLoad);
	ASSERT(newKey >= 0 && newKey <= kMaxLoad);

	if (oldKey == newKey)
		return;

	if (newKey > kHighLoad) {
		if (oldKey <= kHighLoad) {
			gCoreLoadHeap->ModifyKey(entry, -1);
			ASSERT(gCoreLoadHeap->PeekMinimum() == entry);
			gCoreLoadHeap->RemoveMinimum();

			gCoreHighLoadHeap->Insert(entry, newKey);
		} else
			gCoreHighLoadHeap->ModifyKey(entry, newKey);
	} else {
		if (oldKey > kHighLoad) {
			gCoreHighLoadHeap->ModifyKey(entry, -1);
			ASSERT(gCoreHighLoadHeap->PeekMinimum() == entry);
			gCoreHighLoadHeap->RemoveMinimum();

			gCoreLoadHeap->Insert(entry, newKey);
		} else
			gCoreLoadHeap->ModifyKey(entry, newKey);
	}
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
	int32 core = gCPUToCore[cpu];

	int32 corePriority = CPUHeap::GetKey(gCPUPriorityHeaps[core].PeekMaximum());

	gCPUEntries[cpu].fPriority = priority;
	gCPUPriorityHeaps[core].ModifyKey(&gCPUEntries[cpu], priority);

	if (gSingleCore)
		return;

	int32 maxPriority
		= CPUHeap::GetKey(gCPUPriorityHeaps[core].PeekMaximum());

	if (corePriority == maxPriority)
		return;

	int32 package = gCPUToPackage[cpu];
	PackageEntry* packageEntry = &gPackageEntries[package];
	if (maxPriority == B_IDLE_PRIORITY) {
		SpinLocker _(gIdlePackageLock);

		// core goes idle
		ASSERT(packageEntry->fIdleCoreCount >= 0);
		ASSERT(packageEntry->fIdleCoreCount < packageEntry->fCoreCount);

		packageEntry->fIdleCoreCount++;
		packageEntry->fIdleCores.Add(&gCoreEntries[core]);

		if (packageEntry->fIdleCoreCount == 1) {
			// first core on that package to go idle

			if (packageEntry->fCoreCount > 1)
				gPackageUsageHeap->Insert(packageEntry, 1);
			else
				gIdlePackageList->Add(packageEntry);
		} else if (packageEntry->fIdleCoreCount
			== packageEntry->fCoreCount) {
			// package goes idle
			gPackageUsageHeap->ModifyKey(packageEntry, 0);
			ASSERT(gPackageUsageHeap->PeekMinimum() == packageEntry);
			gPackageUsageHeap->RemoveMinimum();

			gIdlePackageList->Add(packageEntry);
		} else {
			gPackageUsageHeap->ModifyKey(packageEntry,
				packageEntry->fIdleCoreCount);
		}
	} else if (corePriority == B_IDLE_PRIORITY) {
		SpinLocker _(gIdlePackageLock);

		// core wakes up
		ASSERT(packageEntry->fIdleCoreCount > 0);
		ASSERT(packageEntry->fIdleCoreCount <= packageEntry->fCoreCount);

		packageEntry->fIdleCoreCount--;
		packageEntry->fIdleCores.Remove(&gCoreEntries[core]);

		if (packageEntry->fIdleCoreCount + 1 == packageEntry->fCoreCount) {
			// package wakes up
			gIdlePackageList->Remove(packageEntry);

			if (packageEntry->fIdleCoreCount > 0) {
				gPackageUsageHeap->Insert(packageEntry,
					packageEntry->fIdleCoreCount);
			}
		} else if (packageEntry->fIdleCoreCount == 0) {
			// no more idle cores in the package
			gPackageUsageHeap->ModifyKey(packageEntry, 0);
			ASSERT(gPackageUsageHeap->PeekMinimum() == packageEntry);
			gPackageUsageHeap->RemoveMinimum();
		} else {
			gPackageUsageHeap->ModifyKey(packageEntry,
				packageEntry->fIdleCoreCount);
		}
	}
}


static inline int32
choose_core(Thread* thread)
{
	ASSERT(!gSingleCore);
	return sCurrentMode->choose_core(thread);
}


static inline int32
choose_cpu(int32 core)
{
	CPUEntry* entry = gCPUPriorityHeaps[core].PeekMinimum();
	ASSERT(entry != NULL);
	return entry->fCPUNumber;
}


static bool
choose_core_and_cpu(Thread* thread, int32& targetCore, int32& targetCPU)
{
	SpinLocker coreLocker(gCoreHeapsLock);

	if (targetCore == -1 && targetCPU != -1)
		targetCore = gCPUToCore[targetCPU];
	else if (targetCore != -1 && targetCPU == -1)
		targetCPU = choose_cpu(targetCore);
	else if (targetCore == -1 && targetCPU == -1) {
		targetCore = choose_core(thread);
		targetCPU = choose_cpu(targetCore);
	}

	ASSERT(targetCore >= 0 && targetCore < gRunQueueCount);
	ASSERT(targetCPU >= 0 && targetCPU < smp_get_num_cpus());

	int32 targetPriority = gCPUEntries[targetCPU].fPriority;
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
should_rebalance(Thread* thread)
{
	ASSERT(!gSingleCore);

	if (thread_is_idle_thread(thread))
		return false;

	return sCurrentMode->should_rebalance(thread);
}


static inline void
compute_cpu_load(int32 cpu)
{
	ASSERT(!gSingleCore);

	int oldLoad = compute_load(gCPUEntries[cpu].fMeasureTime,
		gCPUEntries[cpu].fMeasureActiveTime, gCPUEntries[cpu].fLoad);
	if (oldLoad < 0)
		return;

	if (gCPUEntries[cpu].fLoad > kVeryHighLoad)
		sCurrentMode->rebalance_irqs(false);

	if (oldLoad != gCPUEntries[cpu].fLoad) {
		int32 core = gCPUToCore[cpu];

		int32 delta = gCPUEntries[cpu].fLoad - oldLoad;
		atomic_add(&gCoreEntries[core].fLoad, delta);

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
		= atomic_get64(&gCoreEntries[core].fActiveTime);
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

	if (wentSleep < atomic_get64(&gCoreEntries[core].fReachedIdle))
		return true;

	bigtime_t startedIdle = atomic_get64(&gCoreEntries[core].fStartedIdle);
	if (startedIdle != 0) {
		if (wentSleep < startedIdle && now - startedIdle >= kMinimalWaitTime)
			return true;

		if (wentSleep - startedIdle >= kMinimalWaitTime)
			return true;
	}

	if (get_effective_priority(thread) == B_LOWEST_ACTIVE_PRIORITY)
		return false;

	if (wentSleep < atomic_get64(&gCoreEntries[core].fReachedBottom))
		return true;

	bigtime_t startedBottom = atomic_get64(&gCoreEntries[core].fStartedBottom);
	if (gCoreEntries[core].fStartedBottom != 0) {
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
	else if (gSingleCore)
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

	SpinLocker runQueueLocker(gCoreEntries[targetCore].fLock);
	thread->scheduler_data->enqueued = true;
	if (pinned)
		gPinnedRunQueues[targetCPU].PushBack(thread, threadPriority);
	else
		gRunQueues[targetCore].PushBack(thread, threadPriority);
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

	int32 core = gCPUToCore[smp_get_current_cpu()];

	SpinLocker runQueueLocker(gCoreEntries[core].fLock);
	thread->scheduler_data->enqueued = true;
	if (thread->pinned_to_cpu > 0) {
		int32 pinnedCPU = thread->previous_cpu->cpu_num;

		ASSERT(pinnedCPU == smp_get_current_cpu());
		gPinnedRunQueues[pinnedCPU].PushFront(thread,
			get_effective_priority(thread));
	} else {
		int32 previougCore = thread->scheduler_data->previous_core;
		ASSERT(previougCore >= 0);

		ASSERT(previougCore == core);
		gRunQueues[previougCore].PushFront(thread,
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
			SpinLocker coreLocker(gCoreHeapsLock);
			update_cpu_priority(thread->cpu->cpu_num, priority);
		}
		return oldPriority;
	}

	// The thread is in the run queue. We need to remove it and re-insert it at
	// a new position.

	bool pinned = thread->pinned_to_cpu > 0;
	int32 previougCPU = thread->previous_cpu->cpu_num;
	int32 previougCore = thread->scheduler_data->previous_core;
	ASSERT(previougCore >= 0);

	SpinLocker runQueueLocker(gCoreEntries[previougCore].fLock);

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
			gPinnedRunQueues[previougCPU].Remove(thread);
		else
			gRunQueues[previougCore].Remove(thread);
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
	int32 thisCore = gCPUToCore[thisCPU];

	SpinLocker runQueueLocker(gCoreEntries[thisCore].fLock);

	Thread* sharedThread = gRunQueues[thisCore].PeekMaximum();
	Thread* pinnedThread = gPinnedRunQueues[thisCPU].PeekMaximum();

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

		gRunQueues[thisCore].Remove(sharedThread);
		return sharedThread;
	}

	ASSERT(pinnedThread->scheduler_data->enqueued);
	pinnedThread->scheduler_data->enqueued = false;

	gPinnedRunQueues[thisCPU].Remove(pinnedThread);
	return pinnedThread;
}


static inline void
track_cpu_activity(Thread* oldThread, Thread* nextThread, int32 thisCore)
{
	bigtime_t now = system_time();
	bigtime_t usedTime = now - oldThread->scheduler_data->quantum_start;

	if (thread_is_idle_thread(oldThread) && usedTime >= kMinimalWaitTime) {
		atomic_set64(&gCoreEntries[thisCore].fReachedBottom,
			now - kMinimalWaitTime);
		atomic_set64(&gCoreEntries[thisCore].fReachedIdle,
			now - kMinimalWaitTime);
	}

	if (get_effective_priority(oldThread) == B_LOWEST_ACTIVE_PRIORITY
		&& usedTime >= kMinimalWaitTime) {
		atomic_set64(&gCoreEntries[thisCore].fReachedBottom,
			now - kMinimalWaitTime);
	}

	if (!thread_is_idle_thread(oldThread)) {
		bigtime_t active
			= (oldThread->kernel_time - oldThread->cpu->last_kernel_time)
				+ (oldThread->user_time - oldThread->cpu->last_user_time);

		atomic_add64(&oldThread->cpu->active_time, active);
		oldThread->scheduler_data->measure_active_time += active;

		gCPUEntries[smp_get_current_cpu()].fMeasureActiveTime += active;
		atomic_add64(&gCoreEntries[thisCore].fActiveTime, active);
	}

	if (!gSingleCore)
		compute_cpu_load(smp_get_current_cpu());

	int32 oldPriority = get_effective_priority(oldThread);
	int32 nextPriority = get_effective_priority(nextThread);

	if (thread_is_idle_thread(nextThread)) {
		if (!thread_is_idle_thread(oldThread))
			atomic_set64(&gCoreEntries[thisCore].fStartedIdle, now);
		if (oldPriority > B_LOWEST_ACTIVE_PRIORITY)
			atomic_set64(&gCoreEntries[thisCore].fStartedBottom, now);
	} else if (nextPriority == B_LOWEST_ACTIVE_PRIORITY) {
		atomic_set64(&gCoreEntries[thisCore].fStartedIdle, 0);
		if (oldPriority > B_LOWEST_ACTIVE_PRIORITY)
			atomic_set64(&gCoreEntries[thisCore].fStartedBottom, now);
	} else {
		atomic_set64(&gCoreEntries[thisCore].fStartedBottom, 0);
		atomic_set64(&gCoreEntries[thisCore].fStartedIdle, 0);
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
			gCoreEntries[thisCore].fLoad);
	load = min_c(max_c(load, 0), kMaxLoad);

	if (load < kTargetLoad) {
		int32 delta = kTargetLoad - load;

		delta *= kTargetLoad;
		delta /= kCPUPerformanceScaleMax;

		decrease_cpu_performance(delta);
	} else {
		bool allowBoost = !sCurrentMode->avoid_boost;
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
	int32 thisCore = gCPUToCore[thisCPU];

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
		SpinLocker coreLocker(gCoreHeapsLock);
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
		} else {
			nextThread->scheduler_data->quantum_start = system_time();

			sCurrentMode->rebalance_irqs(true);
		}

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
		static int32 gIdleThreadsID;
		int32 cpu = atomic_add(&gIdleThreadsID, 1);

		thread->previous_cpu = &gCPU[cpu];
		thread->pinned_to_cpu = 1;

		thread->scheduler_data->previous_core = gCPUToCore[cpu];
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

	dprintf("scheduler: switching to %s mode\n", sSchedulerModes[mode]->name);

	InterruptsWriteSpinLocker _(sSchedulerModeLock);
	sCurrentMode = sSchedulerModes[mode];
	sCurrentMode->switch_to_mode();

	return B_OK;
}


static void
traverse_topology_tree(cpu_topology_node* node, int packageID, int coreID)
{
	switch (node->level) {
		case CPU_TOPOLOGY_SMT:
			gCPUToCore[node->id] = coreID;
			gCPUToPackage[node->id] = packageID;
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

	gCPUToCore = new(std::nothrow) int32[cpuCount];
	if (gCPUToCore == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<int32> cpuToCoreDeleter(gCPUToCore);

	gCPUToPackage = new(std::nothrow) int32[cpuCount];
	if (gCPUToPackage == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<int32> cpuToPackageDeleter(gCPUToPackage);

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
	gRunQueueCount = coreCount;
	gSingleCore = coreCount == 1;

	// create package heap and idle package stack
	gPackageEntries = new(std::nothrow) PackageEntry[packageCount];
	if (gPackageEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<PackageEntry> packageEntriesDeleter(gPackageEntries);

	gPackageUsageHeap = new(std::nothrow) PackageHeap(packageCount);
	if (gPackageUsageHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<PackageHeap> packageHeapDeleter(gPackageUsageHeap);

	gIdlePackageList = new(std::nothrow) IdlePackageList;
	if (gIdlePackageList == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<IdlePackageList> packageListDeleter(gIdlePackageList);

	for (int32 i = 0; i < packageCount; i++) {
		gPackageEntries[i].fPackageID = i;
		gPackageEntries[i].fIdleCoreCount = coreCount / packageCount;
		gPackageEntries[i].fCoreCount = coreCount / packageCount;
		gIdlePackageList->Insert(&gPackageEntries[i]);
	}

	// create logical processor and core heaps
	gCPUEntries = new CPUEntry[cpuCount];
	if (gCPUEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<CPUEntry> cpuEntriesDeleter(gCPUEntries);

	gCoreEntries = new CoreEntry[coreCount];
	if (gCoreEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<CoreEntry> coreEntriesDeleter(gCoreEntries);

	gCoreLoadHeap = new CoreLoadHeap;
	if (gCoreLoadHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<CoreLoadHeap> coreLoadHeapDeleter(gCoreLoadHeap);

	gCoreHighLoadHeap = new CoreLoadHeap(coreCount);
	if (gCoreHighLoadHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<CoreLoadHeap> coreHighLoadHeapDeleter(gCoreHighLoadHeap);

	for (int32 i = 0; i < coreCount; i++) {
		gCoreEntries[i].fCoreID = i;

		status_t result = gCoreLoadHeap->Insert(&gCoreEntries[i], 0);
		if (result != B_OK)
			return result;
	}

	gCPUPriorityHeaps = new CPUHeap[coreCount];
	if (gCPUPriorityHeaps == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<CPUHeap> cpuPriorityHeapDeleter(gCPUPriorityHeaps);

	for (int32 i = 0; i < cpuCount; i++) {
		gCPUEntries[i].fCPUNumber = i;

		int32 core = gCPUToCore[i];

		int32 package = gCPUToPackage[i];
		if (gCPUPriorityHeaps[core].PeekMaximum() == NULL)
			gPackageEntries[package].fIdleCores.Insert(&gCoreEntries[core]);

		status_t result
			= gCPUPriorityHeaps[core].Insert(&gCPUEntries[i], B_IDLE_PRIORITY);
		if (result != B_OK)
			return result;
	}

	// create per-logical processor run queues for pinned threads
	TRACE("scheduler_init(): creating %" B_PRId32 " per-cpu queue%s\n",
		cpuCount, cpuCount != 1 ? "s" : "");

	gPinnedRunQueues = new(std::nothrow) ThreadRunQueue[cpuCount];
	if (gPinnedRunQueues == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<ThreadRunQueue> pinnedRunQueuesDeleter(gPinnedRunQueues);
	for (int i = 0; i < cpuCount; i++) {
		status_t result = gPinnedRunQueues[i].GetInitStatus();
		if (result != B_OK)
			return result;
	}

	// create per-core run queues
	TRACE("scheduler_init(): creating %" B_PRId32 " per-core queue%s\n",
		coreCount, coreCount != 1 ? "s" : "");

	gRunQueues = new(std::nothrow) ThreadRunQueue[coreCount];
	if (gRunQueues == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<ThreadRunQueue> runQueuesDeleter(gRunQueues);
	for (int i = 0; i < coreCount; i++) {
		status_t result = gRunQueues[i].GetInitStatus();
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
	if (!gSingleCore) {
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

