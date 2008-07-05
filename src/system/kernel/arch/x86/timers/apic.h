/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_TIMERS_APIC_H
#define _KERNEL_ARCH_x86_TIMERS_APIC_H

#include <SupportDefs.h>

/* Method Prototypes */
static int apic_get_prio(void);
static status_t apic_set_hardware_timer(bigtime_t relativeTimeout);
static status_t apic_clear_hardware_timer(void);
static status_t apic_init(struct kernel_args *args);
status_t apic_smp_init_timer(struct kernel_args *args, int32 cpu);

#endif /* _KERNEL_ARCH_x86_TIMERS_APIC_H */
