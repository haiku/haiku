/* Policy info for timers */
/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <console.h>
#include <debug.h>
#include <thread.h>
#include <arch/int.h>
#include <smp.h>
#include <vm.h>
#include <int.h>
#include <timer.h>
#include <Errors.h>
#include <stage2.h>

#include <arch/cpu.h>
#include <arch/timer.h>
#include <arch/smp.h>

static struct timer_event * volatile events[SMP_MAX_CPUS] = { NULL, };
static spinlock_t timer_spinlock[SMP_MAX_CPUS] = { 0, };

int timer_init(kernel_args *ka)
{
	dprintf("init_timer: entry\n");

	return arch_init_timer(ka);
}

// NOTE: expects interrupts to be off
static void add_event_to_list(struct timer_event *event, struct timer_event * volatile *list)
{
	struct timer_event *next;
	struct timer_event *last = NULL;

	// stick it in the event list
	next = *list;
	while(next != NULL && next->sched_time < event->sched_time) {
		last = next;
		next = next->next;
	}

	if(last != NULL) {
		event->next = last->next;
		last->next = event;
	} else {
		event->next = next;
		*list = event;
	}
}

int timer_interrupt()
{
	bigtime_t curr_time = system_time();
	struct timer_event *event;
	spinlock_t *spinlock;
	int curr_cpu = smp_get_current_cpu();
	int rc = INT_NO_RESCHEDULE;

//	dprintf("timer_interrupt: time 0x%x 0x%x, cpu %d\n", system_time(), smp_get_current_cpu());

	spinlock = &timer_spinlock[curr_cpu];

	acquire_spinlock(spinlock);

restart_scan:
	event = events[curr_cpu];
	if(event != NULL && event->sched_time < curr_time) {
		// this event needs to happen
		int mode = event->mode;

		events[curr_cpu] = event->next;
		event->sched_time = 0;

		release_spinlock(spinlock);

		// call the callback
		// note: if the event is not periodic, it is ok
		// to delete the event structure inside the callback
		if(event->func != NULL) {
			if(event->func(event->data) == INT_RESCHEDULE)
				rc = INT_RESCHEDULE;
		}

		acquire_spinlock(spinlock);

		if(mode == TIMER_MODE_PERIODIC) {
			// we need to adjust it and add it back to the list
			event->sched_time = system_time() + event->periodic_time;
			if(event->sched_time == 0)
				event->sched_time = 1; // if we wrapped around and happen
				                       // to hit zero, set it to one, since
				                       // zero represents not scheduled
			add_event_to_list(event, &events[curr_cpu]);
		}

		goto restart_scan; // the list may have changed
	}

	// setup the next hardware timer
	if(events[curr_cpu] != NULL)
		arch_timer_set_hardware_timer(events[curr_cpu]->sched_time - system_time());

	release_spinlock(spinlock);

	return rc;
}

void timer_setup_timer(timer_callback func, void *data, struct timer_event *event)
{
	event->func = func;
	event->data = data;
	event->sched_time = 0;
}

int timer_set_event(bigtime_t relative_time, timer_mode mode, struct timer_event *event)
{
	int state;
	int curr_cpu;

	if(event == NULL)
		return ERR_INVALID_ARGS;

	if(relative_time < 0)
		relative_time = 0;

	if(event->sched_time != 0)
		panic("timer_set_event: event %p in list already!\n", event);

	event->sched_time = system_time() + relative_time;
	if(event->sched_time == 0)
		event->sched_time = 1; // if we wrapped around and happen
		                       // to hit zero, set it to one, since
		                       // zero represents not scheduled
	event->mode = mode;
	if(event->mode == TIMER_MODE_PERIODIC)
		event->periodic_time = relative_time;

	state = int_disable_interrupts();

	curr_cpu = smp_get_current_cpu();

	acquire_spinlock(&timer_spinlock[curr_cpu]);

	add_event_to_list(event, &events[curr_cpu]);

	// if we were stuck at the head of the list, set the hardware timer
	if(event == events[curr_cpu]) {
		arch_timer_set_hardware_timer(relative_time);
	}

	release_spinlock(&timer_spinlock[curr_cpu]);
	int_restore_interrupts(state);

	return 0;
}

/* this is a fast path to be called from reschedule and from timer_cancel_event */
/* must always be invoked with interrupts disabled */
int _local_timer_cancel_event(int curr_cpu, struct timer_event *event)
{
	struct timer_event *last = NULL;
	struct timer_event *e;
	bool foundit = false;

	acquire_spinlock(&timer_spinlock[curr_cpu]);
	e = events[curr_cpu];
	while(e != NULL) {
		if(e == event) {
			// we found it
			foundit = true;
			if(e == events[curr_cpu]) {
				events[curr_cpu] = e->next;
			} else {
				last->next = e->next;
			}
			e->next = NULL;
			// break out of the whole thing
			goto done;
		}
		last = e;
		e = e->next;
	}
	release_spinlock(&timer_spinlock[curr_cpu]);
done:

	if(events[curr_cpu] == NULL) {
		arch_timer_clear_hardware_timer();
	} else {
		arch_timer_set_hardware_timer(events[curr_cpu]->sched_time - system_time());
	}

	if(foundit) {
		release_spinlock(&timer_spinlock[curr_cpu]);
	}

	return (foundit ? 0 : ERR_GENERAL);
}

int local_timer_cancel_event(struct timer_event *event)
{
	return _local_timer_cancel_event(smp_get_current_cpu(), event);
}

int timer_cancel_event(struct timer_event *event)
{
	int state;
	struct timer_event *last = NULL;
	struct timer_event *e;
	bool foundit = false;
	int num_cpus = smp_get_num_cpus();
	int cpu= 0;
	int curr_cpu;

	if(event->sched_time == 0)
		return 0; // it's not scheduled

	state = int_disable_interrupts();
	curr_cpu = smp_get_current_cpu();

	// walk through all of the cpu's timer queues
	//
	// We start by peeking our own queue, aiming for
	// a cheap match. If this fails, we start harassing
	// other cpus.
	//
	if(_local_timer_cancel_event(curr_cpu, event) < 0) {
		for(cpu = 0; cpu < num_cpus; cpu++) {
			if(cpu== curr_cpu) continue;
			acquire_spinlock(&timer_spinlock[cpu]);
			e = events[cpu];
			while(e != NULL) {
				if(e == event) {
					// we found it
					foundit = true;
					if(e == events[cpu]) {
						events[cpu] = e->next;
					} else {
						last->next = e->next;
					}
					e->next = NULL;
					// break out of the whole thing
					goto done;
				}
				last = e;
				e = e->next;
			}
			release_spinlock(&timer_spinlock[cpu]);
		}
	}
done:

	if(foundit) {
		release_spinlock(&timer_spinlock[cpu]);
	}
	int_restore_interrupts(state);

	return (foundit ? 0 : ERR_GENERAL);
}

void spin(bigtime_t microseconds)
{
	bigtime_t time = system_time();

	while((system_time() - time) < microseconds)
		;
}

