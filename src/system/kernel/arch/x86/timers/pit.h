/*
 * Copyright 2008, Dustin Howett, dustin.howett@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_x86_TIMERS_PIT_H
#define _KERNEL_ARCH_x86_TIMERS_PIT_H

#include <SupportDefs.h>

/* ports */
#define PIT_CTRL	0x43
#define PIT_CNT0	0x40
#define PIT_CNT1	0x41
#define PIT_CNT2	0x42

/* commands */
#define PIT_SELCH0	0x00
#define PIT_SELCH1	0x40
#define PIT_SELCH2	0x80

#define PIT_RWLOW	0x10
#define PIT_RWHIGH	0x20
#define PIT_RWBOTH	0x30

#define PIT_MD_INTON0	0x00
#define PIT_MD_ONESHOT	0x02
#define PIT_MD_RTGEN	0x04
#define PIT_MD_SQGEN	0x06
#define PIT_MD_SW_STRB	0x08
#define PIT_MD_HW_STRB	0x0A

#define PIT_BCD		0x01

#define PIT_LATCH	0x00

#define PIT_READ	0xF0
#define PIT_CNT		0x20
#define PIT_STAT	0x10

#define PIT_CLOCK_RATE	1193180
#define PIT_MAX_TIMER_INTERVAL (0xffff * 1000000ll / PIT_CLOCK_RATE)

/* Method Prototypes */
static int pit_get_prio(void);
static status_t pit_set_hardware_timer(bigtime_t relativeTimeout);
static status_t pit_clear_hardware_timer(void);
static status_t pit_init(struct kernel_args *args);

#endif /* _KERNEL_ARCH_x86_TIMERS_PIT_H */
