/* scheduler.c
 *
 * The scheduler code
 *
 */

/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
 
#include <OS.h>
#include <kernel.h>
#include <thread.h>
#include <thread_types.h>
#include <timer.h>
#include <int.h>
#include <smp.h>
#include <cpu.h>
#include <khash.h>
#ifdef NEW_SCHEDULER
#include <Errors.h>
#include <kerrors.h>
#endif /* NEW_SCHEDULER */

static int _rand(void);

#ifdef NEW_SCHEDULER

// The run queue. Holds the threads ready to run ordered by priority.
static struct thread_queue run_q = { NULL, NULL };


void
thread_enqueue_run_q(struct thread *t)
{
	struct thread *curr, *prev;
	
	// these shouldn't exist
	if(t->priority > B_MAX_PRIORITY)
		t->priority = B_MAX_PRIORITY;
	if(t->priority < B_MIN_PRIORITY)
		t->priority = B_MIN_PRIORITY;
	
	if (run_q.head == NULL) {
		t->q_next = NULL;
		run_q.head = t;
	} else {
		for (curr = run_q.head, prev = NULL; curr && (curr->priority >= t->priority); curr = curr->q_next) {
			if (prev)
				prev = prev->q_next;
			else
				prev = run_q.head;
		}
		t->q_next = curr;
		if (prev)
			prev->q_next = t;
		else
			run_q.head = t;
	}
}


status_t
thread_set_priority(thread_id id, int32 priority)
{
	struct thread *t, *curr, *prev;
	int retval;

	// make sure the passed in priority is within bounds
	if (priority > B_MAX_PRIORITY)
		priority = B_MAX_PRIORITY;
	if (priority < B_MIN_PRIORITY)
		priority = B_MIN_PRIORITY;

	t = thread_get_current_thread();
	if (t->id == id) {
		// it's ourself, so we know we aren't in the run queue, and we can manipulate
		// our structure directly
		retval = t->priority;
		t->priority = priority;
	} else {
		int state = disable_interrupts();
		GRAB_THREAD_LOCK();

		t = thread_get_thread_struct_locked(id);
		if (t) {
			retval = t->priority;
			if ((t->state == B_THREAD_READY) && (t->priority != priority)) {
				// this thread is in the ready queue right now, so it needs to be reinserted
				for (curr = run_q.head, prev = NULL; curr && (curr->id != t->id); curr = curr->q_next) {
					if (prev)
						prev = prev->q_next;
					else
						prev = run_q.head;
				}
				if (prev)
					prev->q_next = curr->q_next;
				else
					run_q.head = curr->q_next;
				t->priority = priority;
				thread_enqueue_run_q(t);
			} else
				t->priority = priority;
		} else
			retval = B_BAD_THREAD_ID;

		RELEASE_THREAD_LOCK();
		restore_interrupts(state);
	}

	return retval;
}


#endif /* NEW_SCHEDULER */
static int
_rand(void)
{
	static int next = 0;

	if (next == 0)
		next = system_time();

	next = next * 1103515245 + 12345;
	return((next >> 16) & 0x7FFF);
}


// this starts the scheduler. Must be run under the context of
// the initial idle thread.
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

	resched();

	RELEASE_THREAD_LOCK();
	restore_interrupts(state);
}


static void
context_switch(struct thread *t_from, struct thread *t_to)
{
	bigtime_t now;

	// track kernel time
	now = system_time();
	t_from->kernel_time += now - t_from->last_time;
	t_to->last_time = now;

	t_to->cpu = t_from->cpu;
	arch_thread_set_current_thread(t_to);
	t_from->cpu = NULL;
	arch_thread_context_switch(t_from, t_to);
}


static int32
reschedule_event(timer *unused)
{
	// this function is called as a result of the timer event set by the scheduler
	// returning this causes a reschedule on the timer event
	thread_get_current_thread()->cpu->info.preempted= 1;
	return B_INVOKE_SCHEDULER;
}


// runs the scheduler.
// NOTE: expects thread_spinlock to be held
void
resched(void)
{
#ifndef NEW_SCHEDULER
	struct thread *next_thread = NULL;
	int last_thread_pri = -1;
#else /* NEW_SCHEDULER */
	struct thread *next_thread, *prev_thread = NULL;
#endif /* NEW_SCHEDULER */
	struct thread *old_thread = thread_get_current_thread();
#ifndef NEW_SCHEDULER
	int i;
#endif /* not NEW_SCHEDULER */
	bigtime_t quantum;
	timer *quantum_timer;

#ifndef NEW_SCHEDULER
//	dprintf("top of thread_resched: cpu %d, cur_thread = 0x%x\n", smp_get_current_cpu(), thread_get_current_thread());
#else /* NEW_SCHEDULER */
//	dprintf("resched(): cpu %d, cur_thread = 0x%x\n", smp_get_current_cpu(), thread_get_current_thread());
#endif /* NEW_SCHEDULER */

	switch(old_thread->next_state) {
		case B_THREAD_RUNNING:
		case B_THREAD_READY:
//			dprintf("enqueueing thread 0x%x into run q. pri = %d\n", old_thread, old_thread->priority);
			thread_enqueue_run_q(old_thread);
			break;
		case B_THREAD_SUSPENDED:
#ifndef NEW_SCHEDULER
			dprintf("suspending thread 0x%lx\n", old_thread->id);
#else /* NEW_SCHEDULER */
			dprintf("resched(): suspending thread 0x%x\n", old_thread->id);
#endif /* NEW_SCHEDULER */
			break;
		case THREAD_STATE_FREE_ON_RESCHED:
			// This will hopefully be eliminated once the slab
			// allocator is done
			thread_enqueue(old_thread, &dead_q);
			break;
		default:
//			dprintf("not enqueueing thread 0x%x into run q. next_state = %d\n", old_thread, old_thread->next_state);
			;
	}
	old_thread->state = old_thread->next_state;

#ifndef NEW_SCHEDULER
	// search the real-time queue
	for(i = B_MAX_PRIORITY; i >= B_FIRST_REAL_TIME_PRIORITY; i-=2) {
		next_thread = thread_dequeue_run_q(i);
		if(next_thread)
			goto found_thread;
	}

	// search the regular queue
	for(i = B_FIRST_REAL_TIME_PRIORITY - 1; i >= B_LOWEST_ACTIVE_PRIORITY; i-=2) {
		next_thread = thread_lookat_run_q(i);
		if(next_thread != NULL) {
			// skip it sometimes
			if(_rand() > 0x3000) {
				next_thread = thread_dequeue_run_q(i);
				goto found_thread;
			}
			last_thread_pri = i;
			next_thread = NULL;
		}
	}
	if(next_thread == NULL) {
		if(last_thread_pri != -1) {
			next_thread = thread_dequeue_run_q(last_thread_pri);
			if(next_thread == NULL)
				panic("next_thread == NULL! last_thread_pri = %d\n", last_thread_pri);
		} else {
			next_thread = thread_dequeue_run_q(B_IDLE_PRIORITY);
			if(next_thread == NULL)
				panic("next_thread == NULL! no idle priorities!\n");
		}
#else /* NEW_SCHEDULER */
	// select next thread from the run queue
	next_thread = run_q.head;
	while ((next_thread) && (next_thread->priority > B_IDLE_PRIORITY)) {
		// always extract real time threads
		if (next_thread->priority >= B_FIRST_REAL_TIME_PRIORITY)
			break;
		// never skip last non-idle normal thread
		if (next_thread->q_next && (next_thread->q_next->priority == B_IDLE_PRIORITY))
			break;
		// skip normal threads sometimes
		if (_rand() > 0x3000)
			break;
		prev_thread = next_thread;
		next_thread = next_thread->q_next;
#endif /* NEW_SCHEDULER */
	}
#ifndef NEW_SCHEDULER

found_thread:
#else /* NEW_SCHEDULER */
	if (!next_thread)
		panic("resched(): run queue is empty!\n");
		
	// extract selected thread from the run queue
	if (prev_thread)
		prev_thread->q_next = next_thread->q_next;
	else
		run_q.head = next_thread->q_next;
	
#endif /* NEW_SCHEDULER */
	next_thread->state = B_THREAD_RUNNING;
	next_thread->next_state = B_THREAD_READY;
	
	if ((next_thread != old_thread) || (old_thread->cpu->info.preempted)) {
		// XXX calculate quantum
		quantum = 3000;
		quantum_timer = &old_thread->cpu->info.quantum_timer;
		if (!old_thread->cpu->info.preempted)
			_local_timer_cancel_event(old_thread->cpu->info.cpu_num, quantum_timer);
		old_thread->cpu->info.preempted = 0;
		add_timer(quantum_timer, &reschedule_event, quantum, B_ONE_SHOT_RELATIVE_TIMER);
		if (next_thread != old_thread)
			context_switch(old_thread, next_thread);
	}
#ifndef NEW_SCHEDULER

#if 0
	// XXX should only reset the quantum timer if we are switching to a new thread,
	// or we got here as a result of a quantum expire.

	// XXX calculate quantum
	quantum = 10000;

	// get the quantum timer for this cpu
	quantum_timer = &old_thread->cpu->info.quantum_timer;
	if(!old_thread->cpu->info.preempted) {
		_local_timer_cancel_event(old_thread->cpu->info.cpu_num, quantum_timer);
	}
	old_thread->cpu->info.preempted = 0;
	add_timer(quantum_timer, &reschedule_event, quantum, B_ONE_SHOT_RELATIVE_TIMER);

	if(next_thread != old_thread) {
//		dprintf("thread_resched: cpu %d switching from thread %d to %d\n",
//			smp_get_current_cpu(), old_thread->id, next_thread->id);
		context_switch(old_thread, next_thread);
	}
#endif
#endif /* not NEW_SCHEDULER */
}

