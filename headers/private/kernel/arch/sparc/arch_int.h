/*
 * Copyright 2005-2021, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 * 		Adrien Destugues <pulkomandy@pulkomandy.tk>
 */
#ifndef _KERNEL_ARCH_SPARC_INT_H
#define _KERNEL_ARCH_SPARC_INT_H

#include <SupportDefs.h>

#define NUM_IO_VECTORS	256

static inline void
arch_int_enable_interrupts_inline(void)
{
	int tmp;
	asm volatile(
		"rdpr %%pstate, %0\n"
		"or %0, 2, %0\n"
		"wrpr %0, %%pstate\n"
		: "=r" (tmp)
	);
}


static inline int
arch_int_disable_interrupts_inline(void)
{
	int flags;
	int tmp;
	asm volatile(
		"rdpr %%pstate, %0\n"
		"andn %0, 2, %1\n"
		"wrpr %1, %%pstate\n"
		: "=r" (flags), "=r" (tmp)
	);
	return flags & 2;
}


static inline void
arch_int_restore_interrupts_inline(int oldState)
{
	if (oldState)
		arch_int_enable_interrupts_inline();
}


static inline bool
arch_int_are_interrupts_enabled_inline(void)
{
	int flags;
	asm volatile(
		"rdpr %%pstate, %0\n"
		: "=r" (flags)
	);

	return flags & 2;
}


// map the functions to the inline versions
#define arch_int_enable_interrupts()	arch_int_enable_interrupts_inline()
#define arch_int_disable_interrupts()	arch_int_disable_interrupts_inline()
#define arch_int_restore_interrupts(status)	\
	arch_int_restore_interrupts_inline(status)
#define arch_int_are_interrupts_enabled()	\
	arch_int_are_interrupts_enabled_inline()


#endif /* _KERNEL_ARCH_SPARC_INT_H */
