/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_INT_H
#define _KERNEL_INT_H

#include <stage2.h>
#include <arch/int.h>

/**
 * @defgroup kernelint Interrupts
 * @ingroup OpenBeOS_Kernel
 * @brief Interrupts for the kernel and device drivers
 * @{
 */

/** 
 * @defgroup Intreturns Interrupt Handler return codes
 * @ingroup kernelint
 * @{
 */
/** @def B_UNHANDLED_INTERRUPT interrupt wasn't handled by this handler */
/** @def B_HANDLED_INTERRUPT the handler handled the interrupt */
/** @def B_INVOKE_SCHEDULER the handler handled the interrupt and wants the
 *  scheduler invoked
 */
#define B_UNHANDLED_INTERRUPT		0
#define B_HANDLED_INTERRUPT			1
#define B_INVOKE_SCHEDULER			2
/** @} */

/** @def B_NO_ENABLE_COUNTER add the handler but don't change whether or
 * not the interrupt is currently enabled
 */
#define B_NO_ENABLE_COUNTER         1

typedef int32 (*interrupt_handler) (void *data);

int int_init(kernel_args *ka);
int int_init2(kernel_args *ka);
int int_io_interrupt_handler(int vector);
long install_interrupt_handler(long, interrupt_handler,	void *);
long remove_interrupt_handler (long, interrupt_handler,	void *);

#define int_enable_interrupts	  arch_int_enable_interrupts
#define int_disable_interrupts	  arch_int_disable_interrupts
#define int_restore_interrupts	  arch_int_restore_interrupts
#define int_is_interrupts_enabled arch_int_is_interrupts_enabled

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

/** @} */

#endif /* _KERNEL_INT_H */
