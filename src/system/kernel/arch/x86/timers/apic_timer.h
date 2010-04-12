/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_TIMERS_APIC_H
#define _KERNEL_ARCH_x86_TIMERS_APIC_H

#include <SupportDefs.h>

status_t apic_timer_per_cpu_init(struct kernel_args *args, int32 cpu);

#endif /* _KERNEL_ARCH_x86_TIMERS_APIC_H */
