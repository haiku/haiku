// KInitializeJob.h

#ifndef _K_DISK_DEVICE_INITIALIZE_JOB_H
#define _K_DISK_DEVICE_INITIALIZE_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

class KInitializeJob : public KDiskDeviceJob {
public:
	KInitializeJob(partition_id partition, disk_system_id diskSystemID,
				   const char *parameters);
	virtual ~KInitializeJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KInitializeJob;

#endif	// _DISK_DEVICE_INITIALIZE_JOB_H
