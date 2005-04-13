// KRepairJob.h

#ifndef _K_DISK_DEVICE_REPAIR_JOB_H
#define _K_DISK_DEVICE_REPAIR_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

class KRepairJob : public KDiskDeviceJob {
public:
	KRepairJob(partition_id partition);
	virtual ~KRepairJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KRepairJob;

#endif	// _DISK_DEVICE_REPAIR_JOB_H
