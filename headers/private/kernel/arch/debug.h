/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_DEBUG_H
#define _KERNEL_ARCH_DEBUG_H


#include <SupportDefs.h>

struct kernel_args;
struct thread;


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_debug_init(kernel_args *args);
void *arch_debug_get_caller(void);
bool arch_debug_contains_call(struct thread *thread, const char *symbol,
	addr_t start, addr_t end);
void arch_debug_save_registers(int *);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_ARCH_DEBUG_H */
