/*
 * Copyright 2002-2011, Haiku. All rights reserved.
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
#include <debug.h>
#include <elf.h>
#include <real_time_clock.h>
#include <smp.h>
#include <thread.h>
#include <util/AutoLock.h>


struct per_cpu_timer_data {
	spinlock		lock;
	timer* volatile	events;
	timer* volatile	current_event;
	vint32			current_event_in_progress;
	bigtime_t		real_time_offset;
};

static per_cpu_timer_data sPerCPU[B_MAX_CPU_COUNT];


//#define TRACE_TIMER
#ifdef TRACE_TIMER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


/*!	Sets the hardware timer to the given absolute time.

	\param scheduleTime The absolute system time for the timer expiration.
	\param now The current system time.
*/
static void
set_hardware_timer(bigtime_t scheduleTime, bigtime_t now)
{
	arch_timer_set_hardware_timer(scheduleTime > now ? scheduleTime - now : 0);
}


/*!	Sets the hardware timer to the given absolute time.

	\param scheduleTime The absolute system time for the timer expiration.
*/
static inline void
set_hardware_timer(bigtime_t scheduleTime)
{
	set_hardware_timer(scheduleTime, system_time());
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


static void
per_cpu_real_time_clock_changed(void*, int cpu)
{
	per_cpu_timer_data& cpuData = sPerCPU[cpu];
	InterruptsSpinLocker cpuDataLocker(cpuData.lock);

	bigtime_t realTimeOffset = rtc_boot_time();
	if (realTimeOffset == cpuData.real_time_offset)
		return;

	// The real time offset has changed. We need to update all affected
	// timers. First find and dequeue them.
	bigtime_t timeDiff = cpuData.real_time_offset - realTimeOffset;
	cpuData.real_time_offset = realTimeOffset;

	timer* affectedTimers = NULL;
	timer* volatile* it = &cpuData.events;
	timer* firstEvent = *it;
	while (timer* event = *it) {
		// check whether it's an absolute real-time timer
		uint32 flags = event->flags;
		if ((flags & ~B_TIMER_FLAGS) != B_ONE_SHOT_ABSOLUTE_TIMER
			|| (flags & B_TIMER_REAL_TIME_BASE) == 0) {
			it = &event->next;
			continue;
		}

		// Yep, remove the timer from the queue and add it to the
		// affectedTimers list.
		*it = event->next;
		event->next = affectedTimers;
		affectedTimers = event;
	}

	// update and requeue the affected timers
	bool firstEventChanged = cpuData.events != firstEvent;
	firstEvent = cpuData.events;

	while (affectedTimers != NULL) {
		timer* event = affectedTimers;
		affectedTimers = event->next;

		bigtime_t oldTime = event->schedule_time;
		event->schedule_time += timeDiff;

		// handle over-/underflows
		if (timeDiff >= 0) {
			if (event->schedule_time < oldTime)
				event->schedule_time = B_INFINITE_TIMEOUT;
		} else {
			if (event->schedule_time < 0)
				event->schedule_time = 0;
		}

		add_event_to_list(event, &cpuData.events);
	}

	firstEventChanged |= cpuData.events != firstEvent;

	// If the first event has changed, reset the hardware timer.
	if (firstEventChanged)
		set_hardware_timer(cpuData.events->schedule_time);
}


// #pragma mark - debugging


static int
dump_timers(int argc, char** argv)
{
	int32 cpuCount = smp_get_num_cpus();
	for (int32 i = 0; i < cpuCount; i++) {
		kprintf("CPU %" B_PRId32 ":\n", i);

		if (sPerCPU[i].events == NULL) {
			kprintf("  no timers scheduled\n");
			continue;
		}

		for (timer* event = sPerCPU[i].events; event != NULL;
				event = event->next) {
			kprintf("  [%9lld] %p: ", (long long)event->schedule_time, event);
			if ((event->flags & ~B_TIMER_FLAGS) == B_PERIODIC_TIMER)
				kprintf("periodic %9lld, ", (long long)event->period);
			else
				kprintf("one shot,           ");

			kprintf("flags: %#x, user data: %p, callback: %p  ",
				event->flags, event->user_data, event->hook);

			// look up and print the hook function symbol
			const char* symbol;
			const char* imageName;
			bool exactMatch;

			status_t error = elf_debug_lookup_symbol_address(
				(addr_t)event->hook, NULL, &symbol, &imageName, &exactMatch);
			if (error == B_OK && exactMatch) {
				if (const char* slash = strchr(imageName, '/'))
					imageName = slash + 1;

				kprintf("   %s:%s", imageName, symbol);
			}

			kprintf("\n");
		}
	}

	kprintf("current time: %lld\n", (long long)system_time());

	return 0;
}


// #pragma mark - kernel-private


status_t
timer_init(kernel_args* args)
{
	TRACE(("timer_init: entry\n"));

	if (arch_init_timer(args) != B_OK)
		panic("arch_init_timer() failed");

	add_debugger_command_etc("timers", &dump_timers, "List all timers",
		"\n"
		"Prints a list of all scheduled timers.\n", 0);

	return B_OK;
}


void
timer_init_post_rtc(void)
{
	bigtime_t realTimeOffset = rtc_boot_time();

	int32 cpuCount = smp_get_num_cpus();
	for (int32 i = 0; i < cpuCount; i++)
		sPerCPU[i].real_time_offset = realTimeOffset;
}


void
timer_real_time_clock_changed()
{
	call_all_cpus(&per_cpu_real_time_clock_changed, NULL);
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

		release_spinlock(spinlock);

		TRACE(("timer_interrupt: calling hook %p for event %p\n", event->hook,
			event));

		// call the callback
		// note: if the event is not periodic, it is ok
		// to delete the event structure inside the callback
		if (event->hook) {
			bool callHook = true;

			// we may need to acquire the scheduler lock
			if ((mode & B_TIMER_ACQUIRE_SCHEDULER_LOCK) != 0) {
				acquire_spinlock(&gSchedulerLock);

				// If the event has been cancelled in the meantime, we don't
				// call the hook anymore.
				if (cpuData.current_event == NULL)
					callHook = false;
			}

			if (callHook)
				rc = event->hook(event);

			if ((mode & B_TIMER_ACQUIRE_SCHEDULER_LOCK) != 0)
				release_spinlock(&gSchedulerLock);
		}

		cpuData.current_event_in_progress = 0;

		acquire_spinlock(spinlock);

		if ((mode & ~B_TIMER_FLAGS) == B_PERIODIC_TIMER
			&& cpuData.current_event != NULL) {
			// we need to adjust it and add it back to the list
			event->schedule_time += event->period;

			// If the new schedule time is a full interval or more in the past,
			// skip ticks.
			bigtime_t now =  system_time();
			if (now >= event->schedule_time + event->period) {
				// pick the closest tick in the past
				event->schedule_time = now
					- (now - event->schedule_time) % event->period;
			}

			add_event_to_list(event, &cpuData.events);
		}

		cpuData.current_event = NULL;

		event = cpuData.events;
	}

	// setup the next hardware timer
	if (cpuData.events != NULL)
		set_hardware_timer(cpuData.events->schedule_time);

	release_spinlock(spinlock);

	return rc;
}


// #pragma mark - public API


status_t
add_timer(timer* event, timer_hook hook, bigtime_t period, int32 flags)
{
	bigtime_t currentTime = system_time();
	cpu_status state;

	if (event == NULL || hook == NULL || period < 0)
		return B_BAD_VALUE;

	TRACE(("add_timer: event %p\n", event));

	// compute the schedule time
	bigtime_t scheduleTime;
	if ((flags & B_TIMER_USE_TIMER_STRUCT_TIMES) != 0) {
		scheduleTime = event->schedule_time;
		period = event->period;
	} else {
		scheduleTime = period;
		if ((flags & ~B_TIMER_FLAGS) != B_ONE_SHOT_ABSOLUTE_TIMER)
			scheduleTime += currentTime;
		event->schedule_time = (int64)scheduleTime;
		event->period = period;
	}

	event->hook = hook;
	event->flags = flags;

	state = disable_interrupts();
	int currentCPU = smp_get_current_cpu();
	per_cpu_timer_data& cpuData = sPerCPU[currentCPU];
	acquire_spinlock(&cpuData.lock);

	// If the timer is an absolute real-time base timer, convert the schedule
	// time to system time.
	if ((flags & ~B_TIMER_FLAGS) == B_ONE_SHOT_ABSOLUTE_TIMER
		&& (flags & B_TIMER_REAL_TIME_BASE) != 0) {
		if (event->schedule_time > cpuData.real_time_offset)
			event->schedule_time -= cpuData.real_time_offset;
		else
			event->schedule_time = 0;
	}

	add_event_to_list(event, &cpuData.events);
	event->cpu = currentCPU;

	// if we were stuck at the head of the list, set the hardware timer
	if (event == cpuData.events)
		set_hardware_timer(scheduleTime, currentTime);

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
			else
				set_hardware_timer(cpuData.events->schedule_time);
		}

		return false;
	}

	// The timer hook is currently being executed. We clear the current
	// event so that timer_interrupt() will not reschedule periodic timers.
	cpuData.current_event = NULL;

	// Unless this is a kernel-private timer that also requires the scheduler
	// lock to be held while calling the event hook, we'll have to wait
	// for the hook to complete. When called from the timer hook we don't
	// wait either, of course.
	if ((event->flags & B_TIMER_ACQUIRE_SCHEDULER_LOCK) == 0
		&& cpu != smp_get_current_cpu()) {
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
