/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_BOOT_DEVICE_H
#define _KERNEL_BOOT_DEVICE_H


#include <sys/types.h>


extern dev_t gBootDevice;
extern bool gReadOnlyBootDevice;
	// defined in fs/vfs_boot.cpp

#endif	/* _KERNEL_BOOT_DEVICE_H */
