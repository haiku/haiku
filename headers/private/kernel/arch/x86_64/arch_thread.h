/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_64_THREAD_H
#define _KERNEL_ARCH_X86_64_THREAD_H


#include <arch/cpu.h>


#ifdef __cplusplus
extern "C" {
#endif

static inline Thread *
arch_thread_get_current_thread(void)
{
	return NULL;
}

static inline void
arch_thread_set_current_thread(Thread *t)
{
	
}

#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_ARCH_X86_64_THREAD_H */
