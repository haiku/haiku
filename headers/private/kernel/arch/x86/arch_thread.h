/*
 * Copyright 2002-2005, The Haiku Team. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_THREAD_H
#define _KERNEL_ARCH_x86_THREAD_H


#include <arch/cpu.h>


#ifdef __cplusplus
extern "C" {
#endif

void x86_push_iframe(struct iframe_stack *stack, struct iframe *frame);
void x86_pop_iframe(struct iframe_stack *stack);
struct iframe *i386_get_user_iframe(void);

void i386_return_from_signal();
void i386_end_return_from_signal();


static
inline struct thread *
arch_thread_get_current_thread(void)
{
	struct thread *t;
	read_dr3(t);
	return t;
}

static inline void
arch_thread_set_current_thread(struct thread *t)
{
	write_dr3(t);
}

#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_ARCH_x86_THREAD_H */

