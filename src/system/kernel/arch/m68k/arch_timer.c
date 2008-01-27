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


void 
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	if(timeout < 1000)
		timeout = 1000;
	
	platform_timer_set_hardware_timer(timeout);
}


void 
arch_timer_clear_hardware_timer()
{
	platform_timer_set_hardware_timer(timeout);
}


int 
arch_init_timer(kernel_args *ka)
{
	return platform_init_timer(ka);
}

