/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */
#ifndef _K_DISK_DEVICE_CREATE_CHILD_JOB_H
#define _K_DISK_DEVICE_CREATE_CHILD_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

/**
 * Create new child of given partition
 */
class KCreateChildJob : public KDiskDeviceJob {
public:
	KCreateChildJob(partition_id partition, partition_id child, off_t offset,
		off_t size, const char *type, const char *parameters);
	virtual ~KCreateChildJob();

	virtual status_t Do();
	
private:
	partition_id	fChildID;
	off_t			fOffset;
	off_t			fSize;
	char*			fType;
	char*			fParameters;
	
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KCreateChildJob;

#endif	// _DISK_DEVICE_CREATE_CHILD_JOB_H
