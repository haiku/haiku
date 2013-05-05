/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_BOOT_DRIVER_SETTINGS_H
#define KERNEL_BOOT_DRIVER_SETTINGS_H


#include <util/FixedWidthPointer.h>
#include <util/list.h>


struct driver_settings_file {
	FixedWidthPointer<struct driver_settings_file> next;
	char	name[B_OS_NAME_LENGTH];
	FixedWidthPointer<char> buffer;
	uint32	size;
} _PACKED;

#endif	/* KERNEL_BOOT_DRIVER_SETTINGS_H */
