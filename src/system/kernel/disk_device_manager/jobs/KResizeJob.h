/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _K_DISK_DEVICE_RESIZE_JOB_H
#define _K_DISK_DEVICE_RESIZE_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

/**
 * Resizes a partition.
 */
class KResizeJob : public KDiskDeviceJob {
public:
	KResizeJob(partition_id parentID, partition_id partitionID, off_t size);
	virtual ~KResizeJob();

	virtual status_t Do();

private:
	off_t	fSize;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KResizeJob;

#endif	// _DISK_DEVICE_RESIZE_JOB_H
