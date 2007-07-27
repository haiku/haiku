// KScanPartitionJob.h

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
	/**
	 * Creates the job.
	 * 
	 * \param partitionID the partition to scan
	 */
	KScanPartitionJob(partition_id partitionID);
	virtual ~KScanPartitionJob();

	virtual status_t Do();

private:
	/**
	 * Do the actual scan.
	 */
	status_t _ScanPartition(KPartition *partition);
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KScanPartitionJob;

#endif	// _DISK_DEVICE_SCAN_PARTITION_JOB_H
