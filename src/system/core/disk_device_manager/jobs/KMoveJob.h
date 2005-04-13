// KMoveJob.h

#ifndef _K_DISK_DEVICE_MOVE_JOB_H
#define _K_DISK_DEVICE_MOVE_JOB_H

#include "KDiskDeviceJob.h"

namespace BPrivate {
namespace DiskDevice {

class KMoveJob : public KDiskDeviceJob {
public:
	KMoveJob(partition_id parent, partition_id partition, off_t offset);
	virtual ~KMoveJob();

	virtual status_t Do();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KMoveJob;

#endif	// _DISK_DEVICE_MOVE_JOB_H
