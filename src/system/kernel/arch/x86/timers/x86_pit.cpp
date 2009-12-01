/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <timer.h>
#include <arch/x86/timer.h>

#include <arch/int.h>
#include <arch/cpu.h>

#include "pit.h"

static bool sPITTimerInitialized = false;

struct timer_info gPITTimer = {
	"PIT",
	&pit_get_prio,
	&pit_set_hardware_timer,
	&pit_clear_hardware_timer,
	&pit_init
};


static int
pit_get_prio(void)
{
	return 1;
}


static int32
pit_timer_interrupt(void *data)
{
	return timer_interrupt();
}


static status_t
pit_set_hardware_timer(bigtime_t relativeTimeout)
{
	uint16 nextEventClocks;

	if (relativeTimeout <= 0)
		nextEventClocks = 2;
	else if (relativeTimeout < PIT_MAX_TIMER_INTERVAL)
		nextEventClocks = relativeTimeout * PIT_CLOCK_RATE / 1000000;
	else
		nextEventClocks = 0xffff;

	out8(PIT_SELCH0 | PIT_RWBOTH | PIT_MD_INTON0, PIT_CTRL);
	out8(nextEventClocks & 0xff, PIT_CNT0);
	out8((nextEventClocks >> 8) & 0xff, PIT_CNT0);

	arch_int_enable_io_interrupt(0);
	return B_OK;
}


static status_t
pit_clear_hardware_timer(void)
{
	arch_int_disable_io_interrupt(0);
	return B_OK;
}


static status_t
pit_init(struct kernel_args *args)
{
	if (sPITTimerInitialized) {
		return B_OK;
	}
	
	install_io_interrupt_handler(0, &pit_timer_interrupt, NULL, 0);
	pit_clear_hardware_timer();	

	sPITTimerInitialized = true;

	return B_OK;
}
