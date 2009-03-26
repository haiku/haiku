/*
 * Copyright 2009, Rene Gollent, rene@gollent.com.
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

// The run queues. Holds the threads ready to run ordered by priority.
// One queue per schedulable target (CPU, core, etc.).
static struct thread* sRunQueue[B_MAX_CPU_COUNT];
static struct thread* sIdleThreads;
static cpu_mask_t sIdleCPUs = 0;

struct affine_scheduler_data : scheduler_thread_data {
	affine_scheduler_data(void) 
	{
		init();
	}
	
	void init() 
	{
		memset(fLastThreadQuantums, 0, sizeof(fLastThreadQuantums));
		fLastQuantumSlot = 0;
	}
	
	int32 fLastThreadQuantums[5];
	int16 fLastQuantumSlot;
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
	struct thread *thread = NULL;

	for (int32 i  = 0; i < smp_get_num_cpus(); i++) {
		thread = sRunQueue[i];
		if (!thread)
			kprintf("Run queue for cpu %ld is empty!\n", i);
		else {
			kprintf("thread    id      priority name\n");
			while (thread) {
				kprintf("%p  %-7ld %-8ld %s\n", thread, thread->id,
					thread->priority, thread->name);
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
		if (targetCPU < 0 
			|| gCPU[i].active_time < gCPU[targetCPU].active_time)
			targetCPU = i;
	}
	
	return targetCPU;
}


static inline int32
affine_get_next_idle_cpu(void)
{
	for (int32 i = 0; i < smp_get_num_cpus(); i++) {
		if (gCPU[i].disabled)
			continue;
		if (sIdleCPUs & (1 << i))
			return i;
	}

	return -1;
}

/*!	Enqueues the thread into the run queue.
	Note: thread lock must be held when entering this function
*/
static void
affine_enqueue_in_run_queue(struct thread *thread)
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
		struct thread *curr, *prev;
		for (curr = sRunQueue[targetCPU], prev = NULL; curr
			&& curr->priority >= thread->next_priority;
			curr = curr->queue_next) {
			if (prev)
				prev = prev->queue_next;
			else
				prev = sRunQueue[targetCPU];
		}

		T(EnqueueThread(thread, prev, curr));

		thread->queue_next = curr;
		if (prev)
			prev->queue_next = thread;
		else
			sRunQueue[targetCPU] = thread;
	}
	
	thread->next_priority = thread->priority;

	if (thread->priority != B_IDLE_PRIORITY && targetCPU != smp_get_current_cpu()) {
		int32 idleCPU = targetCPU;
		if ((sIdleCPUs & (1 << targetCPU)) == 0) {
			idleCPU = affine_get_next_idle_cpu();
			// no idle CPUs are available 
			// to try and grab this task
			if (idleCPU < 0)
				return;
		}
		sIdleCPUs &= ~(1 << idleCPU);
		smp_send_ici(idleCPU, SMP_MSG_RESCHEDULE_IF_IDLE, 0, 0,
			0, NULL, SMP_MSG_FLAG_ASYNC);
	}
}

static inline struct thread *
dequeue_from_run_queue(struct thread *prevThread, int32 currentCPU)
{
	struct thread *resultThread = NULL;
	if (prevThread != NULL) {
		resultThread = prevThread->queue_next;
		prevThread->queue_next = resultThread->queue_next;
	} else {
		resultThread = sRunQueue[currentCPU];
		sRunQueue[currentCPU] = resultThread->queue_next;
	}
	
	return resultThread;
}

/*!	Looks for a possible thread to grab/run from another CPU.
	Note: thread lock must be held when entering this function
*/
static struct thread *steal_thread_from_other_cpus(int32 currentCPU)
{
	int32 targetCPU = -1;
	struct thread* nextThread = NULL;
	struct thread* prevThread = NULL;

	// look through the active CPUs - find the one
	// that has a) threads available to steal, and
	// b) out of those, the one that's the most CPU-bound
	for (int32 i = 0; i < smp_get_num_cpus(); i++) {
		// skip CPUs that have either no or only one thread
		if (sRunQueue[i] == NULL || sRunQueue[i]->queue_next == NULL)
			continue;
		if (i == currentCPU)
			continue;
		// out of the CPUs with threads available to steal,
		// pick whichever one is generally the most CPU bound.
		if (targetCPU < 0)
			targetCPU = i;
		else if (gCPU[i].active_time > gCPU[targetCPU].active_time)
			targetCPU = i;
	}

	if (targetCPU >= 0) {
		nextThread = sRunQueue[targetCPU];
		do {
			// grab the highest priority non-pinned thread
			// out of this CPU's queue, if any.
			if (nextThread->pinned_to_cpu > 0) {
				prevThread = nextThread;
				nextThread = prevThread->queue_next;
			} else 
				break;
		} while (nextThread->queue_next != NULL);
		
		// we reached the end of the queue without finding an
		// eligible thread.
		if (nextThread->pinned_to_cpu > 0)
			nextThread = NULL;
		
		// dequeue the thread we're going to steal	
		if (nextThread != NULL) 
			dequeue_from_run_queue(prevThread, targetCPU);
	}

	return nextThread;
}


/*!	Sets the priority of a thread.
	Note: thread lock must be held when entering this function
*/
static void
affine_set_thread_priority(struct thread *thread, int32 priority)
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

	// search run queues for the thread
	// TODO: keep track of the queue a thread is in (perhaps in a
	// data pointer on the thread struct) so we only have to walk 
	// that exact queue to find it.
	struct thread *item = NULL, *prev = NULL;
	for (int32 i = 0; i < smp_get_num_cpus(); i++) {
		for (item = sRunQueue[i], prev = NULL; item && item != thread;
				item = item->queue_next) {
			if (prev)
				prev = prev->queue_next;
			else
				prev = sRunQueue[i];
		}
		if (item) {
			targetCPU = i;
			break;
		}
	}

	ASSERT(item == thread);

	// remove the thread
	thread = dequeue_from_run_queue(prev, targetCPU);

	// set priority and re-insert
	thread->priority = priority;
	affine_enqueue_in_run_queue(thread);
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
affine_reschedule(void)
{
	int32 currentCPU = smp_get_current_cpu();

	struct thread *oldThread = thread_get_current_thread();
	struct thread *nextThread, *prevThread;

	TRACE(("reschedule(): cpu %ld, cur_thread = %ld\n", currentCPU, oldThread->id));

	oldThread->cpu->invoke_scheduler = false;

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
		TRACE(("Dequeueing next thread from CPU %ld\n", currentCPU));
		// select next thread from the run queue
		while (nextThread->queue_next) {
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
				&& priority == nextThread->queue_next->priority);
		} 
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
		if (nextThread->priority == B_IDLE_PRIORITY)
			sIdleCPUs = SET_BIT(sIdleCPUs, currentCPU);
		else
			sIdleCPUs = CLEAR_BIT(sIdleCPUs, currentCPU);

		if (nextThread != oldThread)
			context_switch(oldThread, nextThread);
	}
}


static void
affine_on_thread_create(struct thread* thread)
{
	thread->scheduler_data = new(std::nothrow) affine_scheduler_data();
	if (thread->scheduler_data == NULL)
		panic("affine_scheduler: Unable to allocate scheduling data structure for thread %ld\n", thread->id);
}


static void
affine_on_thread_init(struct thread* thread)
{
	((affine_scheduler_data *)(thread->scheduler_data))->init();
}


static void
affine_on_thread_destroy(struct thread* thread)
{
	delete thread->scheduler_data;
}


/*!	This starts the scheduler. Must be run under the context of
	the initial idle thread.
*/
static void
affine_start(void)
{
	cpu_status state = disable_interrupts();
	GRAB_THREAD_LOCK();

	affine_reschedule();

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);
}


static scheduler_ops kAffineOps = {
	affine_enqueue_in_run_queue,
	affine_reschedule,
	affine_set_thread_priority,
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

	add_debugger_command_etc("run_queue", &dump_run_queue,
		"List threads in run queue", "\nLists threads in run queue", 0);
}
