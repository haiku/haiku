/*
** Copyright 2002-2004, The Haiku Team. All rights reserved.
** Distributed under the terms of the Haiku License.
**
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

/* This file contains the cpu functions (init, etc). */

#include <kernel.h>
#include <cpu.h>
#include <vm.h>
#include <arch/cpu.h>
#include <boot/kernel_args.h>

#include <string.h>


/* global per-cpu structure */
cpu_ent cpu[MAX_BOOT_CPUS];


status_t
cpu_init(kernel_args *args)
{
	int i;

	memset(cpu, 0, sizeof(cpu));
	for (i = 0; i < MAX_BOOT_CPUS; i++) {
		cpu[i].info.cpu_num = i;
	}

	return arch_cpu_init(args);
}


status_t
cpu_init_post_vm(kernel_args *args)
{
	return arch_cpu_init_post_vm(args);
}


status_t
cpu_preboot_init(kernel_args *args)
{
	return arch_cpu_preboot_init(args);
}
