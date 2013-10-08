/*
 * Copyright 2013, Paweł Dziepak, pdziepak@quarnos.org
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
#include <thread.h>
#include <timer.h>
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


const bigtime_t kThreadQuantum = 3000;


// The run queue. Holds the threads ready to run ordered by priority.
typedef RunQueue<Thread, THREAD_MAX_SET_PRIORITY> SimpleRunQueue;
static SimpleRunQueue* sRunQueue;
static SimpleRunQueue* sExpiredQueue;
static int32 sYieldedThreadPriority = -1;


struct scheduler_thread_data {
				scheduler_thread_data() { Init(); }
	void		Init();

	int32		priority_penalty;
	int32		forced_yield_count;

	bool		lost_cpu;
	bool		cpu_bound;

	bigtime_t	time_left;
	bigtime_t	stolen_time;
	bigtime_t	quantum_start;

	bigtime_t	went_sleep;
};


void
scheduler_thread_data::Init()
{
	priority_penalty = 0;
	forced_yield_count = 0;

	time_left = 0;
	stolen_time = 0;

	went_sleep = 0;

	lost_cpu = false;
	cpu_bound = true;
}


static inline void
dump_queue(SimpleRunQueue::ConstIterator& iterator)
{
	if (!iterator.HasNext())
		kprintf("Queue is empty.\n");
	else {
		kprintf("thread      id      priority penalty  name\n");
		while (iterator.HasNext()) {
			Thread* thread = iterator.Next();
			scheduler_thread_data* schedulerThreadData
				= reinterpret_cast<scheduler_thread_data*>(
					thread->scheduler_data);
			kprintf("%p  %-7" B_PRId32 " %-8" B_PRId32 " %-8" B_PRId32 " %s\n",
				thread, thread->id, thread->priority,
				schedulerThreadData->priority_penalty, thread->name);
		}
	}
}


static int
dump_run_queue(int argc, char** argv)
{
	SimpleRunQueue::ConstIterator iterator;
	kprintf("Current run queue:\n");
	iterator = sRunQueue->GetConstIterator();
	dump_queue(iterator);

	kprintf("\nExpired run queue:\n");
	iterator = sExpiredQueue->GetConstIterator();
	dump_queue(iterator);

	return 0;
}


static inline int32
simple_get_effective_priority(Thread* thread)
{
	if (thread->priority == B_IDLE_PRIORITY)
		return thread->priority;
	if (thread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return thread->priority;

	scheduler_thread_data* schedulerThreadData
		= reinterpret_cast<scheduler_thread_data*>(thread->scheduler_data);

	int32 effectivePriority = thread->priority;
	effectivePriority -= schedulerThreadData->priority_penalty;

	ASSERT(effectivePriority < B_FIRST_REAL_TIME_PRIORITY);
	ASSERT(effectivePriority >= B_LOWEST_ACTIVE_PRIORITY);

	return effectivePriority;
}


static inline void
simple_yield(Thread* thread)
{
	TRACE("thread %ld yielded\n", thread->id);
	int32 effectivePriority = simple_get_effective_priority(thread);
	sYieldedThreadPriority = max_c(sYieldedThreadPriority, effectivePriority);
}


static inline bool
simple_should_force_yield(Thread* thread)
{
	if (thread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return false;

	scheduler_thread_data* schedulerThreadData
		= reinterpret_cast<scheduler_thread_data*>(thread->scheduler_data);

	const int kYieldFrequency = 1 << (min_c(thread->priority, 25) / 5 + 1);

	return schedulerThreadData->forced_yield_count != 0
		&& schedulerThreadData->forced_yield_count % kYieldFrequency == 0;
}


static inline void
simple_increase_penalty(Thread* thread)
{
	if (thread->priority <= B_LOWEST_ACTIVE_PRIORITY)
		return;
	if (thread->priority >= B_FIRST_REAL_TIME_PRIORITY)
		return;

	TRACE("increasing thread %ld penalty\n", thread->id);

	scheduler_thread_data* schedulerThreadData
		= reinterpret_cast<scheduler_thread_data*>(thread->scheduler_data);
	int32 oldPenalty = schedulerThreadData->priority_penalty++;

	ASSERT(thread->priority - oldPenalty >= B_LOWEST_ACTIVE_PRIORITY);
	const int kMinimalPriority
		= min_c(thread->priority, 25) / 5;
	if (thread->priority - oldPenalty <= kMinimalPriority) {
		schedulerThreadData->priority_penalty = oldPenalty;
		schedulerThreadData->forced_yield_count++;
	}
}


static inline void
simple_cancel_penalty(Thread* thread)
{
	scheduler_thread_data* schedulerThreadData
		= reinterpret_cast<scheduler_thread_data*>(thread->scheduler_data);

	if (schedulerThreadData->priority_penalty != 0)
		TRACE("cancelling thread %ld penalty\n", thread->id);
	schedulerThreadData->priority_penalty = 0;
	schedulerThreadData->forced_yield_count = 0;
}


static void
simple_enqueue(Thread* thread, bool newOne)
{
	thread->state = thread->next_state = B_THREAD_READY;

	scheduler_thread_data* schedulerThreadData
		= reinterpret_cast<scheduler_thread_data*>(thread->scheduler_data);

	bigtime_t hasSlept = system_time() - schedulerThreadData->went_sleep;
	if (newOne && hasSlept > kThreadQuantum)
		simple_cancel_penalty(thread);

	if (simple_should_force_yield(thread))
		simple_yield(thread);

	int32 threadPriority = simple_get_effective_priority(thread);
	T(EnqueueThread(thread, threadPriority));

	if (threadPriority <= sYieldedThreadPriority)
		sExpiredQueue->PushBack(thread, threadPriority);
	else
		sRunQueue->PushBack(thread, threadPriority);

	schedulerThreadData->cpu_bound = true;
	schedulerThreadData->time_left = 0;
	schedulerThreadData->stolen_time = 0;

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadEnqueuedInRunQueue,
		thread);

	Thread* currentThread = thread_get_current_thread();
	if (newOne && threadPriority > currentThread->priority) {
		scheduler_thread_data* schedulerCurrentThreadData
			= reinterpret_cast<scheduler_thread_data*>(
				currentThread->scheduler_data);

		schedulerCurrentThreadData->lost_cpu = true;
		gCPU[0].invoke_scheduler = true;
		gCPU[0].invoke_scheduler_if_idle = false;
	}
}


/*!	Enqueues the thread into the run queue.
	Note: thread lock must be held when entering this function
*/
static void
simple_enqueue_in_run_queue(Thread* thread)
{
	simple_enqueue(thread, true);
}


/*!	Sets the priority of a thread.
	Note: thread lock must be held when entering this function
*/
static void
simple_set_thread_priority(Thread* thread, int32 priority)
{
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

	// remove thread from run queue
	sRunQueue->Remove(thread);

	// set priority and re-insert
	simple_cancel_penalty(thread);
	thread->priority = priority;

	simple_enqueue_in_run_queue(thread);
}


static bigtime_t
simple_estimate_max_scheduling_latency(Thread* thread)
{
	// TODO: This is probably meant to be called periodically to return the
	// current estimate depending on the system usage; we return fixed estimates
	// per thread priority, though.

	if (thread->priority >= B_REAL_TIME_DISPLAY_PRIORITY)
		return kThreadQuantum / 4;
	if (thread->priority >= B_DISPLAY_PRIORITY)
		return kThreadQuantum;

	return 2 * kThreadQuantum;
}


static int32
reschedule_event(timer* /* unused */)
{
	// This function is called as a result of the timer event set by the
	// scheduler. Make sure the reschedule() is invoked.
	Thread* thread= thread_get_current_thread();
	scheduler_thread_data* schedulerThreadData
		= reinterpret_cast<scheduler_thread_data*>(thread->scheduler_data);

	schedulerThreadData->lost_cpu = true;
	thread->cpu->invoke_scheduler = true;
	thread->cpu->invoke_scheduler_if_idle = false;
	thread->cpu->preempted = 1;
	return B_HANDLED_INTERRUPT;
}


static inline bool 
simple_quantum_ended(Thread* thread, bool wasPreempted, bool hasYielded)
{
	scheduler_thread_data* schedulerThreadData
		= reinterpret_cast<scheduler_thread_data*>(thread->scheduler_data);

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
simple_compute_quantum(Thread* thread)
{
	scheduler_thread_data* schedulerThreadData
		= reinterpret_cast<scheduler_thread_data*>(thread->scheduler_data);

	bigtime_t quantum;
	if (schedulerThreadData->time_left != 0)
		quantum = schedulerThreadData->time_left;
	else
		quantum = kThreadQuantum;

	quantum += schedulerThreadData->stolen_time;
	schedulerThreadData->stolen_time = 0;

	schedulerThreadData->time_left = quantum;
	schedulerThreadData->quantum_start = system_time();

	return quantum;
}


static inline Thread*
simple_get_next_thread(void)
{
	Thread* thread;
	do {
		thread = sRunQueue->PeekMaximum();

		if (sYieldedThreadPriority >= 0 && thread != NULL
			&& thread_is_idle_thread(thread)) {
			sRunQueue->Remove(thread);
			simple_enqueue_in_run_queue(thread);
			continue;
		}

		break;
	} while (true);
	if (thread == NULL && sYieldedThreadPriority >= 0) {
		SimpleRunQueue* temp = sRunQueue;
		sRunQueue = sExpiredQueue;
		sExpiredQueue = temp;
		sYieldedThreadPriority = -1;

		thread = sRunQueue->PeekMaximum();
	}

	return thread;
}


/*!	Runs the scheduler.
	Note: expects thread spinlock to be held
*/
static void
simple_reschedule(void)
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

	TRACE("reschedule(): current thread = %ld\n", oldThread->id);

	oldThread->state = oldThread->next_state;
	scheduler_thread_data* schedulerOldThreadData
		= reinterpret_cast<scheduler_thread_data*>(oldThread->scheduler_data);

	switch (oldThread->next_state) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			if (!schedulerOldThreadData->lost_cpu)
				schedulerOldThreadData->cpu_bound = false;

			if (simple_quantum_ended(oldThread, oldThread->cpu->preempted,
					oldThread->has_yielded)) {
				if (schedulerOldThreadData->cpu_bound)
					simple_increase_penalty(oldThread);

				if (oldThread->has_fully_yielded)
					simple_yield(oldThread);

				TRACE("enqueueing thread %ld into run queue priority = %ld\n",
					oldThread->id, simple_get_effective_priority(oldThread));
				simple_enqueue(oldThread, false);
			} else {
				TRACE("putting thread %ld back in run queue priority = %ld\n",
					oldThread->id, simple_get_effective_priority(oldThread));
				sRunQueue->PushFront(oldThread,
					simple_get_effective_priority(oldThread));
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
	oldThread->has_fully_yielded = false;
	schedulerOldThreadData->lost_cpu = false;

	// select thread with the biggest priority
	Thread* nextThread = simple_get_next_thread();
	if (!nextThread)
		panic("reschedule(): run queues are empty!\n");
	sRunQueue->Remove(nextThread);

	TRACE("reschedule(): next thread = %ld\n", nextThread->id);

	T(ScheduleThread(nextThread, oldThread));

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadScheduled,
		oldThread, nextThread);

	nextThread->state = B_THREAD_RUNNING;
	nextThread->next_state = B_THREAD_READY;

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
			bigtime_t quantum = simple_compute_quantum(oldThread);
			add_timer(quantumTimer, &reschedule_event, quantum,
				B_ONE_SHOT_RELATIVE_TIMER | B_TIMER_ACQUIRE_SCHEDULER_LOCK);
		}

		if (nextThread != oldThread)
			scheduler_switch_thread(oldThread, nextThread);
	}
}


static status_t
simple_on_thread_create(Thread* thread, bool idleThread)
{
	thread->scheduler_data = new (std::nothrow)scheduler_thread_data;
	if (thread->scheduler_data == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


static void
simple_on_thread_init(Thread* thread)
{
	scheduler_thread_data* schedulerThreadData
		= reinterpret_cast<scheduler_thread_data*>(thread->scheduler_data);
	schedulerThreadData->Init();
}


static void
simple_on_thread_destroy(Thread* thread)
{
	delete thread->scheduler_data;
}


/*!	This starts the scheduler. Must be run in the context of the initial idle
	thread. Interrupts must be disabled and will be disabled when returning.
*/
static void
simple_start(void)
{
	SpinLocker schedulerLocker(gSchedulerLock);

	simple_reschedule();
}


static scheduler_ops kSimpleOps = {
	simple_enqueue_in_run_queue,
	simple_reschedule,
	simple_set_thread_priority,
	simple_estimate_max_scheduling_latency,
	simple_on_thread_create,
	simple_on_thread_init,
	simple_on_thread_destroy,
	simple_start
};


// #pragma mark -


status_t
scheduler_simple_init()
{
	sRunQueue = new(std::nothrow) SimpleRunQueue;
	if (sRunQueue == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<SimpleRunQueue> runQueueDeleter(sRunQueue);

	sExpiredQueue = new(std::nothrow) SimpleRunQueue;
	if (sExpiredQueue == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<SimpleRunQueue> expiredQueueDeleter(sExpiredQueue);

	status_t result = sRunQueue->GetInitStatus();
	if (result != B_OK)
		return result;

	gScheduler = &kSimpleOps;

	add_debugger_command_etc("run_queue", &dump_run_queue,
		"List threads in run queue", "\nLists threads in run queue", 0);

	runQueueDeleter.Detach();
	expiredQueueDeleter.Detach();
	return B_OK;
}
