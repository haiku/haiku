/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <boot/stage2.h>
#include <kernel.h>
#include <debug.h>

#include <timer.h>
#include <arch/timer.h>


static bigtime_t sTickRate;


void 
arch_timer_set_hardware_timer(bigtime_t timeout)
{
	bigtime_t new_val_64;

	if(timeout < 1000)
		timeout = 1000;

	new_val_64 = (timeout * sTickRate) / 1000000;

	asm("mtdec	%0" :: "r"((uint32)new_val_64));
}


void 
arch_timer_clear_hardware_timer()
{
	asm("mtdec	%0" :: "r"(0x7fffffff));
}


int 
arch_init_timer(kernel_args *ka)
{
	sTickRate = ka->arch_args.time_base_frequency;

	return 0;
}

