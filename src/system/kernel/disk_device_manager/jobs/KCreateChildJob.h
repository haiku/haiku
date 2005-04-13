// KCreateChildJob.h

#ifndef _K_DISK_DEVICE_CREATE_CHILD_JOB_H
#define _K_DISK_DEVICE_CREATE_CHILD_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

class KCreateChildJob : public KDiskDeviceJob {
public:
	KCreateChildJob(partition_id partition, partition_id child, off_t offset,
					off_t size, const char *parameters);
	virtual ~KCreateChildJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KCreateChildJob;

#endif	// _DISK_DEVICE_CREATE_CHILD_JOB_H
