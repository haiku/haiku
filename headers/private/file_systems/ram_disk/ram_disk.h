/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PRIVATE_FILE_SYSTEMS_RAM_DISK_RAM_DISK_H
#define _PRIVATE_FILE_SYSTEMS_RAM_DISK_RAM_DISK_H


#include <Drivers.h>
#include <StorageDefs.h>


#define RAM_DISK_CONTROL_DEVICE_NAME	"disk/virtual/ram/control"
#define RAM_DISK_RAW_DEVICE_BASE_NAME	"disk/virtual/ram"


enum {
	RAM_DISK_IOCTL_REGISTER		= B_DEVICE_OP_CODES_END + 1,
	RAM_DISK_IOCTL_UNREGISTER,
	RAM_DISK_IOCTL_FLUSH,
	RAM_DISK_IOCTL_INFO
};


struct ram_disk_ioctl_register {
	uint64	size;
	char	path[B_PATH_NAME_LENGTH];

	// return value
	int32	id;
};


struct ram_disk_ioctl_unregister {
	int32	id;
};


struct ram_disk_ioctl_info {
	// return values
	int32	id;
	uint64	size;
	char	path[B_PATH_NAME_LENGTH];
};


#endif	// _PRIVATE_FILE_SYSTEMS_RAM_DISK_RAM_DISK_H
