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

/* during kernel startup, interrupts are disabled */
extern bool kernel_startup;


#ifdef __cplusplus
extern "C" {
#endif

status_t int_init(struct kernel_args *args);
status_t int_init_post_vm(struct kernel_args *args);
int int_io_interrupt_handler(int vector);
long install_interrupt_handler(long, interrupt_handler,	void *);
long remove_interrupt_handler (long, interrupt_handler,	void *);

static inline void
enable_interrupts(void)
{
	arch_int_enable_interrupts();
}

static inline bool
are_interrupts_enabled(void)
{
	return arch_int_are_interrupts_enabled();
}

#ifdef __cplusplus
}
#endif

#endif /* _KERNEL_INT_H */
