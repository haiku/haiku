/* 
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_TIMER_H
#define _KERNEL_TIMER_H


#include <KernelExport.h>


#ifdef __cplusplus
extern "C" {
#endif

/* kernel functions */
int  timer_init(kernel_args *);
int  timer_interrupt(void);

/* these two are only to be used by the scheduler */
int local_timer_cancel_event(timer *event);
int _local_timer_cancel_event(int curr_cpu, timer *event);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_TIMER_H */
