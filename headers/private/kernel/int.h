/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_INT_H
#define _KERNEL_INT_H

#include <stage2.h>
#include <arch/int.h>

int int_init(kernel_args *ka);
int int_init2(kernel_args *ka);
int int_io_interrupt_handler(int vector);
int int_set_io_interrupt_handler(int vector, int (*func)(void*), void* data);
int int_remove_io_interrupt_handler(int vector, int (*func)(void*), void* data);

#define int_enable_interrupts	arch_int_enable_interrupts
#define int_disable_interrupts	arch_int_disable_interrupts
#define int_restore_interrupts	arch_int_restore_interrupts
#define int_is_interrupts_enabled arch_int_is_interrupts_enabled

enum {
	INT_NO_RESCHEDULE,
	INT_RESCHEDULE
};

#endif

