/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef PLATFORM_KERNEL_ARGS_H
#define PLATFORM_KERNEL_ARGS_H

// must match SMP_MAX_CPUS in arch_smp.h
#define MAX_BOOT_CPUS 4
#define MAX_PHYSICAL_MEMORY_RANGE 4
#define MAX_PHYSICAL_ALLOCATED_RANGE 4
#define MAX_VIRTUAL_ALLOCATED_RANGE 4

struct platform_kernel_args {
	/* they are just empty! */
};

#endif	/* PLATFORM_KERNEL_ARGS_H */
