// ramfs_ioctl.h

#ifndef RAMFS_IOCTL_H
#define RAMFS_IOCTL_H

#include <Drivers.h>

#define RAMFS_IOCTL_BASE	(B_DEVICE_OP_CODES_END + 10001)

enum {
	RAMFS_IOCTL_GET_ALLOCATION_INFO		= RAMFS_IOCTL_BASE,	// AllocationInfo*
	RAMFS_IOCTL_DUMP_INDEX,									// const char* name
};

#endif	// RAMFS_IOCTL_H
