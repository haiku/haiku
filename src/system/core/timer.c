/* Policy info for timers */

/*
** Copyright 2002-2004, The OpenBeOS Team. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
**
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <OS.h>

#include <timer.h>
#include <arch/timer.h>
#include <smp.h>
#include <boot/kernel_args.h>


static timer * volatile sEvents[B_MAX_CPU_COUNT] = { NULL, };
static spinlock sTimerSpinlock[B_MAX_CPU_COUNT] = { 0, };


//#define TRACE_TIMER
#ifdef TRACE_TIMER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


status_t
timer_init(kernel_args *args)
{
	TRACE(("timer_init: entry\n"));

	return arch_init_timer(args);
}


/** NOTE: expects interrupts to be off */

static void
add_event_to_list(timer *event, timer * volatile *list)
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
	} else {
		(timer *)event->entry.next = next;
		*list = event;
	}
}


int32
timer_interrupt()
{
	timer *event;
	spinlock *spinlock;
	int currentCPU = smp_get_current_cpu();
	int32 rc = B_HANDLED_INTERRUPT;

	TRACE(("timer_interrupt: time 0x%x 0x%x, cpu %d\n", system_time(), smp_get_current_cpu()));

	spinlock = &sTimerSpinlock[currentCPU];

	acquire_spinlock(spinlock);

restart_scan:
	event = sEvents[currentCPU];
	if (event != NULL && ((bigtime_t)event->entry.key < system_time())) {
		// this event needs to happen
		int mode = event->flags;

		sEvents[currentCPU] = (timer *)event->entry.next;
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
			bigtime_t scheduleTime = system_time() + event->period;
			if (scheduleTime == 0) {
				// if we wrapped around and happen to hit zero, set
				// it to one, since zero represents not scheduled
				scheduleTime = 1;
			}
			event->entry.key = (int64)scheduleTime;
			add_event_to_list(event, &sEvents[currentCPU]);
		}

		goto restart_scan; // the list may have changed
	}

	// setup the next hardware timer
	if (sEvents[currentCPU] != NULL)
		arch_timer_set_hardware_timer((bigtime_t)sEvents[currentCPU]->entry.key - system_time());

	release_spinlock(spinlock);

	return rc;
}


status_t
add_timer(timer *event, timer_hook hook, bigtime_t period, int32 flags)
{
	bigtime_t scheduleTime;
	bigtime_t currentTime = system_time();
	cpu_status state;
	int currentCPU;
	
	if (event == NULL || hook == NULL || period < 0)
		return B_BAD_VALUE;

	scheduleTime = period;
	if (flags != B_ONE_SHOT_ABSOLUTE_TIMER)
		scheduleTime += currentTime;
	if (scheduleTime == 0)
		scheduleTime = 1;

	event->entry.key = (int64)scheduleTime;
	event->period = period;
	event->hook = hook;
	event->flags = flags;

	state = disable_interrupts();
	currentCPU = smp_get_current_cpu();
	acquire_spinlock(&sTimerSpinlock[currentCPU]);

	add_event_to_list(event, &sEvents[currentCPU]);
	event->cpu = currentCPU;

	// if we were stuck at the head of the list, set the hardware timer
	if (event == sEvents[currentCPU])
		arch_timer_set_hardware_timer(scheduleTime - currentTime);

	release_spinlock(&sTimerSpinlock[currentCPU]);
	restore_interrupts(state);

	return B_OK;
}


/** This is a fast path to be called from reschedule() and from
 *	cancel_timer().
 *	Must always be invoked with interrupts disabled.
 */

status_t
_local_timer_cancel_event(int cpu, timer *event)
{
	timer *last = NULL;
	timer *current;

	acquire_spinlock(&sTimerSpinlock[cpu]);
	current = sEvents[cpu];
	while (current != NULL) {
		if (current == event) {
			// we found it
			if (current == sEvents[cpu])
				sEvents[cpu] = (timer *)current->entry.next;
			else
				(timer *)last->entry.next = (timer *)current->entry.next;
			current->entry.next = NULL;
			// break out of the whole thing
			break;
		}
		last = current;
		current = (timer *)current->entry.next;
	}

	if (sEvents[cpu] == NULL)
		arch_timer_clear_hardware_timer();
	else
		arch_timer_set_hardware_timer((bigtime_t)sEvents[cpu]->entry.key - system_time());

	release_spinlock(&sTimerSpinlock[cpu]);

	return current == event ? B_OK : B_ERROR;
}


bool
cancel_timer(timer *event)
{
	int currentCPU = smp_get_current_cpu();
	cpu_status state;

	state = disable_interrupts();

	// walk through all of the cpu's timer queues
	//
	// We start by peeking our own queue, aiming for
	// a cheap match. If this fails, we start harassing
	// other cpus.

	if (_local_timer_cancel_event(currentCPU, event) < 0) {
		int numCPUs = smp_get_num_cpus();
		int cpu = 0;
		timer *last = NULL;
		timer *current;

		for (cpu = 0; cpu < numCPUs; cpu++) {
			if (cpu == currentCPU)
				continue;

			acquire_spinlock(&sTimerSpinlock[cpu]);
			current = sEvents[cpu];
			while (current != NULL) {
				if (current == event) {
					// we found it
					if (current == sEvents[cpu])
						sEvents[cpu] = (timer *)current->entry.next;
					else
						(timer *)last->entry.next = (timer *)current->entry.next;
					current->entry.next = NULL;

					// break out of the whole thing

					release_spinlock(&sTimerSpinlock[cpu]);
					restore_interrupts(state);
					return (bigtime_t)event->entry.key < system_time();
				}
				last = current;
				current = (timer *)current->entry.next;
			}
			release_spinlock(&sTimerSpinlock[cpu]);
		}
	}

	restore_interrupts(state);
	return false;
}


void
spin(bigtime_t microseconds)
{
	bigtime_t time = system_time();

	while((system_time() - time) < microseconds)
		;
}

