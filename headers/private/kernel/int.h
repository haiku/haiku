/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_INT_H
#define _KERNEL_INT_H

#include <stage2.h>
#include <arch/int.h>
#include <KernelExport.h>

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

int  int_init(kernel_args *ka);
int  int_init2(kernel_args *ka);
int  int_io_interrupt_handler(int vector);
long install_interrupt_handler(long, interrupt_handler,	void *);
long remove_interrupt_handler (long, interrupt_handler,	void *);

#define enable_interrupts		  arch_int_enable_interrupts
#define are_interrupts_enabled    arch_int_is_interrupts_enabled

/** @fn long install_io_interrupt_handler(long interrupt, interrupt_handler handler, void *data, ulong flags);
 *
 * @note This function is used for devices that can generate an "actual"
 *       interrupt, i.e. where IRQ < 16
 */
long 	install_io_interrupt_handler(long, 
                                     interrupt_handler, 
                                     void *, ulong);

/** @fn long remove_io_interrupt_handler(long interrupt, interrupt_handler handler, void *data);
 *
 * @note This function is used for devices that can generate an "actual"
 *       interrupt, i.e. where IRQ < 16
 */
long 	remove_io_interrupt_handler (long,
                                     interrupt_handler,
                                     void *);

/** @} */

/* during kernel startup, interrupts are disabled */
extern bool kernel_startup;

#endif /* _KERNEL_INT_H */
