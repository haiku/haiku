/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
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
	} info;
	uint32 align[16];
} cpu_ent;

/**
 * Defined in core/cpu.c
 */
extern cpu_ent cpu[MAX_BOOT_CPUS];

#ifdef __cplusplus
extern "C" {
#endif

int cpu_preboot_init(struct kernel_args *ka);
int cpu_init(struct kernel_args *ka);
cpu_ent *get_cpu_struct(void);

extern inline cpu_ent *get_cpu_struct(void) { return &cpu[smp_get_current_cpu()]; }

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_CPU_H */
