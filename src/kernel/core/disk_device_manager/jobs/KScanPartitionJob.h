// KScanPartitionJob.h

#ifndef _K_DISK_DEVICE_SCAN_PARTITION_JOB_H
#define _K_DISK_DEVICE_SCAN_PARTITION_JOB_H

namespace BPrivate {
namespace DiskDevice {

#include "KDiskDeviceJob.h"

class KScanPartitionJob : public KDiskDeviceJob {
public:
	KScanPartitionJob(partition_id partition);
	virtual ~KScanPartitionJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KScanPartitionJob;

#endif	// _DISK_DEVICE_SCAN_PARTITION_JOB_H
