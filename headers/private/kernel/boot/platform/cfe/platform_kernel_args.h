/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the OpenBeOS License.
 */
#ifndef KERNEL_BOOT_PLATFORM_CFE_KERNEL_ARGS_H
#define KERNEL_BOOT_PLATFORM_CFE_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

// must match SMP_MAX_CPUS in arch_smp.h
#define MAX_BOOT_CPUS 4
#define MAX_PHYSICAL_MEMORY_RANGE 4
#define MAX_PHYSICAL_ALLOCATED_RANGE 8
#define MAX_VIRTUAL_ALLOCATED_RANGE 32


typedef struct {
	uint64	cfe_entry;	// pointer but always 64bit
	//XXX:char	rtc_path[128];
} platform_kernel_args;

#endif	/* KERNEL_BOOT_PLATFORM_CFE_KERNEL_ARGS_H */
