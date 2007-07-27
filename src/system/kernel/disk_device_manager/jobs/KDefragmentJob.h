/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */
#ifndef _K_DISK_DEVICE_DEFRAGMENT_JOB_H
#define _K_DISK_DEVICE_DEFRAGMENT_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

/**
 * Defragments the device
 */
class KDefragmentJob : public KDiskDeviceJob {
public:
	KDefragmentJob(partition_id partition);
	virtual ~KDefragmentJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDefragmentJob;

#endif	// _DISK_DEVICE_DEFRAGMENT_JOB_H
