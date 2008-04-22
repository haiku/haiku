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

#define B_TIMER_ACQUIRE_THREAD_LOCK	0x8000
#define B_TIMER_FLAGS				B_TIMER_ACQUIRE_THREAD_LOCK

/* kernel functions */
status_t timer_init(struct kernel_args *);
int32 timer_interrupt(void);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_TIMER_H */
