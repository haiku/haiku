/*
** Copyright 2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef KERNEL_ARCH_TIMER_H
#define KERNEL_ARCH_TIMER_H

#include <SupportDefs.h>

struct kernel_args;

#ifdef __cplusplus
extern "C" {
#endif

extern void arch_timer_set_hardware_timer(bigtime_t timeout);
extern void arch_timer_clear_hardware_timer(void);
extern int arch_init_timer(struct kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_ARCH_TIMER_H */
