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

#define int_enable_interrupts	  arch_int_enable_interrupts
#define int_disable_interrupts	  arch_int_disable_interrupts
#define int_restore_interrupts	  arch_int_restore_interrupts
#define int_is_interrupts_enabled arch_int_is_interrupts_enabled

#define B_UNHANDLED_INTERRUPT		0		/* pass to next handler */
#define B_HANDLED_INTERRUPT			1		/* don't pass on */
#define B_INVOKE_SCHEDULER			2		/* don't pass on; invoke the scheduler */

#define B_NO_ENABLE_COUNTER         1

typedef int32 (*interrupt_handler) (void *data);

/* interrupt handling support for device drivers */

long 	install_io_interrupt_handler (
	long 				interrupt_number, 
	interrupt_handler	handler, 
	void				*data, 
	ulong 				flags
);

long 	remove_io_interrupt_handler (
	long 				interrupt_number,
	interrupt_handler	handler,
	void				*data
);

#endif /* _KERNEL_INT_H */
