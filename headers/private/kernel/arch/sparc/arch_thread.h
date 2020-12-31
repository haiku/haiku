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

static inline Thread *
arch_thread_get_current_thread(void)
{
    Thread *t;
    asm volatile("mov %%g7, %0" : "=r"(t));
    return t;
}


static inline void
arch_thread_set_current_thread(Thread *t)
{
    asm volatile("mov %0,%%g7" : : "r"(t));
}


#ifdef __cplusplus
}
#endif


#endif /* _KERNEL_ARCH_SPARC_THREAD_H */

