/* The thread scheduler */

/*
** Copyright 2002-2004, The OpenBeOS Team. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
 
#include <OS.h>

#include <thread.h>
#include <timer.h>
#include <int.h>
#include <smp.h>
#include <cpu.h>
#include <khash.h>


//#define TRACE_SCHEDULER
#ifdef TRACE_SCHEDULER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


// prototypes
static int dump_run_queue(int argc, char **argv);
static int _rand(void);

// The run queue. Holds the threads ready to run ordered by priority.
static struct thread_queue sRunQueue = {NULL, NULL};


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

	thread = sRunQueue.head;
	if (!thread)
		dprintf("Run queue is empty!\n");
	else {
		while (thread) {
			dprintf("Thread id: %ld - priority: %d\n", thread->id, thread->priority);
			thread = thread->queue_next;
		}
	}

	return 0;
}


/** Enqueues the thread into the run queue.
 *	Note: THREAD_LOCK must be held when entering this function
 */

void
scheduler_enqueue_in_run_queue(struct thread *thread)
{
	struct thread *curr, *prev;

	// these shouldn't exist
	if (thread->priority > B_MAX_PRIORITY)
		thread->priority = B_MAX_PRIORITY;
	if (thread->priority < B_MIN_PRIORITY)
		thread->priority = B_MIN_PRIORITY;

	for (curr = sRunQueue.head, prev = NULL; curr && (curr->priority >= thread->priority); curr = curr->queue_next) {
		if (prev)
			prev = prev->queue_next;
		else
			prev = sRunQueue.head;
	}
	thread->queue_next = curr;
	if (prev)
		prev->queue_next = thread;
	else
		sRunQueue.head = thread;
}


/** Removes a thread from the run queue.
 *	Note: THREAD_LOCK must be held when entering this function
 */

void
scheduler_remove_from_run_queue(struct thread *thread)
{
	struct thread *item, *prev;

	// find thread in run queue
	for (item = sRunQueue.head, prev = NULL; item && item != thread; item = item->queue_next) {
		if (prev)
			prev = prev->queue_next;
		else
			prev = sRunQueue.head;
	}

	ASSERT(item == thread);

	if (prev)
		prev->queue_next = item->queue_next;
	else
		sRunQueue.head = item->queue_next;
}


static void
context_switch(struct thread *fromThread, struct thread *toThread)
{
	bigtime_t now;

	// track kernel & user time
	now = system_time();
	if (fromThread->last_time_type == KERNEL_TIME)
		fromThread->kernel_time += now - fromThread->last_time;
	else
		fromThread->user_time += now - fromThread->last_time;
	toThread->last_time = now;

	toThread->cpu = fromThread->cpu;
	fromThread->cpu = NULL;

	arch_thread_set_current_thread(toThread);
	arch_thread_context_switch(fromThread, toThread);
}


static int32
reschedule_event(timer *unused)
{
	// this function is called as a result of the timer event set by the scheduler
	// returning this causes a reschedule on the timer event
	thread_get_current_thread()->cpu->info.preempted = 1;
	return B_INVOKE_SCHEDULER;
}


/** Runs the scheduler.
 *	NOTE: expects thread_spinlock to be held
 */

void
scheduler_reschedule(void)
{
	struct thread *oldThread = thread_get_current_thread();
	struct thread *nextThread, *prevThread;

	TRACE(("reschedule(): cpu %d, cur_thread = 0x%lx\n", smp_get_current_cpu(), thread_get_current_thread()->id));

	switch (oldThread->next_state) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
			TRACE(("enqueueing thread 0x%lx into run q. pri = %d\n", oldThread->id, oldThread->priority));
			scheduler_enqueue_in_run_queue(oldThread);
			break;
		case B_THREAD_SUSPENDED:
			TRACE(("reschedule(): suspending thread 0x%lx\n", oldThread->id));
			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			// This will hopefully be eliminated once the slab
			// allocator is done
			thread_enqueue(oldThread, &dead_q);
			break;
		default:
			TRACE(("not enqueueing thread 0x%lx into run q. next_state = %d\n", oldThread->id, oldThread->next_state));
			break;
	}
	oldThread->state = oldThread->next_state;

	// select next thread from the run queue
	nextThread = sRunQueue.head;
	prevThread = NULL;
	while (nextThread && (nextThread->priority > B_IDLE_PRIORITY)) {
		// always extract real time threads
		if (nextThread->priority >= B_FIRST_REAL_TIME_PRIORITY)
			break;

		// never skip last non-idle normal thread
		if (nextThread->queue_next && (nextThread->queue_next->priority == B_IDLE_PRIORITY))
			break;

		// skip normal threads sometimes
		if (_rand() > 0x3000)
			break;

		prevThread = nextThread;
		nextThread = nextThread->queue_next;
	}

	if (!nextThread)
		panic("reschedule(): run queue is empty!\n");

	// extract selected thread from the run queue
	if (prevThread)
		prevThread->queue_next = nextThread->queue_next;
	else
		sRunQueue.head = nextThread->queue_next;

	nextThread->state = B_THREAD_RUNNING;
	nextThread->next_state = B_THREAD_READY;

	if (nextThread != oldThread || oldThread->cpu->info.preempted) {
		bigtime_t quantum = 3000;	// ToDo: calculate quantum!
		timer *quantum_timer= &oldThread->cpu->info.quantum_timer;

		if (!oldThread->cpu->info.preempted)
			_local_timer_cancel_event(oldThread->cpu->info.cpu_num, quantum_timer);

		oldThread->cpu->info.preempted = 0;
		add_timer(quantum_timer, &reschedule_event, quantum, B_ONE_SHOT_RELATIVE_TIMER);

		if (nextThread != oldThread)
			context_switch(oldThread, nextThread);
	}
}


/** This starts the scheduler. Must be run under the context of
 *	the initial idle thread.
 */

void
start_scheduler(void)
{
	int state;

	// XXX may not be the best place for this
	// invalidate all of the other processors' TLB caches
	state = disable_interrupts();
	arch_cpu_global_TLB_invalidate();
	smp_send_broadcast_ici(SMP_MSG_GLOBAL_INVL_PAGE, 0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
	restore_interrupts(state);

	// start the other processors
	smp_send_broadcast_ici(SMP_MSG_RESCHEDULE, 0, 0, 0, NULL, SMP_MSG_FLAG_ASYNC);

	state = disable_interrupts();
	GRAB_THREAD_LOCK();

	scheduler_reschedule();

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);

	add_debugger_command("run_queue", &dump_run_queue, "list threads in run queue");
}


