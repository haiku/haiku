/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ARCH_x86_THREAD_H
#define _KERNEL_ARCH_x86_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <arch/cpu.h>


void i386_push_iframe(struct thread *t, struct iframe *frame);
void i386_pop_iframe(struct thread *t);

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

