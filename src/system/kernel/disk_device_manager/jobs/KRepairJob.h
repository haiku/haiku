/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */
#ifndef _K_DISK_DEVICE_REPAIR_JOB_H
#define _K_DISK_DEVICE_REPAIR_JOB_H

#include "KDiskDeviceJob.h"


namespace BPrivate {
namespace DiskDevice {

/**
 * Repair partition (or check for repairs)
 */
class KRepairJob : public KDiskDeviceJob {
public:
	KRepairJob(partition_id partition, bool checkOnly);
	virtual ~KRepairJob();

	virtual status_t Do();
	
private:
	bool fCheckOnly;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KRepairJob;

#endif	// _DISK_DEVICE_REPAIR_JOB_H
