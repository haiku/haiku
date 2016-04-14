/*
 * Copyright 2013-2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_EFI_KERNEL_ARGS_H
#define KERNEL_BOOT_PLATFORM_EFI_KERNEL_ARGS_H


#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

// currently the EFI loader pretends to be the bios_ia32 platform.
// not quite right, as the kernel needs to be aware of efi runtime services

#include <arch/x86/apm.h>
#include <boot/disk_identifier.h>
#include <util/FixedWidthPointer.h>


#define SMP_MAX_CPUS 64

#define MAX_PHYSICAL_MEMORY_RANGE 32
#define MAX_PHYSICAL_ALLOCATED_RANGE 32
#define MAX_VIRTUAL_ALLOCATED_RANGE 32

#define MAX_SERIAL_PORTS 4

typedef struct bios_drive {
	struct bios_drive	*next;
	uint16				drive_number;
	disk_identifier		identifier;
} bios_drive;

typedef struct {
	uint16		serial_base_ports[MAX_SERIAL_PORTS];

	FixedWidthPointer<bios_drive> drives;
		// this does not contain the boot drive
	// seems to be ignored entirely?

	apm_info	apm;
} _PACKED platform_kernel_args;


#endif	/* KERNEL_BOOT_PLATFORM_BIOS_IA32_KERNEL_ARGS_H */
