/*
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_BIOS_IA32_KERNEL_ARGS_H
#define KERNEL_BOOT_PLATFORM_BIOS_IA32_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif


#include <apm.h>
#include <bios_drive.h>


// must match SMP_MAX_CPUS in arch_smp.h
#define MAX_BOOT_CPUS 8
#define MAX_PHYSICAL_MEMORY_RANGE 32
#define MAX_PHYSICAL_ALLOCATED_RANGE 32
#define MAX_VIRTUAL_ALLOCATED_RANGE 32

#define MAX_SERIAL_PORTS 4

typedef struct {
	uint16		serial_base_ports[MAX_SERIAL_PORTS];

	bios_drive	*drives;	// this does not contain the boot drive

	apm_info	apm;
} platform_kernel_args;

#endif	/* KERNEL_BOOT_PLATFORM_BIOS_IA32_KERNEL_ARGS_H */
