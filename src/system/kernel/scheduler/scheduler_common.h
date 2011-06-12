/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_SCHEDULER_COMMON_H
#define KERNEL_SCHEDULER_COMMON_H


#include <kscheduler.h>
#include <smp.h>
#include <user_debugger.h>


/*!	Switches the currently running thread.
	This is a service function for scheduler implementations.

	\param fromThread The currently running thread.
	\param toThread The thread to switch to. Must be different from
		\a fromThread.
*/
static inline void
scheduler_switch_thread(Thread* fromThread, Thread* toThread)
{
	// notify the user debugger code
	if ((fromThread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_unscheduled(fromThread);

	// stop CPU time based user timers
	if (fromThread->HasActiveCPUTimeUserTimers()
		|| fromThread->team->HasActiveCPUTimeUserTimers()) {
		user_timer_stop_cpu_timers(fromThread, toThread);
	}

	// update CPU and Thread structures and perform the context switch
	cpu_ent* cpu = fromThread->cpu;
	toThread->previous_cpu = toThread->cpu = cpu;
	fromThread->cpu = NULL;
	cpu->running_thread = toThread;
	cpu->previous_thread = fromThread;

	arch_thread_set_current_thread(toThread);
	arch_thread_context_switch(fromThread, toThread);

	// The use of fromThread below looks weird, but is correct. fromThread had
	// been unscheduled earlier, but is back now. For a thread scheduled the
	// first time the same is done in thread.cpp:common_thread_entry().

	// continue CPU time based user timers
	if (fromThread->HasActiveCPUTimeUserTimers()
		|| fromThread->team->HasActiveCPUTimeUserTimers()) {
		user_timer_continue_cpu_timers(fromThread, cpu->previous_thread);
	}

	// notify the user debugger code
	if ((fromThread->flags & THREAD_FLAGS_DEBUGGER_INSTALLED) != 0)
		user_debug_thread_scheduled(fromThread);
}


static inline void
scheduler_update_thread_times(Thread* oldThread, Thread* nextThread)
{
	bigtime_t now = system_time();
	if (oldThread == nextThread) {
		acquire_spinlock(&oldThread->time_lock);
		oldThread->kernel_time += now - oldThread->last_time;
		oldThread->last_time = now;
		release_spinlock(&oldThread->time_lock);
	} else {
		acquire_spinlock(&oldThread->time_lock);
		oldThread->kernel_time += now - oldThread->last_time;
		release_spinlock(&oldThread->time_lock);

		acquire_spinlock(&nextThread->time_lock);
		nextThread->last_time = now;
		release_spinlock(&nextThread->time_lock);
	}

	// If the old thread's team has user time timers, check them now.
	Team* team = oldThread->team;
	if (team->HasActiveUserTimeUserTimers())
		user_timer_check_team_user_timers(team);
}


#endif	// KERNEL_SCHEDULER_COMMON_H
