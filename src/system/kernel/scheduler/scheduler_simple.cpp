/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002, Angelo Mottola, a.mottola@libero.it.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*! The thread scheduler */


#include <OS.h>

#include <cpu.h>
#include <int.h>
#include <kernel.h>
#include <kscheduler.h>
#include <scheduler_defs.h>
#include <smp.h>
#include <thread.h>
#include <timer.h>
#include <user_debugger.h>

#include "scheduler_tracing.h"


//#define TRACE_SCHEDULER
#ifdef TRACE_SCHEDULER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// The run queue. Holds the threads ready to run ordered by priority.
static struct thread *sRunQueue = NULL;
static cpu_mask_t sIdleCPUs = 0;


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
	struct thread *thread;

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


/*!	Enqueues the thread into the run queue.
	Note: thread lock must be held when entering this function
*/
static void
simple_enqueue_in_run_queue(struct thread *thread)
{
	if (thread->state == B_THREAD_RUNNING) {
		// The thread is currently running (on another CPU) and we cannot
		// insert it into the run queue. Set the next state to ready so the
		// thread is inserted into the run queue on the next reschedule.
		thread->next_state = B_THREAD_READY;
		return;
	}

	thread->state = thread->next_state = B_THREAD_READY;

	struct thread *curr, *prev;
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
		int32 currentCPU = smp_get_current_cpu();
		if (sIdleCPUs != 0) {
			if (thread->pinned_to_cpu > 0) {
				// thread is pinned to a CPU -- notify it, if it is idle
				int32 targetCPU = thread->previous_cpu->cpu_num;
				if ((sIdleCPUs & (1 << targetCPU)) != 0) {
					sIdleCPUs &= ~(1 << targetCPU);
					smp_send_ici(targetCPU, SMP_MSG_RESCHEDULE_IF_IDLE, 0, 0,
						0, NULL, SMP_MSG_FLAG_ASYNC);
				}
			} else {
				// Thread is not pinned to any CPU -- take it ourselves, if we
				// are idle, otherwise notify the next idle CPU. In either case
				// we clear the idle bit of the chosen CPU, so that the
				// simple_enqueue_in_run_queue() won't try to bother the
				// same CPU again, if invoked before it handled the interrupt.
				cpu_mask_t idleCPUs = CLEAR_BIT(sIdleCPUs, currentCPU);
				if ((sIdleCPUs & (1 << currentCPU)) != 0) {
					sIdleCPUs = idleCPUs;
				} else {
					int32 targetCPU = 0;
					for (; targetCPU < B_MAX_CPU_COUNT; targetCPU++) {
						cpu_mask_t mask = 1 << targetCPU;
						if ((idleCPUs & mask) != 0) {
							sIdleCPUs &= ~mask;
							break;
						}
					}

					smp_send_ici(targetCPU, SMP_MSG_RESCHEDULE_IF_IDLE, 0, 0,
						0, NULL, SMP_MSG_FLAG_ASYNC);
				}
			}
		}
	}
}


/*!	Sets the priority of a thread.
	Note: thread lock must be held when entering this function
*/
static void
simple_set_thread_priority(struct thread *thread, int32 priority)
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

	// find thread in run queue
	struct thread *item, *prev;
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
	thread->priority = priority;
	simple_enqueue_in_run_queue(thread);
}


static void
context_switch(struct thread *fromThread, struct thread *toThread)
{
	if ((fromThread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_unscheduled(fromThread);

	toThread->previous_cpu = toThread->cpu = fromThread->cpu;
	fromThread->cpu = NULL;

	arch_thread_set_current_thread(toThread);
	arch_thread_context_switch(fromThread, toThread);

	// Looks weird, but is correct. fromThread had been unscheduled earlier,
	// but is back now. The notification for a thread scheduled the first time
	// happens in thread.cpp:thread_kthread_entry().
	if ((fromThread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_scheduled(fromThread);
}


static int32
reschedule_event(timer *unused)
{
	if (thread_get_current_thread()->keep_scheduled > 0)
		return B_HANDLED_INTERRUPT;

	// this function is called as a result of the timer event set by the
	// scheduler returning this causes a reschedule on the timer event
	thread_get_current_thread()->cpu->preempted = 1;
	return B_INVOKE_SCHEDULER;
}


/*!	Runs the scheduler.
	Note: expects thread spinlock to be held
*/
static void
simple_reschedule(void)
{
	struct thread *oldThread = thread_get_current_thread();
	struct thread *nextThread, *prevThread;

	TRACE(("reschedule(): cpu %d, cur_thread = %ld\n", smp_get_current_cpu(), thread_get_current_thread()->id));

	oldThread->cpu->invoke_scheduler = false;

	oldThread->state = oldThread->next_state;
	switch (oldThread->next_state) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			TRACE(("enqueueing thread %ld into run q. pri = %ld\n", oldThread->id, oldThread->priority));
			simple_enqueue_in_run_queue(oldThread);
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

	nextThread = sRunQueue;
	prevThread = NULL;

	if (oldThread->cpu->disabled) {
		// CPU is disabled - service any threads we may have that are pinned,
		// otherwise just select the idle thread
		while (nextThread && nextThread->priority > B_IDLE_PRIORITY) {
			if (nextThread->pinned_to_cpu > 0 && 
				nextThread->previous_cpu == oldThread->cpu)
					break;
			prevThread = nextThread;
			nextThread = nextThread->queue_next;
		}
	} else {
		while (nextThread) {
			// select next thread from the run queue
			while (nextThread && nextThread->priority > B_IDLE_PRIORITY) {
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

				// never skip last non-idle normal thread
				if (nextThread->queue_next && nextThread->queue_next->priority == B_IDLE_PRIORITY)
					break;

				// skip normal threads sometimes (roughly 20%)
				if (_rand() > 0x1a00)
					break;

				// skip until next lower priority
				int32 priority = nextThread->priority;
				do {
					prevThread = nextThread;
					nextThread = nextThread->queue_next;
				} while (nextThread->queue_next != NULL
					&& priority == nextThread->queue_next->priority
					&& nextThread->queue_next->priority > B_IDLE_PRIORITY);
			}

			if (nextThread->cpu
				&& nextThread->cpu->cpu_num != oldThread->cpu->cpu_num) {
				panic("thread in run queue that's still running on another CPU!\n");
				// ToDo: remove this check completely when we're sure that this
				// cannot happen anymore.
				prevThread = nextThread;
				nextThread = nextThread->queue_next;
				continue;
			}

			break;
		}
	}

	if (!nextThread)
		panic("reschedule(): run queue is empty!\n");

	// extract selected thread from the run queue
	if (prevThread)
		prevThread->queue_next = nextThread->queue_next;
	else
		sRunQueue = nextThread->queue_next;

	T(ScheduleThread(nextThread, oldThread));

	nextThread->state = B_THREAD_RUNNING;
	nextThread->next_state = B_THREAD_READY;
	oldThread->was_yielded = false;

	// track kernel time (user time is tracked in thread_at_kernel_entry())
	bigtime_t now = system_time();
	oldThread->kernel_time += now - oldThread->last_time;
	nextThread->last_time = now;

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
		bigtime_t quantum = 3000;	// ToDo: calculate quantum!
		timer *quantumTimer = &oldThread->cpu->quantum_timer;

		if (!oldThread->cpu->preempted)
			cancel_timer(quantumTimer);

		oldThread->cpu->preempted = 0;
		add_timer(quantumTimer, &reschedule_event, quantum,
			B_ONE_SHOT_RELATIVE_TIMER | B_TIMER_ACQUIRE_THREAD_LOCK);

		// update the idle bit for this CPU in the CPU mask
		int32 cpuNum = smp_get_current_cpu();
		if (nextThread->priority == B_IDLE_PRIORITY)
			sIdleCPUs = SET_BIT(sIdleCPUs, cpuNum);
		else
			sIdleCPUs = CLEAR_BIT(sIdleCPUs, cpuNum);

		if (nextThread != oldThread)
			context_switch(oldThread, nextThread);
	}
}


/*!	This starts the scheduler. Must be run under the context of
	the initial idle thread.
*/
static void
simple_start(void)
{
	cpu_status state = disable_interrupts();
	GRAB_THREAD_LOCK();

	simple_reschedule();

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);
}


static scheduler_ops kSimpleOps = {
	simple_enqueue_in_run_queue,
	simple_reschedule,
	simple_set_thread_priority,
	simple_start
};


// #pragma mark -


void
scheduler_simple_init()
{
	gScheduler = &kSimpleOps;

	add_debugger_command_etc("run_queue", &dump_run_queue,
		"List threads in run queue", "\nLists threads in run queue", 0);
}
