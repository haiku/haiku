/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_DEBUG_H
#define _KERNEL_ARCH_DEBUG_H


#include <SupportDefs.h>

struct kernel_args;


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_dbg_init(kernel_args *args);
void *arch_get_caller(void);
void arch_dbg_save_registers(int *);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_ARCH_DEBUG_H */
