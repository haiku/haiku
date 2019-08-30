/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef RAMFS_IOCTL_H
#define RAMFS_IOCTL_H

#include <Drivers.h>

#define RAMFS_IOCTL_BASE	(B_DEVICE_OP_CODES_END + 10001)

enum {
	RAMFS_IOCTL_GET_ALLOCATION_INFO		= RAMFS_IOCTL_BASE,	// AllocationInfo*
	RAMFS_IOCTL_DUMP_INDEX,									// const char* name
};

#endif	// RAMFS_IOCTL_H
