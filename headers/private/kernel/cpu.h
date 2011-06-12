/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_CPU_H
#define _KERNEL_CPU_H


#include <setjmp.h>

#include <smp.h>
#include <timer.h>
#include <arch/cpu.h>


// define PAUSE, if not done in arch/cpu.h
#ifndef PAUSE
#	define PAUSE()
#endif


struct kernel_args;

namespace BKernel {
	struct Thread;
}

using BKernel::Thread;


/* CPU local data structure */

typedef struct cpu_ent {
	int				cpu_num;

	// thread.c: used to force a reschedule at quantum expiration time
	int				preempted;
	timer			quantum_timer;

	// keeping track of CPU activity
	bigtime_t		active_time;
	bigtime_t		last_kernel_time;
	bigtime_t		last_user_time;

	// used in the kernel debugger
	addr_t			fault_handler;
	addr_t			fault_handler_stack_pointer;
	jmp_buf			fault_jump_buffer;

	Thread*			running_thread;
	Thread*			previous_thread;
	bool			invoke_scheduler;
	bool			invoke_scheduler_if_idle;
	bool			disabled;

	// arch-specific stuff
	arch_cpu_info arch;
} cpu_ent __attribute__((aligned(64)));


//extern cpu_ent gCPU[MAX_BOOT_CPUS];
extern cpu_ent gCPU[];


#ifdef __cplusplus
extern "C" {
#endif

status_t cpu_preboot_init_percpu(struct kernel_args *args, int curr_cpu);
status_t cpu_init(struct kernel_args *args);
status_t cpu_init_percpu(struct kernel_args *ka, int curr_cpu);
status_t cpu_init_post_vm(struct kernel_args *args);
status_t cpu_init_post_modules(struct kernel_args *args);
bigtime_t cpu_get_active_time(int32 cpu);

cpu_ent *get_cpu_struct(void);
extern inline cpu_ent *get_cpu_struct(void) { return &gCPU[smp_get_current_cpu()]; }

void _user_clear_caches(void *address, size_t length, uint32 flags);
bool _user_cpu_enabled(int32 cpu);
status_t _user_set_cpu_enabled(int32 cpu, bool enabled);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_CPU_H */
