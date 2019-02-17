/*
 * Copyright 2003-2019, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		Adrien Destugues <pulkomandy@pulkomandy.tk>
 */
#ifndef _KERNEL_ARCH_SPARC_THREAD_H
#define _KERNEL_ARCH_SPARC_THREAD_H

#include <arch/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

void ppc_push_iframe(struct iframe_stack *stack, struct iframe *frame);
void ppc_pop_iframe(struct iframe_stack *stack);
struct iframe *ppc_get_user_iframe(void);


static inline Thread *
arch_thread_get_current_thread(void)
{
    Thread *t;
    //asm volatile("mfsprg2 %0" : "=r"(t));
	t = NULL;
    return t;
}


static inline void
arch_thread_set_current_thread(Thread *t)
{
    //asm volatile("mtsprg2 %0" : : "r"(t));
}


#ifdef __cplusplus
}
#endif


#endif /* _KERNEL_ARCH_SPARC_THREAD_H */

