/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_PLATFORM_BIOS_IA32_BIOS_DRIVE_H
#define KERNEL_BOOT_PLATFORM_BIOS_IA32_BIOS_DRIVE_H


#include <boot/disk_identifier.h>


typedef struct bios_drive {
	struct bios_drive	*next;
	uint16				drive_number;
	disk_identifier		identifier;
} bios_drive;

#endif	/* KERNEL_BOOT_PLATFORM_BIOS_IA32_BIOS_DRIVE_H */
