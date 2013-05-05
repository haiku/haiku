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
#include <safemode.h>

#include <arch/timer.h>

#include <arch/x86/smp_priv.h>
#include <arch/x86/timer.h>


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
	&gHPETTimer,
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


static void
sort_timers(timer_info *timers[], int numTimers)
{
	timer_info *tempPtr;
	int max = 0;	
	int i = 0;
	int j = 0;
	
	for (i = 0; i < numTimers - 1; i++) {
		max = i;
		for (j = i + 1; j < numTimers; j++) {
			if (timers[j]->get_priority() > timers[max]->get_priority())
				max = j;
		}
		if (max != i) {
			tempPtr = timers[max];
			timers[max] = timers[i];
			timers[i] = tempPtr;		
		}
	}
	
#if 0
	for (i = 0; i < numTimers; i++)
		dprintf(" %s: priority %d\n", timers[i]->name, timers[i]->get_priority());
#endif
}


int
arch_init_timer(kernel_args *args)
{
	// Sort timers by priority
	sort_timers(sTimers, (sizeof(sTimers) / sizeof(sTimers[0])) - 1);

	timer_info *timer = NULL;
	cpu_status state = disable_interrupts();
	
	for (int i = 0; (timer = sTimers[i]) != NULL; i++) {
		if (timer->init(args) == B_OK)
			break;
	}

	sTimer = timer;

	if (sTimer != NULL) {
		dprintf("arch_init_timer: using %s timer.\n", sTimer->name);
	} else {
		panic("No system timers were found usable.\n");
	}

	restore_interrupts(state);

	return 0;
}

