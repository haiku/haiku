/*
 * Copyright 2009 Jonas Sundström, jonas@kirilla.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_BOOT_PLATFORM_ROUTERBOARD_MIPSEL_KERNEL_ARGS_H
#define _KERNEL_BOOT_PLATFORM_ROUTERBOARD_MIPSEL_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

#warning IMPLEMENT platform_kernel_args.h

// must match SMP_MAX_CPUS in arch_smp.h
#define MAX_BOOT_CPUS 1
#define MAX_PHYSICAL_MEMORY_RANGE 4
#define MAX_PHYSICAL_ALLOCATED_RANGE 8
#define MAX_VIRTUAL_ALLOCATED_RANGE 32


typedef struct {
	void*	whateverPointer;
	char	whateverPath[128];
} platform_kernel_args;

#endif /* _KERNEL_BOOT_PLATFORM_ROUTERBOARD_MIPSEL_KERNEL_ARGS_H */

