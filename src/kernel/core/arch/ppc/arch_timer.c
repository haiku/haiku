/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <boot/stage2.h>
#include <kernel/kernel.h>

#include <kernel/timer.h>

void arch_timer_set_hardware_timer(time_t timeout)
{
}

void arch_timer_clear_hardware_timer()
{
}

int arch_init_timer(kernel_args *ka)
{
	return 0;
}
