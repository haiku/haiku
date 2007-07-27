/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */
#ifndef _K_DISK_DEVICE_SCAN_PARTITION_JOB_H
#define _K_DISK_DEVICE_SCAN_PARTITION_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

class KPartition;

/**
 * Scans partition and tries to find the most suitable disk system for it
 */
class KScanPartitionJob : public KDiskDeviceJob {
public:
	KScanPartitionJob(partition_id partitionID);
	virtual ~KScanPartitionJob();

	virtual status_t Do();

private:
	status_t _ScanPartition(KPartition *partition);
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KScanPartitionJob;

#endif	// _DISK_DEVICE_SCAN_PARTITION_JOB_H
