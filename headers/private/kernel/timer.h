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

/* kernel functions */
status_t timer_init(struct kernel_args *);
int32 timer_interrupt(void);

/* this one is only to be used by the scheduler */
status_t _local_timer_cancel_event(int currentCPU, timer *event);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_TIMER_H */
