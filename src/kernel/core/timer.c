/* Policy info for timers */
/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <OS.h>

#include <kernel.h>
#include <console.h>
#include <debug.h>
#include <thread.h>
#include <arch/int.h>
#include <smp.h>
#include <vm.h>
#include <int.h>
#include <timer.h>
#include <boot/kernel_args.h>

#include <arch/cpu.h>
#include <arch/timer.h>
#include <arch/smp.h>

static timer * volatile events[SMP_MAX_CPUS] = { NULL, };
static spinlock timer_spinlock[SMP_MAX_CPUS] = { 0, };


int timer_init(kernel_args *ka)
{
	dprintf("init_timer: entry\n");

	return arch_init_timer(ka);
}

// NOTE: expects interrupts to be off
static void add_event_to_list(timer *event, timer * volatile *list)
{
	timer *next;
	timer *last = NULL;

	// stick it in the event list
	for (next = *list; next; last = next, next = (timer *)next->entry.next) {
		if ((bigtime_t)next->entry.key >= (bigtime_t)event->entry.key)
			break;
	}

	if (last != NULL) {
		(timer *)event->entry.next = (timer *)last->entry.next;
		(timer *)last->entry.next = event;
	}
	else {
		(timer *)event->entry.next = next;
		*list = event;
	}
}

int timer_interrupt()
{
	bigtime_t sched_time;
	timer *event;
	spinlock *spinlock;
	int curr_cpu = smp_get_current_cpu();
	int rc = B_HANDLED_INTERRUPT;

//	dprintf("timer_interrupt: time 0x%x 0x%x, cpu %d\n", system_time(), smp_get_current_cpu());

	spinlock = &timer_spinlock[curr_cpu];

	acquire_spinlock(spinlock);

restart_scan:
	event = events[curr_cpu];
	if ((event) && ((bigtime_t)event->entry.key < system_time())) {
		// this event needs to happen
		int mode = event->flags;

		events[curr_cpu] = (timer *)event->entry.next;
		event->entry.key = 0;

		release_spinlock(spinlock);

		// call the callback
		// note: if the event is not periodic, it is ok
		// to delete the event structure inside the callback
		if (event->hook) {
			rc = event->hook(event);
//			if (event->func(event->data) == INT_RESCHEDULE)
//				rc = INT_RESCHEDULE;
		}

		acquire_spinlock(spinlock);

		if (mode == B_PERIODIC_TIMER) {
			// we need to adjust it and add it back to the list
			sched_time = system_time() + event->period;
			if (sched_time == 0)
				sched_time = 1; // if we wrapped around and happen
				                // to hit zero, set it to one, since
				                // zero represents not scheduled
			event->entry.key = (int64)sched_time;
			add_event_to_list(event, &events[curr_cpu]);
		}

		goto restart_scan; // the list may have changed
	}

	// setup the next hardware timer
	if (events[curr_cpu] != NULL)
		arch_timer_set_hardware_timer((bigtime_t)events[curr_cpu]->entry.key - system_time());

	release_spinlock(spinlock);

	return rc;
}

status_t add_timer(timer *t, timer_hook hook, bigtime_t period, int32 flags)
{
	bigtime_t sched_time;
	bigtime_t curr_time = system_time();
	int state;
	int curr_cpu;
	
	if ((!t) || (!hook) || (period < 0))
		return B_BAD_VALUE;
		
	sched_time = period;
	if (flags != B_ONE_SHOT_ABSOLUTE_TIMER)
		sched_time += curr_time;
	if (sched_time == 0)
		sched_time = 1;
	
	t->entry.key = (int64)sched_time;
	t->period = period;
	t->hook = hook;
	t->flags = flags;

	state = disable_interrupts();
	curr_cpu = smp_get_current_cpu();
	acquire_spinlock(&timer_spinlock[curr_cpu]);

	add_event_to_list(t, &events[curr_cpu]);
	t->cpu = curr_cpu;

	// if we were stuck at the head of the list, set the hardware timer
	if (t == events[curr_cpu])
		arch_timer_set_hardware_timer(sched_time - curr_time);

	release_spinlock(&timer_spinlock[curr_cpu]);
	restore_interrupts(state);

	return B_OK;
}

/* this is a fast path to be called from reschedule and from timer_cancel_event */
/* must always be invoked with interrupts disabled */
int _local_timer_cancel_event(int curr_cpu, timer *event)
{
	timer *last = NULL;
	timer *e;

	acquire_spinlock(&timer_spinlock[curr_cpu]);
	e = events[curr_cpu];
	while (e != NULL) {
		if (e == event) {
			// we found it
			if (e == events[curr_cpu])
				events[curr_cpu] = (timer *)e->entry.next;
			else
				(timer *)last->entry.next = (timer *)e->entry.next;
			e->entry.next = NULL;
			// break out of the whole thing
			break;
		}
		last = e;
		e = (timer *)e->entry.next;
	}

	if (events[curr_cpu] == NULL)
		arch_timer_clear_hardware_timer();
	else
		arch_timer_set_hardware_timer((bigtime_t)events[curr_cpu]->entry.key - system_time());
	
	release_spinlock(&timer_spinlock[curr_cpu]);

	return (e == event ? 0 : B_ERROR);
}

int local_timer_cancel_event(timer *event)
{
	return _local_timer_cancel_event(smp_get_current_cpu(), event);
}

bool cancel_timer(timer *event)
{
	int state;
	timer *last = NULL;
	timer *e;
	bool foundit = false;
	int num_cpus = smp_get_num_cpus();
	int cpu= 0;
	int curr_cpu;

//	if (event->sched_time == 0)
//		return 0; // it's not scheduled

	state = disable_interrupts();
	curr_cpu = smp_get_current_cpu();

	// walk through all of the cpu's timer queues
	//
	// We start by peeking our own queue, aiming for
	// a cheap match. If this fails, we start harassing
	// other cpus.
	//
	if (_local_timer_cancel_event(curr_cpu, event) < 0) {
		for (cpu = 0; cpu < num_cpus; cpu++) {
			if (cpu== curr_cpu) continue;
			acquire_spinlock(&timer_spinlock[cpu]);
			e = events[cpu];
			while (e != NULL) {
				if (e == event) {
					// we found it
					foundit = true;
					if(e == events[cpu])
						events[cpu] = (timer *)e->entry.next;
					else
						(timer *)last->entry.next = (timer *)e->entry.next;
					e->entry.next = NULL;
					// break out of the whole thing
					goto done;
				}
				last = e;
				e = (timer *)e->entry.next;
			}
			release_spinlock(&timer_spinlock[cpu]);
		}
	}
done:

	if (foundit)
		release_spinlock(&timer_spinlock[cpu]);
	restore_interrupts(state);

	if (foundit && ((bigtime_t)event->entry.key < system_time()))
		return true;
	return false;
}

void spin(bigtime_t microseconds)
{
	bigtime_t time = system_time();

	while((system_time() - time) < microseconds)
		;
}

