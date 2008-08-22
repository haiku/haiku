/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_TIMERS_HPET_H
#define _KERNEL_ARCH_x86_TIMERS_HPET_H

#include <SupportDefs.h>

/* Method prototypes */
static int hpet_get_prio(void);
static status_t hpet_set_hardware_timer(bigtime_t relativeTimeout);
static status_t hpet_clear_hardware_timer(void);
static status_t hpet_init(struct kernel_args *args);

#endif /* _KERNEL_ARCH_x86_TIMERS_HPET_H */
