// KDiskDeviceJobQueue.h

#ifndef _K_DISK_DEVICE_JOB_QUEUE_H
#define _K_DISK_DEVICE_JOB_QUEUE_H

#include "disk_device_manager.h"

namespace BPrivate {
namespace DiskDevice {

class KDiskDevice;
class KDiskDeviceJob;

class KDiskDeviceJobQueue {
public:
	KDiskDeviceJobQueue();
	~KDiskDeviceJobQueue();

	void SetDevice(KDiskDevice *device);
	KDiskDevice *Device() const;

	KDiskDeviceJob *ActiveJob() const;	// is not in list of scheduled jobs

	// list of scheduled jobs
	bool AddJob(KDiskDeviceJob *job);
	KDiskDeviceJob *RemoveJob(int32 index);
	bool RemoveJob(KDiskDeviceJob *job);
	KDiskDeviceJob *JobAt(int32 index) const;
	int32 CountJobs() const;

	void Execute();
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceJobQueue;

#endif	// _K_DISK_DEVICE_JOB_QUEUE_H
