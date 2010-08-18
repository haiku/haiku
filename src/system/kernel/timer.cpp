/*
 * Copyright 2002-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


/*! Policy info for timers */


#include <timer.h>

#include <OS.h>

#include <arch/timer.h>
#include <boot/kernel_args.h>
#include <cpu.h>
#include <smp.h>
#include <thread.h>
#include <util/AutoLock.h>


struct per_cpu_timer_data {
	spinlock		lock;
	timer* volatile	events;
	timer* volatile	current_event;
	vint32			current_event_in_progress;
};

static per_cpu_timer_data sPerCPU[B_MAX_CPU_COUNT];


//#define TRACE_TIMER
#ifdef TRACE_TIMER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


status_t
timer_init(kernel_args* args)
{
	TRACE(("timer_init: entry\n"));

	return arch_init_timer(args);
}


/*! NOTE: expects interrupts to be off */
static void
add_event_to_list(timer* event, timer* volatile* list)
{
	timer* next;
	timer* last = NULL;

	// stick it in the event list
	for (next = *list; next; last = next, next = (timer*)next->next) {
		if ((bigtime_t)next->schedule_time >= (bigtime_t)event->schedule_time)
			break;
	}

	if (last != NULL) {
		event->next = last->next;
		last->next = event;
	} else {
		event->next = next;
		*list = event;
	}
}


int32
timer_interrupt()
{
	timer* event;
	spinlock* spinlock;
	per_cpu_timer_data& cpuData = sPerCPU[smp_get_current_cpu()];
	int32 rc = B_HANDLED_INTERRUPT;

	TRACE(("timer_interrupt: time %lld, cpu %ld\n", system_time(),
		smp_get_current_cpu()));

	spinlock = &cpuData.lock;

	acquire_spinlock(spinlock);

	event = cpuData.events;
	while (event != NULL && ((bigtime_t)event->schedule_time < system_time())) {
		// this event needs to happen
		int mode = event->flags;

		cpuData.events = (timer*)event->next;
		cpuData.current_event = event;
		cpuData.current_event_in_progress = 1;
		event->schedule_time = 0;

		release_spinlock(spinlock);

		TRACE(("timer_interrupt: calling hook %p for event %p\n", event->hook,
			event));

		// call the callback
		// note: if the event is not periodic, it is ok
		// to delete the event structure inside the callback
		if (event->hook) {
			bool callHook = true;

			// we may need to acquire the thread spinlock
			if ((mode & B_TIMER_ACQUIRE_THREAD_LOCK) != 0) {
				GRAB_THREAD_LOCK();

				// If the event has been cancelled in the meantime, we don't
				// call the hook anymore.
				if (cpuData.current_event == NULL)
					callHook = false;
			}

			if (callHook)
				rc = event->hook(event);

			if ((mode & B_TIMER_ACQUIRE_THREAD_LOCK) != 0)
				RELEASE_THREAD_LOCK();
		}

		cpuData.current_event_in_progress = 0;

		acquire_spinlock(spinlock);

		if ((mode & ~B_TIMER_FLAGS) == B_PERIODIC_TIMER
			&& cpuData.current_event != NULL) {
			// we need to adjust it and add it back to the list
			bigtime_t scheduleTime = system_time() + event->period;
			if (scheduleTime == 0) {
				// if we wrapped around and happen to hit zero, set
				// it to one, since zero represents not scheduled
				scheduleTime = 1;
			}
			event->schedule_time = (int64)scheduleTime;
			add_event_to_list(event, &cpuData.events);
		}

		cpuData.current_event = NULL;

		event = cpuData.events;
	}

	// setup the next hardware timer
	if (cpuData.events != NULL) {
		bigtime_t timeout = (bigtime_t)cpuData.events->schedule_time
			- system_time();
		if (timeout <= 0)
			timeout = 1;
		arch_timer_set_hardware_timer(timeout);
	}

	release_spinlock(spinlock);

	return rc;
}


status_t
add_timer(timer* event, timer_hook hook, bigtime_t period, int32 flags)
{
	bigtime_t scheduleTime;
	bigtime_t currentTime = system_time();
	cpu_status state;

	if (event == NULL || hook == NULL || period < 0)
		return B_BAD_VALUE;

	TRACE(("add_timer: event %p\n", event));

	scheduleTime = period;
	if ((flags & ~B_TIMER_FLAGS) != B_ONE_SHOT_ABSOLUTE_TIMER)
		scheduleTime += currentTime;
	if (scheduleTime == 0)
		scheduleTime = 1;

	event->schedule_time = (int64)scheduleTime;
	event->period = period;
	event->hook = hook;
	event->flags = flags;

	state = disable_interrupts();
	int currentCPU = smp_get_current_cpu();
	per_cpu_timer_data& cpuData = sPerCPU[currentCPU];
	acquire_spinlock(&cpuData.lock);

	add_event_to_list(event, &cpuData.events);
	event->cpu = currentCPU;

	// if we were stuck at the head of the list, set the hardware timer
	if (event == cpuData.events)
		arch_timer_set_hardware_timer(scheduleTime - currentTime);

	release_spinlock(&cpuData.lock);
	restore_interrupts(state);

	return B_OK;
}


bool
cancel_timer(timer* event)
{
	TRACE(("cancel_timer: event %p\n", event));

	InterruptsLocker _;

	// lock the right CPU spinlock
	int cpu = event->cpu;
	SpinLocker spinLocker;
	while (true) {
		if (cpu >= B_MAX_CPU_COUNT)
			return false;

		spinLocker.SetTo(sPerCPU[cpu].lock, false);
		if (cpu == event->cpu)
			break;

		// cpu field changed while we were trying to lock
		spinLocker.Unlock();
		cpu = event->cpu;
	}

	per_cpu_timer_data& cpuData = sPerCPU[cpu];

	if (event != cpuData.current_event) {
		// The timer hook is not yet being executed.
		timer* current = cpuData.events;
		timer* last = NULL;

		while (current != NULL) {
			if (current == event) {
				// we found it
				if (last == NULL)
					cpuData.events = current->next;
				else
					last->next = current->next;
				current->next = NULL;
				// break out of the whole thing
				break;
			}
			last = current;
			current = current->next;
		}

		// If not found, we assume this was a one-shot timer and has already
		// fired.
		if (current == NULL)
			return true;

		// invalidate CPU field
		event->cpu = 0xffff;

		// If on the current CPU, also reset the hardware timer.
		if (cpu == smp_get_current_cpu()) {
			if (cpuData.events == NULL)
				arch_timer_clear_hardware_timer();
			else {
				arch_timer_set_hardware_timer(
					(bigtime_t)cpuData.events->schedule_time - system_time());
			}
		}

		return false;
	}

	// The timer hook is currently being executed. We clear the current
	// event so that timer_interrupt() will not reschedule periodic timers.
	cpuData.current_event = NULL;

	// Unless this is a kernel-private timer that also requires the thread
	// lock to be held while calling the event hook, we'll have to wait
	// for the hook to complete. When called from the timer hook we don't
	// wait either, of course.
	if ((event->flags & B_TIMER_ACQUIRE_THREAD_LOCK) == 0
		|| cpu == smp_get_current_cpu()) {
		spinLocker.Unlock();

		while (cpuData.current_event_in_progress == 1) {
			PAUSE();
		}
	}

	return true;
}


void
spin(bigtime_t microseconds)
{
	bigtime_t time = system_time();

	while ((system_time() - time) < microseconds) {
		PAUSE();
	}
}
