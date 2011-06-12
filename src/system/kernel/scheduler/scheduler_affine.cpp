/*
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

#include "scheduler_common.h"
#include "scheduler_tracing.h"


//#define TRACE_SCHEDULER
#ifdef TRACE_SCHEDULER
#	define TRACE(x) dprintf_no_syslog x
#else
#	define TRACE(x) ;
#endif

// The run queues. Holds the threads ready to run ordered by priority.
// One queue per schedulable target (CPU, core, etc.).
// TODO: consolidate this such that HT/SMT entities on the same physical core
// share a queue, once we have the necessary API for retrieving the topology
// information
static Thread* sRunQueue[B_MAX_CPU_COUNT];
static int32 sRunQueueSize[B_MAX_CPU_COUNT];
static Thread* sIdleThreads;

const int32 kMaxTrackingQuantums = 5;
const bigtime_t kMinThreadQuantum = 3000;
const bigtime_t kMaxThreadQuantum = 10000;


struct scheduler_thread_data {
	scheduler_thread_data(void)
	{
		Init();
	}


	void Init()
	{
		fQuantumAverage = 0;
		fLastQuantumSlot = 0;
		fLastQueue = -1;
		memset(fLastThreadQuantums, 0, sizeof(fLastThreadQuantums));
	}

	inline void SetQuantum(int32 quantum)
	{
		fQuantumAverage -= fLastThreadQuantums[fLastQuantumSlot];
		fLastThreadQuantums[fLastQuantumSlot] = quantum;
		fQuantumAverage += quantum;
		if (fLastQuantumSlot < kMaxTrackingQuantums - 1)
			++fLastQuantumSlot;
		else
			fLastQuantumSlot = 0;
	}

	inline int32 GetAverageQuantumUsage() const
	{
		return fQuantumAverage / kMaxTrackingQuantums;
	}

	int32 fQuantumAverage;
	int32 fLastThreadQuantums[kMaxTrackingQuantums];
	int16 fLastQuantumSlot;
	int32 fLastQueue;
};


static int
_rand(void)
{
	static int next = 0;

	if (next == 0)
		next = system_time();

	next = next * 1103515245 + 12345;
	return (next >> 16) & 0x7FFF;
}


static int
dump_run_queue(int argc, char **argv)
{
	Thread *thread = NULL;

	for (int32 i = 0; i < smp_get_num_cpus(); i++) {
		thread = sRunQueue[i];
		kprintf("Run queue for cpu %ld (%ld threads)\n", i,
			sRunQueueSize[i]);
		if (sRunQueueSize[i] > 0) {
			kprintf("thread      id      priority  avg. quantum  name\n");
			while (thread) {
				kprintf("%p  %-7ld %-8ld  %-12ld  %s\n", thread, thread->id,
					thread->priority,
					thread->scheduler_data->GetAverageQuantumUsage(),
					thread->name);
				thread = thread->queue_next;
			}
		}
	}
	return 0;
}


/*!	Returns the most idle CPU based on the active time counters.
	Note: thread lock must be held when entering this function
*/
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


/*!	Enqueues the thread into the run queue.
	Note: thread lock must be held when entering this function
*/
static void
affine_enqueue_in_run_queue(Thread *thread)
{
	int32 targetCPU = -1;
	if (thread->pinned_to_cpu > 0)
		targetCPU = thread->previous_cpu->cpu_num;
	else if (thread->previous_cpu == NULL || thread->previous_cpu->disabled)
		targetCPU = affine_get_most_idle_cpu();
	else
		targetCPU = thread->previous_cpu->cpu_num;

	thread->state = thread->next_state = B_THREAD_READY;

	if (thread->priority == B_IDLE_PRIORITY) {
		thread->queue_next = sIdleThreads;
		sIdleThreads = thread;
	} else {
		Thread *curr, *prev;
		for (curr = sRunQueue[targetCPU], prev = NULL; curr
			&& curr->priority >= thread->next_priority;
			curr = curr->queue_next) {
			if (prev)
				prev = prev->queue_next;
			else
				prev = sRunQueue[targetCPU];
		}

		T(EnqueueThread(thread, prev, curr));
		sRunQueueSize[targetCPU]++;
		thread->queue_next = curr;
		if (prev)
			prev->queue_next = thread;
		else
			sRunQueue[targetCPU] = thread;

		thread->scheduler_data->fLastQueue = targetCPU;
	}

	thread->next_priority = thread->priority;

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadEnqueuedInRunQueue,
		thread);

	if (thread->priority > gCPU[targetCPU].running_thread->priority) {
		if (targetCPU == smp_get_current_cpu()) {
			gCPU[targetCPU].invoke_scheduler = true;
			gCPU[targetCPU].invoke_scheduler_if_idle = false;
		} else {
			smp_send_ici(targetCPU, SMP_MSG_RESCHEDULE, 0, 0, 0, NULL,
				SMP_MSG_FLAG_ASYNC);
		}
	}
}


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


/*!	Sets the priority of a thread.
	Note: thread lock must be held when entering this function
*/
static void
affine_set_thread_priority(Thread *thread, int32 priority)
{
	int32 targetCPU = -1;

	if (priority == thread->priority)
		return;

	if (thread->state != B_THREAD_READY) {
		thread->priority = priority;
		return;
	}

	// The thread is in the run queue. We need to remove it and re-insert it at
	// a new position.

	T(RemoveThread(thread));

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadRemovedFromRunQueue,
		thread);

	// search run queues for the thread
	Thread *item = NULL, *prev = NULL;
	targetCPU = thread->scheduler_data->fLastQueue;

	for (item = sRunQueue[targetCPU], prev = NULL; item && item != thread;
			item = item->queue_next) {
		if (prev)
			prev = prev->queue_next;
		else
			prev = item;
	}

	ASSERT(item == thread);

	// remove the thread
	thread = dequeue_from_run_queue(prev, targetCPU);

	// set priority and re-insert
	thread->priority = thread->next_priority = priority;
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
	thread_get_current_thread()->cpu->invoke_scheduler = true;
	thread_get_current_thread()->cpu->invoke_scheduler_if_idle = false;
	thread_get_current_thread()->cpu->preempted = 1;
	return B_HANDLED_INTERRUPT;
}


/*!	Runs the scheduler.
	Note: expects thread spinlock to be held
*/
static void
affine_reschedule(void)
{
	int32 currentCPU = smp_get_current_cpu();
	Thread *oldThread = thread_get_current_thread();

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

	Thread *nextThread, *prevThread;

	TRACE(("reschedule(): cpu %ld, cur_thread = %ld\n", currentCPU, oldThread->id));

	oldThread->state = oldThread->next_state;
	switch (oldThread->next_state) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			TRACE(("enqueueing thread %ld into run q. pri = %ld\n", oldThread->id, oldThread->priority));
			affine_enqueue_in_run_queue(oldThread);
			break;
		case B_THREAD_SUSPENDED:
			TRACE(("reschedule(): suspending thread %ld\n", oldThread->id));
			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			break;
		default:
			TRACE(("not enqueueing thread %ld into run q. next_state = %ld\n", oldThread->id, oldThread->next_state));
			break;
	}

	nextThread = sRunQueue[currentCPU];
	prevThread = NULL;

	if (sRunQueue[currentCPU] != NULL) {
		TRACE(("dequeueing next thread from cpu %ld\n", currentCPU));
		// select next thread from the run queue
		while (nextThread->queue_next) {
			// always extract real time threads
			if (nextThread->priority >= B_FIRST_REAL_TIME_PRIORITY)
				break;

			// find next thread with lower priority
			Thread *lowerNextThread = nextThread->queue_next;
			Thread *lowerPrevThread = nextThread;
			int32 priority = nextThread->priority;

			while (lowerNextThread != NULL
				&& priority == lowerNextThread->priority) {
				lowerPrevThread = lowerNextThread;
				lowerNextThread = lowerNextThread->queue_next;
			}
			if (lowerNextThread == NULL)
				break;

			int32 priorityDiff = priority - lowerNextThread->priority;
			if (priorityDiff > 15)
				break;

			// skip normal threads sometimes
			// (twice as probable per priority level)
			if ((_rand() >> (15 - priorityDiff)) != 0)
				break;

			nextThread = lowerNextThread;
			prevThread = lowerPrevThread;
		}

		TRACE(("dequeuing thread %ld from cpu %ld\n", nextThread->id,
			currentCPU));
		// extract selected thread from the run queue
		dequeue_from_run_queue(prevThread, currentCPU);
	} else {
		if (!gCPU[currentCPU].disabled) {
			TRACE(("CPU %ld stealing thread from other CPUs\n", currentCPU));
			nextThread = steal_thread_from_other_cpus(currentCPU);
		} else
			nextThread = NULL;

		if (nextThread == NULL) {
			TRACE(("No threads to steal, grabbing from idle pool\n"));
			// no other CPU had anything for us to take,
			// grab one from the kernel's idle pool
			nextThread = sIdleThreads;
			if (nextThread)
				sIdleThreads = nextThread->queue_next;
		}
	}

	if (!nextThread)
		panic("reschedule(): run queue is empty!\n");

	T(ScheduleThread(nextThread, oldThread));

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadScheduled,
		oldThread, nextThread);

	nextThread->state = B_THREAD_RUNNING;
	nextThread->next_state = B_THREAD_READY;
	oldThread->was_yielded = false;

	// track kernel time (user time is tracked in thread_at_kernel_entry())
	scheduler_update_thread_times(oldThread, nextThread);

	// track CPU activity
	if (!thread_is_idle_thread(oldThread)) {
		bigtime_t activeTime =
			(oldThread->kernel_time - oldThread->cpu->last_kernel_time)
			+ (oldThread->user_time - oldThread->cpu->last_user_time);
		oldThread->cpu->active_time += activeTime;
		scheduler_thread_data *data = oldThread->scheduler_data;
		data->SetQuantum(activeTime);
	}

	if (!thread_is_idle_thread(nextThread)) {
		oldThread->cpu->last_kernel_time = nextThread->kernel_time;
		oldThread->cpu->last_user_time = nextThread->user_time;
	}

	if (nextThread != oldThread || oldThread->cpu->preempted) {
		timer *quantumTimer = &oldThread->cpu->quantum_timer;
		if (!oldThread->cpu->preempted)
			cancel_timer(quantumTimer);
		oldThread->cpu->preempted = 0;

		// we do not adjust the quantum for the idle thread as it is going to be
		// preempted most of the time and would likely get the longer quantum
		// over time, indeed we use a smaller quantum to avoid running idle too
		// long
		bigtime_t quantum = kMinThreadQuantum;
		// give CPU-bound background threads a larger quantum size
		// to minimize unnecessary context switches if the system is idle
		if (nextThread->priority != B_IDLE_PRIORITY
			&& nextThread->scheduler_data->GetAverageQuantumUsage()
			> (kMinThreadQuantum >> 1)
			&& nextThread->priority < B_NORMAL_PRIORITY)
			quantum = kMaxThreadQuantum;

		add_timer(quantumTimer, &reschedule_event, quantum,
			B_ONE_SHOT_RELATIVE_TIMER | B_TIMER_ACQUIRE_SCHEDULER_LOCK);

		if (nextThread != oldThread)
			scheduler_switch_thread(oldThread, nextThread);
	}
}


static status_t
affine_on_thread_create(Thread* thread, bool idleThread)
{
	// we don't need a data structure for the idle threads
	if (idleThread) {
		thread->scheduler_data = NULL;
		return B_OK;
	}

	thread->scheduler_data = new(std::nothrow) scheduler_thread_data();
	if (thread->scheduler_data == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


static void
affine_on_thread_init(Thread* thread)
{
	((scheduler_thread_data *)(thread->scheduler_data))->Init();
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
	affine_start
};


// #pragma mark -


void
scheduler_affine_init()
{
	gScheduler = &kAffineOps;
	memset(sRunQueue, 0, sizeof(sRunQueue));
	memset(sRunQueueSize, 0, sizeof(sRunQueueSize));
	add_debugger_command_etc("run_queue", &dump_run_queue,
		"List threads in run queue", "\nLists threads in run queue", 0);
}
