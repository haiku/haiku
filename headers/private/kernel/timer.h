/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_TIMER_H
#define _KERNEL_TIMER_H

#include <stage2.h>

typedef int (*timer_callback)(void *);


typedef enum {
	TIMER_MODE_ONESHOT = 0,
	TIMER_MODE_PERIODIC
} timer_mode;

struct timer_event {
	struct timer_event *next;
	timer_mode mode;
	bigtime_t sched_time;
	bigtime_t periodic_time;
	timer_callback func;
	void *data;
};

void timer_setup_timer(timer_callback func, void *data, struct timer_event *event);
int timer_set_event(bigtime_t relative_time, timer_mode mode, struct timer_event *event);
int timer_cancel_event(struct timer_event *event);
void spin(bigtime_t microseconds);

/* kernel functions */
int  timer_init(kernel_args *);
int  timer_interrupt(void);

/*
 * these two are only to be used by the scheduler
 */
int local_timer_cancel_event(struct timer_event *event);
int _local_timer_cancel_event(int curr_cpu, struct timer_event *event);

#endif

