// KInitializeJob.h

#ifndef _K_DISK_DEVICE_UNINITIALIZE_JOB_H
#define _K_DISK_DEVICE_UNINITIALIZE_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

class KUninitializeJob : public KDiskDeviceJob {
public:
	KUninitializeJob(partition_id partitionID);
	virtual ~KUninitializeJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KUninitializeJob;

#endif	// _K_DISK_DEVICE_UNINITIALIZE_JOB_H
