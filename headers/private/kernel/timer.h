/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_TIMER_H
#define _KERNEL_TIMER_H


#include <KernelExport.h>


#ifdef __cplusplus
extern "C" {
#endif

struct kernel_args;

#define B_TIMER_REAL_TIME_BASE			0x2000
	// For an absolute timer the given time is interpreted as a real-time, not
	// as a system time. Note that setting the real-time clock will cause the
	// timer to be updated -- it will expire according to the new clock.
	// Relative timers are unaffected by this flag.
#define B_TIMER_USE_TIMER_STRUCT_TIMES	0x4000
	// For add_timer(): Use the timer::schedule_time (absolute time) and
	// timer::period values instead of the period parameter.
#define B_TIMER_ACQUIRE_SCHEDULER_LOCK	0x8000
	// The timer hook is invoked with the scheduler lock held. When invoking
	// cancel_timer() with the scheduler lock held, too, this helps to avoid
	// race conditions.
#define B_TIMER_FLAGS	\
	(B_TIMER_USE_TIMER_STRUCT_TIMES | B_TIMER_ACQUIRE_SCHEDULER_LOCK	\
	| B_TIMER_REAL_TIME_BASE)

/* Timer info structure */
struct timer_info {
	const char *name;
	int (*get_priority)(void);
	status_t (*set_hardware_timer)(bigtime_t timeout);
	status_t (*clear_hardware_timer)(void);
	status_t (*init)(struct kernel_args *args);
};

typedef struct timer_info timer_info;


/* kernel functions */
status_t timer_init(struct kernel_args *);
void timer_init_post_rtc(void);
void timer_real_time_clock_changed();
int32 timer_interrupt(void);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_TIMER_H */
