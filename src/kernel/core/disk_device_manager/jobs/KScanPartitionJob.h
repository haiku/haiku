// KScanPartitionJob.h

#ifndef _K_DISK_DEVICE_SCAN_PARTITION_JOB_H
#define _K_DISK_DEVICE_SCAN_PARTITION_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

class KPartition;

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
