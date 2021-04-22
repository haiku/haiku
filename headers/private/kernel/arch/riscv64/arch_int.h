/*
 * Copyright 2005-2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _KERNEL_ARCH_RISCV64_INT_H
#define _KERNEL_ARCH_RISCV64_INT_H

#include <SupportDefs.h>

#define NUM_IO_VECTORS	256


static inline void
arch_int_enable_interrupts_inline(void)
{
	// TODO: implement
}


static inline int
arch_int_disable_interrupts_inline(void)
{
	// TODO: implement
	return 0;
}


static inline void
arch_int_restore_interrupts_inline(int oldState)
{
	// TODO: implement
}


static inline bool
arch_int_are_interrupts_enabled_inline(void)
{
	// TODO: implement
	return false;
}


// map the functions to the inline versions
#define arch_int_enable_interrupts()	arch_int_enable_interrupts_inline()
#define arch_int_disable_interrupts()	arch_int_disable_interrupts_inline()
#define arch_int_restore_interrupts(status)	\
	arch_int_restore_interrupts_inline(status)
#define arch_int_are_interrupts_enabled()	\
	arch_int_are_interrupts_enabled_inline()


#endif /* _KERNEL_ARCH_RISCV64_INT_H */
