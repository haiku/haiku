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

/* as we won't support SMP on arm (yet?) we can use a global here */
/*TODO SMP-ARMS are comming..*/
extern struct thread *gCurrentThread;

extern inline struct thread *
arch_thread_get_current_thread(void)
{
	return gCurrentThread;
}


extern inline void
arch_thread_set_current_thread(struct thread *t)
{
	gCurrentThread = t;
}

#ifdef __cplusplus
}
#endif


#endif /* _KERNEL_ARCH_ARM_THREAD_H */
