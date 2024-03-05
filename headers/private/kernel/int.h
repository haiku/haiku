/*
 * Copyright 2003-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_INT_H
#define _KERNEL_INT_H


#include <KernelExport.h>
#include <arch/int.h>

#include <util/list.h>

// private install_io_interrupt_handler() flags
#define B_NO_LOCK_VECTOR	0x100
#define B_NO_HANDLED_INFO	0x200

struct kernel_args;


enum interrupt_type {
	INTERRUPT_TYPE_EXCEPTION,
	INTERRUPT_TYPE_IRQ,
	INTERRUPT_TYPE_LOCAL_IRQ,
	INTERRUPT_TYPE_SYSCALL,
	INTERRUPT_TYPE_ICI,
	INTERRUPT_TYPE_UNKNOWN
};

struct irq_assignment {
	list_link	link;

	uint32		irq;
	uint32		count;

	int32		handlers_count;

	int32		load;
	int32		cpu;
};


#ifdef __cplusplus
extern "C" {
#endif

status_t int_init(struct kernel_args* args);
status_t int_init_post_vm(struct kernel_args* args);
status_t int_init_io(struct kernel_args* args);
status_t int_init_post_device_manager(struct kernel_args* args);
int int_io_interrupt_handler(int vector, bool levelTriggered);

bool interrupts_enabled(void);

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


// map those directly to the arch versions, so they can be inlined
#define disable_interrupts()		arch_int_disable_interrupts()
#define restore_interrupts(status)	arch_int_restore_interrupts(status)


status_t reserve_io_interrupt_vectors(int32 count, int32 startVector,
	enum interrupt_type type);
status_t allocate_io_interrupt_vectors(int32 count, int32 *startVector,
	enum interrupt_type type);
void free_io_interrupt_vectors(int32 count, int32 startVector);

void assign_io_interrupt_to_cpu(int32 vector, int32 cpu);

#endif /* _KERNEL_INT_H */
