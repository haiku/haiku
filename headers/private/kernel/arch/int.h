/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_ARCH_INT_H
#define _NEWOS_KERNEL_ARCH_INT_H

#include <ktypes.h>
#include <stage2.h>

int arch_int_init(kernel_args *ka);
int arch_int_init2(kernel_args *ka);

void arch_int_enable_interrupts(void);
int arch_int_disable_interrupts(void);
void arch_int_restore_interrupts(int oldstate);
void arch_int_enable_io_interrupt(int irq);
void arch_int_disable_io_interrupt(int irq);
bool arch_int_is_interrupts_enabled(void);

#endif

