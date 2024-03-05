/*
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_ARCH_INT_H
#define KERNEL_ARCH_INT_H


// config flags for arch_int_configure_io_interrupt()
#define B_EDGE_TRIGGERED		1
#define B_LEVEL_TRIGGERED		2
#define B_LOW_ACTIVE_POLARITY	4
#define B_HIGH_ACTIVE_POLARITY	8


#ifdef __cplusplus
extern "C" {
#endif

struct kernel_args;

status_t arch_int_init(struct kernel_args* args);
status_t arch_int_init_post_vm(struct kernel_args* args);
status_t arch_int_init_io(struct kernel_args* args);
status_t arch_int_init_post_device_manager(struct kernel_args* args);

void arch_int_enable_interrupts(void);
int arch_int_disable_interrupts(void);
void arch_int_restore_interrupts(int oldState);
void arch_int_enable_io_interrupt(int32 irq);
void arch_int_disable_io_interrupt(int32 irq);
void arch_int_configure_io_interrupt(int32 irq, uint32 config);
bool arch_int_are_interrupts_enabled(void);
int32 arch_int_assign_to_cpu(int32 irq, int32 cpu);

#ifdef __cplusplus
}
#endif


#include <arch_int.h>


#endif	/* KERNEL_ARCH_INT_H */
