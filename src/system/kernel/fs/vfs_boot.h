/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VFS_BOOT_H
#define _VFS_BOOT_H


#include <disk_device_manager/KDiskDevice.h>
#include <util/KMessage.h>


class BootMethod {
public:
	BootMethod(const KMessage& bootVolume, int32 method);
	virtual ~BootMethod();

	virtual status_t Init();

	virtual bool IsBootDevice(KDiskDevice* device, bool strict) = 0;
	virtual bool IsBootPartition(KPartition* partition, bool& foundForSure) = 0;
	virtual void SortPartitions(KPartition** partitions, int32 count) = 0;

protected:
	const KMessage&	fBootVolume;
	int32			fMethod;
};


#endif	// _VFS_BOOT_H
