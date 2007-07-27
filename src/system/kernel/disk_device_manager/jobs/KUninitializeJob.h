/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _K_DISK_DEVICE_UNINITIALIZE_JOB_H
#define _K_DISK_DEVICE_UNINITIALIZE_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

/**
 * Uninitializes the partition/device
 */
class KUninitializeJob : public KDiskDeviceJob {
public:
	KUninitializeJob(partition_id partitionID);
	virtual ~KUninitializeJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KUninitializeJob;

#endif	// _K_DISK_DEVICE_UNINITIALIZE_JOB_H
