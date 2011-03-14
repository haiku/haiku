/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Johannes Wischert <johanneswi@gmail.com>
 */
#ifndef _KERNEL_ARCH_ARM_THREAD_H
#define _KERNEL_ARCH_ARM_THREAD_H

#include <arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

void arm_push_iframe(struct iframe_stack *stack, struct iframe *frame);
void arm_pop_iframe(struct iframe_stack *stack);
struct iframe *arm_get_user_iframe(void);

/* TODO fix this global once we support SMP ARM... */
extern Thread *gCurrentThread;

extern inline Thread *
arch_thread_get_current_thread(void)
{
	return gCurrentThread;
}


extern inline void
arch_thread_set_current_thread(Thread *t)
{
	gCurrentThread = t;
}

#ifdef __cplusplus
}
#endif


#endif /* _KERNEL_ARCH_ARM_THREAD_H */
