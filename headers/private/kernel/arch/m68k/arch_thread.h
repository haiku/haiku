/*
 * Copyright 2003-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _KERNEL_ARCH_M68K_THREAD_H
#define _KERNEL_ARCH_M68K_THREAD_H

#include <arch/cpu.h>
#include <arch/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

void m68k_push_iframe(struct iframe_stack *stack, struct iframe *frame);
void m68k_pop_iframe(struct iframe_stack *stack);
struct iframe *m68k_get_user_iframe(void);

uint32 m68k_next_page_directory(Thread *from, Thread *to);

/* as we won't support SMP on m68k (yet?) we can use a global here */
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

#if 0
/* this would only work on 030... */

extern inline Thread *
arch_thread_get_current_thread(void)
{
	uint64 v = 0;
	asm volatile("pmove %%srp,(%0)" : : "a"(&v));
	return (Thread *)(uint32)(v & 0xffffffff);
}


extern inline void
arch_thread_set_current_thread(Thread *t)
{
	uint64 v;
	asm volatile("pmove %%srp,(%0)\n" \
			"move %1,(4,%0)\n" \
			"pmove (%0),%%srp" : : "a"(&v), "d"(t));
}
#endif

#ifdef __cplusplus
}
#endif


#endif /* _KERNEL_ARCH_M68K_THREAD_H */
