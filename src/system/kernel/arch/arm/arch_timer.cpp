/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <boot/stage2.h>
#include <kernel.h>
#include <debug.h>

#include <timer.h>
#include <arch/timer.h>
//#include <arch_platform.h>


void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	#warning ARM:WRITEME
	// M68KPlatform::Default()->SetHardwareTimer(timeout);
}


void
arch_timer_clear_hardware_timer()
{
	#warning ARM:WRITEME
	// M68KPlatform::Default()->ClearHardwareTimer();
}


int
arch_init_timer(kernel_args *args)
{
	#warning ARM:WRITEME
	// M68KPlatform::Default()->InitTimer(args);
	return 0;
}

