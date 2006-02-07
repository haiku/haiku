/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_ARCH_INT_H
#define KERNEL_ARCH_INT_H


#include <boot/kernel_args.h>
#include <arch_int.h>


#ifdef __cplusplus
extern "C" {
#endif

status_t arch_int_init(kernel_args *args);
status_t arch_int_init_post_vm(kernel_args *args);
status_t arch_int_init_post_device_manager(struct kernel_args *args);

void arch_int_enable_interrupts(void);
int arch_int_disable_interrupts(void);
void arch_int_restore_interrupts(int oldstate);
void arch_int_enable_io_interrupt(int irq);
void arch_int_disable_io_interrupt(int irq);
bool arch_int_are_interrupts_enabled(void);

#ifdef __cplusplus
}
#endif

#endif	/* KERNEL_ARCH_INT_H */
