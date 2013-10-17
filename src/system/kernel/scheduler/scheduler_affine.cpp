/*
 * Copyright 2013. Paweł Dziepak, pdziepak@quarnos.org.
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


struct CPUHeapEntry : public MinMaxHeapLinkImpl<CPUHeapEntry, int32> {
	int32		fCPUNumber;
};

static CPUHeapEntry* sCPUPriorityEntries;
typedef MinMaxHeap<CPUHeapEntry, int32> AffineCPUHeap;
static AffineCPUHeap* sCPUPriorityHeaps;

struct CoreHeapEntry : public HeapLinkImpl<CoreHeapEntry, int32> {
	int32		fCoreID;
};

static CoreHeapEntry* sCorePriorityEntries;
typedef Heap<CoreHeapEntry, int32> AffineCoreHeap;
static AffineCoreHeap* sCorePriorityHeap;

// The run queues. Holds the threads ready to run ordered by priority.
// One queue per schedulable target per core. Additionally, each
// logical processor has its sPinnedRunQueues used for scheduling
// pinned threads.
typedef RunQueue<Thread, THREAD_MAX_SET_PRIORITY> AffineRunQueue;
static AffineRunQueue* sRunQueues;
static AffineRunQueue* sPinnedRunQueues;
static int32 sRunQueueCount;
static int32* sCPUToCore;


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
	AffineCPUHeap temp;

	kprintf("cpu priority actual priority\n");
	CPUHeapEntry* entry = heap->PeekMinimum();
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


static int
dump_cpu_heap(int argc, char** argv)
{
	AffineCoreHeap temp;

	kprintf("core priority\n");
	CoreHeapEntry* entry = sCorePriorityHeap->PeekRoot();
	while (entry) {
		int32 core = entry->fCoreID;
		int32 key = AffineCoreHeap::GetKey(entry);
		kprintf("%4" B_PRId32 " %8" B_PRId32 "\n", core, key);

		sCorePriorityHeap->RemoveRoot();
		temp.Insert(entry, key);

		entry = sCorePriorityHeap->PeekRoot();
	}

	entry = temp.PeekRoot();
	while (entry) {
		int32 key = AffineCoreHeap::GetKey(entry);
		temp.RemoveRoot();
		sCorePriorityHeap->Insert(entry, key);
		entry = temp.PeekRoot();
	}

	for (int32 i = 0; i < sRunQueueCount; i++) {
		kprintf("\nCore %" B_PRId32 " heap:\n", i);
		dump_heap(&sCPUPriorityHeaps[i]);
	}

	return 0;
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
	kprintf("\tprevious_core:\t\t%" B_PRId32 "\n",
		schedulerThreadData->previous_core);
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


/*!	Returns the most idle CPU based on the active time counters.
	Note: thread lock must be held when entering this function
*/
#if 0
static int32
affine_get_most_idle_cpu()
{
	int32 targetCPU = -1;
	for (int32 i = 0; i < smp_get_num_cpus(); i++) {
		if (gCPU[i].disabled)
			continue;
		if (targetCPU < 0 || sRunQueueSize[i] < sRunQueueSize[targetCPU])
			targetCPU = i;
	}

	return targetCPU;
}
#endif


static inline void
affine_update_priority_heaps(int32 cpu, int32 priority)
{
	int32 core = sCPUToCore[cpu];

	sCPUPriorityHeaps[core].ModifyKey(&sCPUPriorityEntries[cpu], priority);

	int32 maxPriority
		= AffineCPUHeap::GetKey(sCPUPriorityHeaps[core].PeekMaximum());
	int32 corePriority = AffineCoreHeap::GetKey(&sCorePriorityEntries[core]);

	if (corePriority != maxPriority)
		sCorePriorityHeap->ModifyKey(&sCorePriorityEntries[core], maxPriority);
}


static inline int32
affine_choose_core(void)
{
	CoreHeapEntry* entry = sCorePriorityHeap->PeekRoot();
	ASSERT(entry != NULL);
	return entry->fCoreID;
}


static inline int32
affine_choose_cpu(int32 core)
{
	CPUHeapEntry* entry = sCPUPriorityHeaps[core].PeekMinimum();
	ASSERT(entry != NULL);
	return entry->fCPUNumber;
}


static void
affine_enqueue(Thread* thread, bool newOne)
{
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
	} else if (schedulerThreadData->previous_core < 0) {
		if (thread->priority == B_IDLE_PRIORITY) {
			static int32 idleThreads = 0;
			targetCPU = idleThreads++;
			targetCore = sCPUToCore[targetCPU];
		} else {
			targetCore = affine_choose_core();
			targetCPU = affine_choose_cpu(targetCore);
		}
		schedulerThreadData->previous_core = targetCore;
	} else {
		targetCore = schedulerThreadData->previous_core;
		targetCPU = affine_choose_cpu(targetCore);
	}

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

		if (targetCPU == smp_get_current_cpu()) {
			gCPU[targetCPU].invoke_scheduler = true;
			gCPU[targetCPU].invoke_scheduler_if_idle = false;
		} else {
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


#if 0
/*!	Dequeues the thread after the given \a prevThread from the run queue.
*/
static inline Thread *
dequeue_from_run_queue(Thread *prevThread, int32 currentCPU)
{
	Thread *resultThread = NULL;
	if (prevThread != NULL) {
		resultThread = prevThread->queue_next;
		prevThread->queue_next = resultThread->queue_next;
	} else {
		resultThread = sRunQueue[currentCPU];
		sRunQueue[currentCPU] = resultThread->queue_next;
	}
	sRunQueueSize[currentCPU]--;
	resultThread->scheduler_data->fLastQueue = -1;

	return resultThread;
}


/*!	Looks for a possible thread to grab/run from another CPU.
	Note: thread lock must be held when entering this function
*/
static Thread *
steal_thread_from_other_cpus(int32 currentCPU)
{
	// look through the active CPUs - find the one
	// that has a) threads available to steal, and
	// b) out of those, the one that's the most CPU-bound
	// TODO: make this more intelligent along with enqueue
	// - we need to try and maintain a reasonable balance
	// in run queue sizes across CPUs, and also try to maintain
	// an even distribution of cpu bound / interactive threads
	int32 targetCPU = -1;
	for (int32 i = 0; i < smp_get_num_cpus(); i++) {
		// skip CPUs that have either no or only one thread
		if (i == currentCPU || sRunQueueSize[i] < 2)
			continue;

		// out of the CPUs with threads available to steal,
		// pick whichever one is generally the most CPU bound.
		if (targetCPU < 0
			|| sRunQueue[i]->priority > sRunQueue[targetCPU]->priority
			|| (sRunQueue[i]->priority == sRunQueue[targetCPU]->priority
				&& sRunQueueSize[i] > sRunQueueSize[targetCPU]))
			targetCPU = i;
	}

	if (targetCPU < 0)
		return NULL;

	Thread* nextThread = sRunQueue[targetCPU];
	Thread* prevThread = NULL;

	while (nextThread != NULL) {
		// grab the highest priority non-pinned thread
		// out of this CPU's queue, dequeue and return it
		if (nextThread->pinned_to_cpu <= 0) {
			dequeue_from_run_queue(prevThread, targetCPU);
			return nextThread;
		}

		prevThread = nextThread;
		nextThread = nextThread->queue_next;
	}

	return NULL;
}
#endif


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
		affine_update_priority_heaps(thread->cpu->cpu_num, priority);

	if (thread->state != B_THREAD_READY) {
		affine_cancel_penalty(thread);
		thread->priority = priority;
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
	thread->cpu->invoke_scheduler_if_idle = false;
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


/*!	Runs the scheduler.
	Note: expects thread spinlock to be held
*/
static void
affine_reschedule(void)
{
	Thread* oldThread = thread_get_current_thread();

	// check whether we're only supposed to reschedule, if the current thread
	// is idle
	if (oldThread->cpu->invoke_scheduler) {
		oldThread->cpu->invoke_scheduler = false;
		if (oldThread->cpu->invoke_scheduler_if_idle
			&& oldThread->priority != B_IDLE_PRIORITY) {
			oldThread->cpu->invoke_scheduler_if_idle = false;
			return;
		}
	}

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
			schedulerOldThreadData->went_sleep = system_time();
			TRACE("reschedule(): suspending thread %ld\n", oldThread->id);
			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			break;
		default:
			schedulerOldThreadData->went_sleep = system_time();
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
	if (!thread_is_idle_thread(oldThread)) {
		oldThread->cpu->active_time +=
			(oldThread->kernel_time - oldThread->cpu->last_kernel_time)
			+ (oldThread->user_time - oldThread->cpu->last_user_time);
	}

	if (!thread_is_idle_thread(nextThread)) {
		oldThread->cpu->last_kernel_time = nextThread->kernel_time;
		oldThread->cpu->last_user_time = nextThread->user_time;
	}

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


static scheduler_ops kAffineOps = {
	affine_enqueue_in_run_queue,
	affine_reschedule,
	affine_set_thread_priority,
	affine_estimate_max_scheduling_latency,
	affine_on_thread_create,
	affine_on_thread_init,
	affine_on_thread_destroy,
	affine_start,
	affine_dump_thread_data
};


// #pragma mark -


status_t
scheduler_affine_init()
{
	int32 cpuCount = smp_get_num_cpus();

	// create logical processor to core mapping
	sCPUToCore = new(std::nothrow) int32[cpuCount];
	if (sCPUToCore == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<int32> cpuToCoreDeleter(sCPUToCore);

	int32 coreCount = 0;
	for (int32 i = 0; i < cpuCount; i++) {
		if (gCPU[i].topology_id[CPU_TOPOLOGY_SMT] == 0)
			sCPUToCore[i] = coreCount++;
	}
	sRunQueueCount = coreCount;

	// TODO: Nasty O(n^2), solutions with better complexity will require
	// creating helper data structures. This code is run only once, so it
	// probably won't be a problem until we support systems with large
	// number of processors.
	for (int32 i = 0; i < cpuCount; i++) {
		if (gCPU[i].topology_id[CPU_TOPOLOGY_SMT] == 0)
			continue;

		for (int32 j = 0; j < cpuCount; j++) {
			bool sameCore = true;
			for (int32 k = 0; k < CPU_TOPOLOGY_LEVELS && sameCore; k++) {
				if (k == CPU_TOPOLOGY_SMT && gCPU[j].topology_id[k] == 0)
					continue;
				if (k == CPU_TOPOLOGY_SMT && gCPU[j].topology_id[k] != 0) {
					sameCore = false;
					continue;
				}

				if (gCPU[i].topology_id[k] != gCPU[j].topology_id[k])
					sameCore = false;
			}

			if (sameCore)
				sCPUToCore[i] = sCPUToCore[j];
		}
	}

	// create logical processor and core heaps
	sCPUPriorityEntries = new CPUHeapEntry[cpuCount];
	if (sCPUPriorityEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<CPUHeapEntry> cpuPriorityEntriesDeleter(sCPUPriorityEntries);

	sCorePriorityEntries = new CoreHeapEntry[coreCount];
	if (sCorePriorityEntries == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<CoreHeapEntry> corePriorityEntriesDeleter(
		sCorePriorityEntries);

	sCPUPriorityHeaps = new AffineCPUHeap[coreCount];
	if (sCPUPriorityHeaps == NULL)
		return B_NO_MEMORY;
	ArrayDeleter<AffineCPUHeap> cpuPriorityHeapDeleter(sCPUPriorityHeaps);

	for (int32 i = 0; i < cpuCount; i++) {
		sCPUPriorityEntries[i].fCPUNumber = i;
		int32 core = sCPUToCore[i];

		status_t result
			= sCPUPriorityHeaps[core].Insert(&sCPUPriorityEntries[i],
				B_IDLE_PRIORITY);
		if (result != B_OK)
			return result;
	}

	sCorePriorityHeap = new AffineCoreHeap;
	if (sCorePriorityHeap == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<AffineCoreHeap> corePriorityHeapDeleter(sCorePriorityHeap);

	for (int32 i = 0; i < coreCount; i++) {
		sCorePriorityEntries[i].fCoreID = i;
		status_t result = sCorePriorityHeap->Insert(&sCorePriorityEntries[i],
			B_IDLE_PRIORITY);
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

	gScheduler = &kAffineOps;

	add_debugger_command_etc("run_queue", &dump_run_queue,
		"List threads in run queue", "\nLists threads in run queue", 0);
	add_debugger_command_etc("cpu_heap", &dump_cpu_heap,
		"List CPUs in CPU priority heap", "\nList CPUs in CPU priority heap",
		0);

	runQueuesDeleter.Detach();
	pinnedRunQueuesDeleter.Detach();
	corePriorityHeapDeleter.Detach();
	cpuPriorityHeapDeleter.Detach();
	corePriorityEntriesDeleter.Detach();
	cpuPriorityEntriesDeleter.Detach();
	cpuToCoreDeleter.Detach();
	return B_OK;
}
