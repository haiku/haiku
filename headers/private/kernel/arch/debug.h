/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_ARCH_DEBUG_H
#define _KERNEL_ARCH_DEBUG_H

int arch_dbg_init(kernel_args *ka);
void arch_dbg_save_registers(int *);

#endif	/* _KERNEL_ARCH_DEBUG_H */
