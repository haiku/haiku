// KRepairJob.h

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
	/**
	 * Creates the job.
	 * 
	 * \param partition the partition which should be repared
	 * \param checkOnly when true, the partition is only checked, but no actual
	 * 		repairs are proceeded
	 */
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
