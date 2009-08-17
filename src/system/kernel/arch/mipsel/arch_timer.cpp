/*
 * Copyright 2007-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		François Revol <revol@free.fr>
 *		Jonas Sundström <jonas@kirilla.com>
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
#warning IMPLEMENT arch_timer_set_hardware_timer
}


void 
arch_timer_clear_hardware_timer()
{
#warning IMPLEMENT arch_timer_clear_hardware_timer
}


int 
arch_init_timer(kernel_args* args)
{
#warning IMPLEMENT arch_init_timer
	return 0;
}

