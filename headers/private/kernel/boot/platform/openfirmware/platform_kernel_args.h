/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef KERNEL_BOOT_PLATFORM_OPENFIRMWARE_KERNEL_ARGS_H
#define KERNEL_BOOT_PLATFORM_OPENFIRMWARE_KERNEL_ARGS_H


#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

#define SMP_MAX_CPUS 1
	// TODO: Until inline kernel atomic code for ppc is fixed

#define MAX_PHYSICAL_MEMORY_RANGE 4
#define MAX_PHYSICAL_ALLOCATED_RANGE 8
#define MAX_VIRTUAL_ALLOCATED_RANGE 32


typedef struct {
	void	*openfirmware_entry;
	char	rtc_path[128];

	// XXX: HACK: must match the U-Boot platform args
	// FIXME: use a union instead?

	// Flattened Device Tree blob
	void	*fdt;
} platform_kernel_args;

#endif	/* KERNEL_BOOT_PLATFORM_OPENFIRMWARE_KERNEL_ARGS_H */
