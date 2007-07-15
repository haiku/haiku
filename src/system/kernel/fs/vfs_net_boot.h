/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _VFS_NET_BOOT_H
#define _VFS_NET_BOOT_H


#include "vfs_boot.h"


int compare_image_boot(const void *_a, const void *_b);


class NetBootMethod : public BootMethod {
public:
	NetBootMethod(const KMessage& bootVolume, int32 method);
	virtual ~NetBootMethod();

	virtual status_t Init();

	virtual bool IsBootDevice(KDiskDevice* device, bool strict);
	virtual bool IsBootPartition(KPartition* partition, bool& foundForSure);
	virtual void SortPartitions(KPartition** partitions, int32 count);
};


#endif	// _VFS_NET_BOOT_H
