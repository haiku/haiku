/*
 * Copyright 2003-2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Jonas Sundström <jonas@kirilla.com>
 */
#ifndef _KERNEL_ARCH_RISCV64_THREAD_H
#define _KERNEL_ARCH_RISCV64_THREAD_H

#include <arch/cpu.h>

#warning IMPLEMENT arch_thread.h

#ifdef __cplusplus
extern "C" {
#endif

void riscv64_push_iframe(struct iframe_stack* stack, struct iframe* frame);
void riscv64_pop_iframe(struct iframe_stack* stack);
struct iframe* riscv64_get_user_iframe(void);


static inline Thread*
arch_thread_get_current_thread(void)
{
#warning IMPLEMENT arch_thread_get_current_thread
    Thread* t = NULL;
    return t;
}


static inline void
arch_thread_set_current_thread(Thread* t)
{
#warning IMPLEMENT arch_thread_set_current_thread
}


#ifdef __cplusplus
}
#endif


#endif /* _KERNEL_ARCH_RISCV64_THREAD_H */
