/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ARCH_x86_THREAD_H
#define _KERNEL_ARCH_x86_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <arch/cpu.h>

extern inline struct thread *arch_thread_get_current_thread(void) {
	struct thread *t;
	read_dr3(t);
	return t;
}

extern inline void arch_thread_set_current_thread(struct thread *t) {
	write_dr3(t);
}

#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_ARCH_x86_THREAD_H */

