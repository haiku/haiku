/*
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_ARCH_INT_H
#define KERNEL_ARCH_INT_H


typedef enum interrupt_trigger_mode {
	B_EDGE_TRIGGERED = 1,
	B_LEVEL_TRIGGERED = 2
} interrupt_trigger_mode;

typedef enum interrupt_trigger_polarity {
	// For B_EDGE_TRIGGERED interrupts
	B_FALLING_EDGE_POLARITY = 1,
	B_RISING_EDGE_POLARITY = 2,

	// For B_LEVEL_TRIGGERED interrupts
	B_LOW_ACTIVE_POLARITY = 1,
	B_HIGH_ACTIVE_POLARITY = 2,
} interrupt_trigger_polarity;


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
void arch_int_configure_io_interrupt(int32 irq, interrupt_trigger_mode mode,
	interrupt_trigger_polarity polarity);
bool arch_int_are_interrupts_enabled(void);
int32 arch_int_assign_to_cpu(int32 irq, int32 cpu);

#ifdef __cplusplus
}
#endif


#include <arch_int.h>


#endif	/* KERNEL_ARCH_INT_H */
