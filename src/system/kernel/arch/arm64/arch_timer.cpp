/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include <boot/stage2.h>
#include <kernel.h>
#include <debug.h>

#include <timer.h>
#include <arch/timer.h>
#include <arch/cpu.h>


void
arch_timer_set_hardware_timer(bigtime_t timeout)
{
}


void
arch_timer_clear_hardware_timer()
{
}


int
arch_init_timer(kernel_args *args)
{
	return B_OK;
}


bigtime_t
system_time(void)
{
	return 0;
}
