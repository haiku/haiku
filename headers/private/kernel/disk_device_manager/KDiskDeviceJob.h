// KDiskDeviceJob.h

#ifndef _K_DISK_DEVICE_JOB_H
#define _K_DISK_DEVICE_JOB_H

#include "disk_device_manager.h"

struct user_disk_device_job_info;

namespace BPrivate {
namespace DiskDevice {

class KDiskDeviceJobQueue;

class KDiskDeviceJob {
public:
	KDiskDeviceJob(uint32 type, partition_id partitionID,
				   partition_id scopeID = -1);
	virtual ~KDiskDeviceJob();

	disk_job_id ID() const;

	void SetJobQueue(KDiskDeviceJobQueue *queue);
	KDiskDeviceJobQueue *JobQueue() const;

	uint32 Type() const;

	void SetStatus(uint32 status);
	uint32 Status() const;

	status_t SetDescription(const char *description);
	const char *Description() const;

	partition_id PartitionID() const;
	partition_id ScopeID() const;

	status_t SetErrorMessage(const char *message);
	const char *ErrorMessage() const;

	void SetTaskCount(int32 count);
	int32 TaskCount() const;

	void SetCompletedTasks(int32 count);
	int32 CompletedTasks() const;

	void SetInterruptProperties(uint32 properties);
	uint32 InterruptProperties() const;

	virtual void GetTaskDescription(char *description);

	void UpdateProgress(float progress);
		// may trigger a notification
	void UpdateExtraProgress(const char *info);
		// triggers a notification
	float Progress() const;

	status_t GetInfo(user_disk_device_job_info *info);
	status_t GetProgressInfo(disk_device_job_progress_info *info);

	virtual status_t Do() = 0;

private:
	static disk_job_id _NextID();

	disk_job_id			fID;
	KDiskDeviceJobQueue	*fJobQueue;
	uint32				fType;
	uint32				fStatus;
	partition_id		fPartitionID;
	partition_id		fScopeID;
	char				*fDescription;
	char				*fErrorMessage;
	int32				fTaskCount;
	int32				fCompletedTasks;
	uint32				fInterruptProperties;
	float				fProgress;

	static disk_job_id	fNextID;
};

} // namespace DiskDevice
} // namespace BPrivate

using BPrivate::DiskDevice::KDiskDeviceJob;

#endif	// _DISK_DEVICE_JOB_H
