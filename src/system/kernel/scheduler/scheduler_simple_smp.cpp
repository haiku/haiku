/*
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


const bigtime_t kThreadQuantum = 3000;


// The run queue. Holds the threads ready to run ordered by priority.
static Thread *sRunQueue = NULL;
static int32 sCPUCount = 1;
static int32 sNextCPUForSelection = 0;


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
	Thread *thread;

	thread = sRunQueue;
	if (!thread)
		kprintf("Run queue is empty!\n");
	else {
		kprintf("thread    id      priority name\n");
		while (thread) {
			kprintf("%p  %-7ld %-8ld %s\n", thread, thread->id,
				thread->priority, thread->name);
			thread = thread->queue_next;
		}
	}

	return 0;
}


static int32
select_cpu(int32 currentCPU, Thread* thread, int32& targetPriority)
{
	if (thread->pinned_to_cpu > 0) {
		// the thread is pinned to a specific CPU
		int32 targetCPU = thread->previous_cpu->cpu_num;
		targetPriority = gCPU[targetCPU].running_thread->priority;
		return targetCPU;
	}

	// Choose the CPU running the lowest priority thread. Favor the current CPU
	// as it doesn't require ICI to be notified.
	int32 targetCPU = currentCPU;
	targetPriority = B_IDLE_PRIORITY;
	if (gCPU[currentCPU].disabled)
		targetCPU = -1;
	else
		targetPriority = gCPU[currentCPU].running_thread->priority;

	int32 cpu = sNextCPUForSelection;
	for (int32 i = 0; i < sCPUCount; i++, cpu++) {
		if (cpu >= sCPUCount)
			cpu = 0;

		if (!gCPU[cpu].disabled) {
			int32 cpuPriority = gCPU[cpu].running_thread->priority;
			if (targetCPU < 0 || cpuPriority < targetPriority) {
				targetCPU = cpu;
				targetPriority = cpuPriority;
			}
		}
	}

	if (++sNextCPUForSelection >= sCPUCount)
		sNextCPUForSelection = 0;

	return targetCPU;
}


/*!	Enqueues the thread into the run queue.
	Note: thread lock must be held when entering this function
*/
static void
enqueue_in_run_queue(Thread *thread)
{
	thread->state = thread->next_state = B_THREAD_READY;

	Thread *curr, *prev;
	for (curr = sRunQueue, prev = NULL; curr
			&& curr->priority >= thread->next_priority;
			curr = curr->queue_next) {
		if (prev)
			prev = prev->queue_next;
		else
			prev = sRunQueue;
	}

	T(EnqueueThread(thread, prev, curr));

	thread->queue_next = curr;
	if (prev)
		prev->queue_next = thread;
	else
		sRunQueue = thread;

	thread->next_priority = thread->priority;

	if (thread->priority != B_IDLE_PRIORITY) {
		// Select a CPU for the thread to run on. It's not certain that the
		// thread will actually run on it, but we will notify the CPU to
		// preempt the thread it is currently running, if the new thread has
		// a higher priority.
		int32 currentCPU = smp_get_current_cpu();
		int32 targetPriority;
		int32 targetCPU = select_cpu(currentCPU, thread, targetPriority);

		// If the target CPU runs a thread with a lower priority, tell it to
		// reschedule.
		if (thread->priority > targetPriority) {
			if (targetCPU == currentCPU) {
				gCPU[targetCPU].invoke_scheduler = true;
				gCPU[targetCPU].invoke_scheduler_if_idle = false;
			} else {
				if (targetPriority == B_IDLE_PRIORITY) {
					smp_send_ici(targetCPU, SMP_MSG_RESCHEDULE_IF_IDLE, 0, 0,
						0, NULL, SMP_MSG_FLAG_ASYNC);
				} else {
					smp_send_ici(targetCPU, SMP_MSG_RESCHEDULE, 0, 0, 0, NULL,
						SMP_MSG_FLAG_ASYNC);
				}
			}
		}
	}

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadEnqueuedInRunQueue,
		thread);
}


/*!	Sets the priority of a thread.
	Note: thread lock must be held when entering this function
*/
static void
set_thread_priority(Thread *thread, int32 priority)
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

	// find thread in run queue
	Thread *item, *prev;
	for (item = sRunQueue, prev = NULL; item && item != thread;
			item = item->queue_next) {
		if (prev)
			prev = prev->queue_next;
		else
			prev = sRunQueue;
	}

	ASSERT(item == thread);

	// remove the thread
	if (prev)
		prev->queue_next = item->queue_next;
	else
		sRunQueue = item->queue_next;

	// set priority and re-insert
	thread->priority = thread->next_priority = priority;
	enqueue_in_run_queue(thread);
}


static bigtime_t
estimate_max_scheduling_latency(Thread* thread)
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
reschedule(void)
{
	Thread *oldThread = thread_get_current_thread();
	Thread *nextThread, *prevThread;

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

	TRACE(("reschedule(): cpu %ld, cur_thread = %ld\n", smp_get_current_cpu(),
		thread_get_current_thread()->id));

	oldThread->state = oldThread->next_state;
	switch (oldThread->next_state) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			TRACE(("enqueueing thread %ld into run q. pri = %ld\n",
				oldThread->id, oldThread->priority));
			enqueue_in_run_queue(oldThread);
			break;
		case B_THREAD_SUSPENDED:
			TRACE(("reschedule(): suspending thread %ld\n", oldThread->id));
			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			break;
		default:
			TRACE(("not enqueueing thread %ld into run q. next_state = %ld\n",
				oldThread->id, oldThread->next_state));
			break;
	}

	nextThread = sRunQueue;
	prevThread = NULL;

	if (oldThread->cpu->disabled) {
		// CPU is disabled - service any threads we may have that are pinned,
		// otherwise just select the idle thread
		while (nextThread != NULL && nextThread->priority > B_IDLE_PRIORITY) {
			if (nextThread->pinned_to_cpu > 0
				&& nextThread->previous_cpu == oldThread->cpu)
				break;
			prevThread = nextThread;
			nextThread = nextThread->queue_next;
		}
	} else {
		while (nextThread != NULL) {
			// select next thread from the run queue
			// TODO: nextThread cannot really be NULL here, so we should be able
			// to remove the check, as well as the panic later on.
			while (nextThread != NULL
				&& nextThread->priority > B_IDLE_PRIORITY) {
#if 0
				if (oldThread == nextThread && nextThread->was_yielded) {
					// ignore threads that called thread_yield() once
					nextThread->was_yielded = false;
					prevThread = nextThread;
					nextThread = nextThread->queue_next;
				}
#endif

				// skip thread, if it doesn't want to run on this CPU
				if (nextThread->pinned_to_cpu > 0
					&& nextThread->previous_cpu != oldThread->cpu) {
					prevThread = nextThread;
					nextThread = nextThread->queue_next;
					continue;
				}

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
				// never skip last non-idle normal thread
				if (lowerNextThread == NULL
					|| lowerNextThread->priority == B_IDLE_PRIORITY)
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

			if (nextThread != NULL && nextThread->cpu
				&& nextThread->cpu->cpu_num != oldThread->cpu->cpu_num) {
				panic("thread in run queue that's still running on another CPU!\n");
				// TODO: remove this check completely when we're sure that this
				// cannot happen anymore.
				prevThread = nextThread;
				nextThread = nextThread->queue_next;
				continue;
			}

			break;
		}
	}

	if (nextThread == NULL)
		panic("reschedule(): run queue is empty!\n");

	// extract selected thread from the run queue
	if (prevThread != NULL)
		prevThread->queue_next = nextThread->queue_next;
	else
		sRunQueue = nextThread->queue_next;

	T(ScheduleThread(nextThread, oldThread));

	// notify listeners
	NotifySchedulerListeners(&SchedulerListener::ThreadScheduled, oldThread,
		nextThread);

	nextThread->state = B_THREAD_RUNNING;
	nextThread->next_state = B_THREAD_READY;
	oldThread->was_yielded = false;

	// track kernel time (user time is tracked in thread_at_kernel_entry())
	scheduler_update_thread_times(oldThread, nextThread);

	// track CPU activity
	if (!thread_is_idle_thread(oldThread)) {
		oldThread->cpu->active_time
			+= (oldThread->kernel_time - oldThread->cpu->last_kernel_time)
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
on_thread_create(Thread* thread, bool idleThread)
{
	// do nothing
	return B_OK;
}


static void
on_thread_init(Thread* thread)
{
	// do nothing
}


static void
on_thread_destroy(Thread* thread)
{
	// do nothing
}


/*!	This starts the scheduler. Must be run in the context of the initial idle
	thread. Interrupts must be disabled and will be disabled when returning.
*/
static void
start(void)
{
	SpinLocker schedulerLocker(gSchedulerLock);

	reschedule();
}


static scheduler_ops kSimpleSMPOps = {
	enqueue_in_run_queue,
	reschedule,
	set_thread_priority,
	estimate_max_scheduling_latency,
	on_thread_create,
	on_thread_init,
	on_thread_destroy,
	start
};


// #pragma mark -


void
scheduler_simple_smp_init()
{
	sCPUCount = smp_get_num_cpus();

	gScheduler = &kSimpleSMPOps;

	add_debugger_command_etc("run_queue", &dump_run_queue,
		"List threads in run queue", "\nLists threads in run queue", 0);
}
