/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_INT_H
#define _KERNEL_INT_H

#include <KernelExport.h>

#include <arch/int.h>

struct kernel_args;

/* adds the handler but don't change whether or not the interrupt is currently enabled */
#define B_NO_ENABLE_COUNTER	1

int  int_init(struct kernel_args *ka);
int  int_init2(struct kernel_args *ka);
int  int_io_interrupt_handler(int vector);
long install_interrupt_handler(long, interrupt_handler,	void *);
long remove_interrupt_handler (long, interrupt_handler,	void *);

#define enable_interrupts		  arch_int_enable_interrupts
#define are_interrupts_enabled    arch_int_is_interrupts_enabled

/* during kernel startup, interrupts are disabled */
extern bool kernel_startup;

#endif /* _KERNEL_INT_H */
