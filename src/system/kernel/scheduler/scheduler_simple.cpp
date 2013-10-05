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
#	define TRACE(x) dprintf_no_syslog x
#else
#	define TRACE(x) ;
#endif


const bigtime_t kThreadQuantum = 3000;


// The run queue. Holds the threads ready to run ordered by priority.
static RunQueue<Thread, THREAD_MAX_SET_PRIORITY>* sRunQueue;


static int
dump_run_queue(int argc, char **argv)
{
	RunQueue<Thread, THREAD_MAX_SET_PRIORITY>::ConstIterator iterator;
	iterator = sRunQueue->GetConstIterator();

	if (!iterator.HasNext())
		kprintf("Run queue is empty!\n");
	else {
		kprintf("thread    id      priority name\n");
		while (iterator.HasNext()) {
			Thread *thread = iterator.Next();
			kprintf("%p  %-7" B_PRId32 " %-8" B_PRId32 " %s\n", thread,
				thread->id, thread->priority, thread->name);
		}
	}

	return 0;
}


/*!	Enqueues the thread into the run queue.
	Note: thread lock must be held when entering this function
*/
static void
simple_enqueue_in_run_queue(Thread *thread)
{
	thread->state = thread->next_state = B_THREAD_READY;

	//T(EnqueueThread(thread, prev, curr));

	sRunQueue->PushBack(thread, thread->next_priority);
	thread->next_priority = thread->priority;

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadEnqueuedInRunQueue,
		thread);

	if (thread->priority > thread_get_current_thread()->priority) {
		gCPU[0].invoke_scheduler = true;
		gCPU[0].invoke_scheduler_if_idle = false;
	}
}


/*!	Sets the priority of a thread.
	Note: thread lock must be held when entering this function
*/
static void
simple_set_thread_priority(Thread *thread, int32 priority)
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
	thread->priority = thread->next_priority = priority;
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
simple_reschedule(void)
{
	Thread* oldThread = thread_get_current_thread();
	Thread* nextThread;

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

	TRACE(("reschedule(): current thread = %ld\n", oldThread->id));

	oldThread->state = oldThread->next_state;
	switch (oldThread->next_state) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			TRACE(("enqueueing thread %ld into run queue priority = %ld\n",
				oldThread->id, oldThread->priority));
			simple_enqueue_in_run_queue(oldThread);
			break;
		case B_THREAD_SUSPENDED:
			TRACE(("reschedule(): suspending thread %ld\n", oldThread->id));
			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			break;
		default:
			TRACE(("not enqueueing thread %ld into run queue next_state = %ld\n",
				oldThread->id, oldThread->next_state));
			break;
	}

	while (true) {
		// select thread with the biggest priority
		nextThread = sRunQueue->PeekMaximum();
		if (!nextThread)
			panic("reschedule(): run queue is empty!\n");

#if 0
		if (oldThread == nextThread && nextThread->was_yielded) {
			// ignore threads that called thread_yield() once
			nextThread->was_yielded = false;
			prevThread = nextThread;
			nextThread = nextThread->queue_next;
		}
#endif

		// always extract real time threads
		if (nextThread->priority >= B_FIRST_REAL_TIME_PRIORITY)
			break;

		// find thread with the second biggest priority
		Thread* lowerNextThread = sRunQueue->PeekSecondMaximum();

		// never skip last non-idle normal thread
		if (lowerNextThread == NULL
			|| lowerNextThread->priority == B_IDLE_PRIORITY)
			break;

		int32 priorityDiff = nextThread->priority - lowerNextThread->priority;
		if (priorityDiff > 15)
			break;

		// skip normal threads sometimes
		// (twice as probable per priority level)
		if ((fast_random_value() >> (15 - priorityDiff)) != 0)
			break;

		nextThread = lowerNextThread;

		break;
	}

	sRunQueue->Remove(nextThread);

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
		oldThread->cpu->active_time +=
			(oldThread->kernel_time - oldThread->cpu->last_kernel_time)
			+ (oldThread->user_time - oldThread->cpu->last_user_time);
	}

	if (!thread_is_idle_thread(nextThread)) {
		oldThread->cpu->last_kernel_time = nextThread->kernel_time;
		oldThread->cpu->last_user_time = nextThread->user_time;
	}

	if (nextThread != oldThread || oldThread->cpu->preempted) {
		bigtime_t quantum = kThreadQuantum;	// TODO: calculate quantum?
		timer* quantumTimer = &oldThread->cpu->quantum_timer;

		if (!oldThread->cpu->preempted)
			cancel_timer(quantumTimer);

		oldThread->cpu->preempted = 0;
		if (!thread_is_idle_thread(nextThread)) {
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
	// do nothing
	return B_OK;
}


static void
simple_on_thread_init(Thread* thread)
{
	// do nothing
}


static void
simple_on_thread_destroy(Thread* thread)
{
	// do nothing
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


void
scheduler_simple_init()
{
	sRunQueue = new(std::nothrow) RunQueue<Thread, THREAD_MAX_SET_PRIORITY>;

	gScheduler = &kSimpleOps;

	add_debugger_command_etc("run_queue", &dump_run_queue,
		"List threads in run queue", "\nLists threads in run queue", 0);
}
