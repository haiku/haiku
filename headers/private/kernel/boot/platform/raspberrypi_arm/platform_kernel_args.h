/*
 * Copyright 2009-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_PI_KERNEL_ARGS_H
#define KERNEL_BOOT_PLATFORM_PI_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

// must match SMP_MAX_CPUS in arch_smp.h
#define MAX_BOOT_CPUS 1
#define MAX_PHYSICAL_MEMORY_RANGE 4
#define MAX_PHYSICAL_ALLOCATED_RANGE 8
#define MAX_VIRTUAL_ALLOCATED_RANGE 32


typedef struct {
	char dummy;
} platform_kernel_args;

#endif	/* KERNEL_BOOT_PLATFORM_PI_KERNEL_ARGS_H */
