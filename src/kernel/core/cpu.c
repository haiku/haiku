/* This file contains the cpu functions (init, etc). */

/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

// ToDo: fix the atomic_*() functions and possibly move them elsewhere;
//	they are exported in libroot.so anyway, so the best place would
//	probably there.


#include <kernel.h>
#include <cpu.h>
#include <vm.h>
#include <arch/cpu.h>
#include <boot/kernel_args.h>

#include <string.h>

/* global per-cpu structure */
cpu_ent cpu[MAX_BOOT_CPUS];


int
cpu_init(kernel_args *ka)
{
	int i;

	memset(cpu, 0, sizeof(cpu));
	for(i = 0; i < MAX_BOOT_CPUS; i++) {
		cpu[i].info.cpu_num = i;
	}

	return arch_cpu_init(ka);
}


int
cpu_preboot_init(kernel_args *ka)
{
	return arch_cpu_preboot_init(ka);
}
