// KDefragmentJob.h

#ifndef _K_DISK_DEVICE_DEFRAGMENT_JOB_H
#define _K_DISK_DEVICE_DEFRAGMENT_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

class KDefragmentJob : public KDiskDeviceJob {
public:
	KDefragmentJob(partition_id partition);
	virtual ~KDefragmentJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDefragmentJob;

#endif	// _DISK_DEVICE_DEFRAGMENT_JOB_H
