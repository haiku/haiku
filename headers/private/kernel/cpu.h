/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_CPU_H
#define _KERNEL_CPU_H


#include <smp.h>
#include <timer.h>
#include <boot/kernel_args.h>


/* CPU local data structure */

typedef union cpu_ent {
	struct {
		int cpu_num;

		// thread.c: used to force a reschedule at quantum expiration time
		int preempted;
		timer quantum_timer;
		bool disabled;
	} info;
} cpu_ent __attribute__((aligned(64)));


extern cpu_ent gCPU[MAX_BOOT_CPUS];


#ifdef __cplusplus
extern "C" {
#endif

status_t cpu_preboot_init(struct kernel_args *args);
status_t cpu_init(struct kernel_args *args);
status_t cpu_init_post_vm(struct kernel_args *args);
status_t cpu_init_post_modules(struct kernel_args *args);

cpu_ent *get_cpu_struct(void);
extern inline cpu_ent *get_cpu_struct(void) { return &gCPU[smp_get_current_cpu()]; }

void _user_clear_caches(void *address, size_t length, uint32 flags);
bool _user_cpu_enabled(int32 cpu);
status_t _user_set_cpu_enabled(int32 cpu, bool enabled);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_CPU_H */
