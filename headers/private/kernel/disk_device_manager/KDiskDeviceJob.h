// KDiskDeviceJob.h

#ifndef _K_DISK_DEVICE_JOB_H
#define _K_DISK_DEVICE_JOB_H

#include "disk_device_manager.h"

struct user_disk_device_job_info;

namespace BPrivate {
namespace DiskDevice {

class KDiskDeviceJob {
public:
	KDiskDeviceJob(partition_id partition, partition_id scope = -1);
	virtual ~KDiskDeviceJob();

	disk_job_id ID() const;

	void SetStatus(uint32 status);
	uint32 Status() const;

	void SetDescription(const char *description);
	const char *Description() const;
		// Maybe better just a virtual void GetDescription(char*)?

	void SetPartitionID(partition_id partitionID);
		// Probably not needed, since passed to the constructor.
	partition_id PartitionID() const;
	partition_id ScopeID() const;

	virtual void UpdateProgress(float progress);
		// may trigger a notification
		// virtual, since some jobs are composed of several tasks (e.g. Move).
		// We might want to explicitly support subtasks in the base class, or
		// even in the userland API.
	void UpdateExtraProgress(const char *info);
		// triggers a notification
	float Progress() const;

	status_t GetInfo(user_disk_device_job_info *info);

	virtual status_t Do();

	// TODO: Do we want to add a Cancel() here? We probably can't tell the
	// disk system directly anyway. That is, if the job is in progress,
	// the disk system would have to check the job status periodically
	// to find out, if the job had been canceled. But then a
	// SetStatus(B_DISK_DEVICE_JOB_CANCELED) would be sufficient.
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceJob;

#endif	// _DISK_DEVICE_JOB_H
