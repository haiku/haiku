/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef KERNEL_ARCH_TIMER_H
#define KERNEL_ARCH_TIMER_H

#include <SupportDefs.h>

struct kernel_args;


void arch_timer_set_hardware_timer(bigtime_t timeout);
void arch_timer_clear_hardware_timer(void);
int arch_init_timer(struct kernel_args *ka);

#endif	/* KERNEL_ARCH_TIMER_H */
