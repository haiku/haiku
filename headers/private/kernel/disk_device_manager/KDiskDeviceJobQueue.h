// KDiskDeviceJobQueue.h

#ifndef _K_DISK_DEVICE_JOB_QUEUE_H
#define _K_DISK_DEVICE_JOB_QUEUE_H

#include <OS.h>

#include "disk_device_manager.h"

namespace BPrivate {
namespace DiskDevice {

class KDiskDevice;
class KDiskDeviceJob;

class KDiskDeviceJobQueue {
public:
	KDiskDeviceJobQueue();
	~KDiskDeviceJobQueue();

	status_t InitCheck() const;

	void SetDevice(KDiskDevice *device);
	KDiskDevice *Device() const;

	int32 ActiveJobIndex() const;
	KDiskDeviceJob *ActiveJob() const;

	// list of scheduled jobs
	bool AddJob(KDiskDeviceJob *job);
	KDiskDeviceJob *RemoveJob(int32 index);
	bool RemoveJob(KDiskDeviceJob *job);
		// Adding/removing is only possible before the queues is added to the
		// manager.
	KDiskDeviceJob *JobAt(int32 index) const;
	int32 CountJobs() const;

	status_t Execute();
		// Called by the manager, when the queue is added to it.
	// manager must be locked
	status_t Cancel(bool reverse);
	status_t Pause();
	status_t Continue();
	sem_id ReadyToPause();
		// called only by the queue's thread from within a module hook

	bool IsExecuting() const;
	bool IsCanceled() const;
	bool ShallReverse() const;
	bool IsPaused() const;
	bool IsPauseRequested() const;

private:
	struct JobQueue;

	int32 _ThreadLoop();
	static int32 _ThreadEntry(void *data);

	KDiskDevice		*fDevice;
	int32			fActiveJob;
	JobQueue		*fJobs;
	uint32			fFlags;
	thread_id		fThread;
	sem_id			fSyncSemaphore;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceJobQueue;

#endif	// _K_DISK_DEVICE_JOB_QUEUE_H
