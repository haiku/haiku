//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <syscalls.h>

#include <DiskDeviceJob.h>

// constructor
BDiskDeviceJob::BDiskDeviceJob()
	: fID(B_NO_INIT),
	  fType(B_DISK_DEVICE_JOB_BAD_TYPE),
	  fPartitionID(-1),
	  fDescription()
{
}

// destructor
BDiskDeviceJob::~BDiskDeviceJob()
{
}

// InitCheck
status_t
BDiskDeviceJob::InitCheck() const
{
	return (fID >= 0 ? B_OK : fID);
}

// SetTo
status_t
BDiskDeviceJob::SetTo(disk_job_id id)
{
	Unset();
	user_disk_device_job_info info;
	status_t error = _kern_get_disk_device_job_info(id, &info);
	if (error != B_OK)
		return error;
	return _SetTo(&info);
}

// Unset
void
BDiskDeviceJob::Unset()
{
	fID = B_NO_INIT;
	fType = B_DISK_DEVICE_JOB_BAD_TYPE;
	fPartitionID = -1;
	fDescription.SetTo(NULL);
}

// ID
disk_job_id
BDiskDeviceJob::ID() const
{
	return fID;
}

// Type
uint32
BDiskDeviceJob::Type() const
{
	return fType;
}

// PartitionID
partition_id
BDiskDeviceJob::PartitionID() const
{
	return fPartitionID;
}

// Description
const char *
BDiskDeviceJob::Description() const
{
	return fDescription.String();
}

// Status
uint32
BDiskDeviceJob::Status() const
{
	if (InitCheck() != B_OK)
		return B_DISK_DEVICE_JOB_UNINITIALIZED;
	disk_device_job_progress_info info;
	status_t error = _kern_get_disk_device_job_progress_info(fID, &info);
	if (error != B_OK)
		return B_DISK_DEVICE_JOB_UNINITIALIZED;
	return info.status;
}

// Progress
float
BDiskDeviceJob::Progress() const
{
	if (InitCheck() != B_OK)
		return 0;
	disk_device_job_progress_info info;
	status_t error = _kern_get_disk_device_job_progress_info(fID, &info);
	if (error != B_OK)
		return 0;
	if (info.task_count < 1 || info.task_count <= info.completed_tasks)
		return 1;
	if (info.current_task_progress < 0)
		info.current_task_progress = 0;
	if (info.current_task_progress > 1)
		info.current_task_progress = 1;
	return (info.completed_tasks + info.current_task_progress)
		   / info.task_count;
}

// GetProgressInfo
status_t
BDiskDeviceJob::GetProgressInfo(disk_device_job_progress_info *info) const
{
	if (InitCheck() != B_OK)
		return InitCheck();
	return _kern_get_disk_device_job_progress_info(fID, info);
}

// InterruptProperties
uint32
BDiskDeviceJob::InterruptProperties() const
{
	if (InitCheck() != B_OK)
		return 0;
	disk_device_job_progress_info info;
	status_t error = _kern_get_disk_device_job_progress_info(fID, &info);
	if (error != B_OK)
		return 0;
	return info.interrupt_properties;
}

// Cancel
status_t
BDiskDeviceJob::Cancel(bool reverse)
{
	if (InitCheck() != B_OK)
		return InitCheck();
	return _kern_cancel_disk_device_job(fID, reverse);
}

// Pause
status_t
BDiskDeviceJob::Pause()
{
	if (InitCheck() != B_OK)
		return InitCheck();
	return _kern_pause_disk_device_job(fID);
}

// _SetTo
status_t
BDiskDeviceJob::_SetTo(user_disk_device_job_info *info)
{
	Unset();
	if (!info)
		return B_BAD_VALUE;
	fID = info->id;
	fType = info->type;
	fPartitionID = info->partition;
	fDescription.SetTo(info->description);
	return B_OK;
}

