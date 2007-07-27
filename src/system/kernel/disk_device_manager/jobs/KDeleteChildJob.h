/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */
#ifndef _K_DISK_DEVICE_DELETE_CHILD_JOB_H
#define _K_DISK_DEVICE_DELETE_CHILD_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

/**
 * Deletes specific child of a partition/device
 */
class KDeleteChildJob : public KDiskDeviceJob {
public:
	KDeleteChildJob(partition_id parent, partition_id partition);
	virtual ~KDeleteChildJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDeleteChildJob;

#endif	// _DISK_DEVICE_DELETE_CHILD_JOB_H
