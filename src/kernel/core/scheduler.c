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

static int _rand(void);

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


static int
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
	struct thread *next_thread = NULL;
	int last_thread_pri = -1;
	struct thread *old_thread = thread_get_current_thread();
	int i;
	bigtime_t quantum;
	timer *quantum_timer;

//	dprintf("top of thread_resched: cpu %d, cur_thread = 0x%x\n", smp_get_current_cpu(), thread_get_current_thread());

	switch(old_thread->next_state) {
		case THREAD_STATE_RUNNING:
		case THREAD_STATE_READY:
//			dprintf("enqueueing thread 0x%x into run q. pri = %d\n", old_thread, old_thread->priority);
			thread_enqueue_run_q(old_thread);
			break;
		case THREAD_STATE_SUSPENDED:
			dprintf("suspending thread 0x%x\n", old_thread->id);
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

	// search the real-time queue
	for(i = THREAD_MAX_RT_PRIORITY; i >= THREAD_MIN_RT_PRIORITY; i--) {
		next_thread = thread_dequeue_run_q(i);
		if(next_thread)
			goto found_thread;
	}

	// search the regular queue
	for(i = THREAD_MAX_PRIORITY; i > THREAD_IDLE_PRIORITY; i--) {
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
			next_thread = thread_dequeue_run_q(THREAD_IDLE_PRIORITY);
			if(next_thread == NULL)
				panic("next_thread == NULL! no idle priorities!\n");
		}
	}

found_thread:
	next_thread->state = THREAD_STATE_RUNNING;
	next_thread->next_state = THREAD_STATE_READY;
	
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
}

