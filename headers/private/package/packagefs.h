/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _PACKAGE__PRIVATE__PACKAGE_FS_H_
#define _PACKAGE__PRIVATE__PACKAGE_FS_H_


#include <Drivers.h>


enum PackageFSMountType {
	PACKAGE_FS_MOUNT_TYPE_SYSTEM,
	PACKAGE_FS_MOUNT_TYPE_COMMON,
	PACKAGE_FS_MOUNT_TYPE_HOME,
	PACKAGE_FS_MOUNT_TYPE_CUSTOM
};


enum {
	PACKAGE_FS_OPERATION_GET_VOLUME_INFO	= B_DEVICE_OP_CODES_END + 1
};


struct PackageFSVolumeInfo {
	PackageFSMountType	mountType;

	// device and node id of the respective package FS root scope (e.g. "/boot"
	// for the three standard volumes)
	dev_t				rootDeviceID;
	ino_t				rootDirectoryID;
};


#endif	// _PACKAGE__PRIVATE__PACKAGE_FS_H_
