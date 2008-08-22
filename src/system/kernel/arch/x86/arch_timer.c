/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <boot/stage2.h>
#include <kernel.h>

#include <arch/int.h>
#include <arch/cpu.h>

#include <console.h>
#include <debug.h>
#include <timer.h>
#include <int.h>

#include <arch/timer.h>

#include <arch/x86/smp_priv.h>
#include <arch/x86/timer.h>

#include "interrupts.h"

//#define TRACE_TIMER
#ifdef TRACE_TIMER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

extern timer_info gPITTimer;
extern timer_info gAPICTimer;
extern timer_info gHPETTimer;

static timer_info *sTimers[] = {
//	&gHPETTimer,
	&gAPICTimer,
	&gPITTimer,
	NULL
};

static timer_info *sTimer = NULL;

void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	TRACE(("arch_timer_set_hardware_timer: timeout %lld\n", timeout));
	sTimer->set_hardware_timer(timeout);
}


void
arch_timer_clear_hardware_timer(void)
{
	TRACE(("arch_timer_clear_hardware_timer\n"));
	sTimer->clear_hardware_timer();
}


int
arch_init_timer(kernel_args *args)
{
	int i = 0;
	int bestPriority = -1;
	timer_info *timer = NULL;
	cpu_status state = disable_interrupts();

	for (i = 0; (timer = sTimers[i]) != NULL; i++) {
		int priority = timer->get_priority();

		if (priority < bestPriority) {
			TRACE(("arch_init_timer: Skipping %s because there is a higher priority timer (%s) initialized.\n", timer->name, sTimer->name));
			continue;
		}

		if (timer->init(args) != B_OK) {
			TRACE(("arch_init_timer: %s failed init. Skipping.\n", timer->name));
			continue;
		}

		if (priority > bestPriority) {
			bestPriority = priority;
			sTimer = timer;
			TRACE(("arch_init_timer: %s is now best timer module with prio %d.\n", timer->name, bestPriority));
		}
	}

	if (sTimer != NULL) {
		dprintf("arch_init_timer: using %s timer.\n", sTimer->name);
	} else {
		panic("No system timers were found usable.\n");
	}

	restore_interrupts(state);

	return 0;
}

