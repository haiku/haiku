/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <kscheduler.h>
#include <smp.h>

#include "scheduler_affine.h"
#include "scheduler_simple.h"

// Defines which scheduler(s) to use. Possible values:
// 0 - Auto-select scheduler based on detected core count
// 1 - Always use the simple scheduler
// 2 - Always use the affine scheduler
#define SCHEDULER_TYPE 1

struct scheduler_ops* gScheduler;
SchedulerListenerList gSchedulerListeners;


SchedulerListener::~SchedulerListener()
{
}


/*!	Add the given scheduler listener. Thread lock must be held.
*/
void
scheduler_add_listener(struct SchedulerListener* listener)
{
	gSchedulerListeners.Add(listener);
}


/*!	Remove the given scheduler listener. Thread lock must be held.
*/
void
scheduler_remove_listener(struct SchedulerListener* listener)
{
	gSchedulerListeners.Remove(listener);
}


void
scheduler_init(void)
{
	int32 cpu_count = smp_get_num_cpus();
	dprintf("scheduler_init: found %ld logical cpus\n", cpu_count);
#if SCHEDULER_TYPE == 0
	if (cpu_count > 1) {
		dprintf("scheduler_init: using affine scheduler\n");
		scheduler_affine_init();
	} else {
		dprintf("scheduler_init: using simple scheduler\n");
		scheduler_simple_init();
	}
#elif SCHEDULER_TYPE == 1
	scheduler_simple_init();
#elif SCHEDULER_TYPE == 2
	scheduler_affine_init();
#endif

#if SCHEDULER_TRACING
	add_debugger_command_etc("scheduler", &cmd_scheduler,
		"Analyze scheduler tracing information",
		"<thread>\n"
		"Analyzes scheduler tracing information for a given thread.\n"
		"  <thread>  - ID of the thread.\n", 0);
#endif
}
