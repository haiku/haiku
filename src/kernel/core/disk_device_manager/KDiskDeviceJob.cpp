// KDiskDeviceJob.cpp

#include <stdio.h>

#include "ddm_userland_interface.h"

#include <KDiskDeviceJob.h>
#include <KDiskDeviceUtils.h>

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT printf

// constructor
KDiskDeviceJob::KDiskDeviceJob(uint32 type, partition_id partitionID,
							   partition_id scopeID)
	: fID(_NextID()),
	  fJobQueue(NULL),
	  fType(type),
	  fStatus(B_DISK_DEVICE_JOB_UNINITIALIZED),
	  fPartitionID(partitionID),
	  fScopeID(scopeID),
	  fDescription(NULL),
	  fErrorMessage(NULL),
	  fTaskCount(1),
	  fCompletedTasks(0),
	  fInterruptProperties(0),
	  fProgress(0)
{
	if (fScopeID < 0)
		fScopeID = fPartitionID;
}

// destructor
KDiskDeviceJob::~KDiskDeviceJob()
{
	if (fDescription)
		free(fDescription);
}

// ID
disk_job_id
KDiskDeviceJob::ID() const
{
	return fID;
}

// SetJobQueue
void
KDiskDeviceJob::SetJobQueue(KDiskDeviceJobQueue *queue)
{
	fJobQueue = queue;
}

// JobQueue
BPrivate::DiskDevice::KDiskDeviceJobQueue *
KDiskDeviceJob::JobQueue() const
{
	return fJobQueue;
}

// Type
uint32
KDiskDeviceJob::Type() const
{
	return fType;
}

// SetStatus
void
KDiskDeviceJob::SetStatus(uint32 status)
{
	fStatus = status;
}

// Status
uint32
KDiskDeviceJob::Status() const
{
	return fStatus;
}

// SetDescription
status_t
KDiskDeviceJob::SetDescription(const char *description)
{
	return set_string(fDescription, description);
}

// Description
const char *
KDiskDeviceJob::Description() const
{
	return fDescription;
}

// PartitionID
partition_id
KDiskDeviceJob::PartitionID() const
{
	return fPartitionID;
}

// ScopeID
partition_id
KDiskDeviceJob::ScopeID() const
{
	return fScopeID;
}

// SetErrorMessage
status_t
KDiskDeviceJob::SetErrorMessage(const char *message)
{
	return set_string(fErrorMessage, message);
}

// ErrorMessage
const char *
KDiskDeviceJob::ErrorMessage() const
{
	return fErrorMessage;
}

// SetTaskCount
void
KDiskDeviceJob::SetTaskCount(int32 count)
{
	fTaskCount = count;
}

// TaskCount
int32
KDiskDeviceJob::TaskCount() const
{
	return fTaskCount;
}

// SetCompletedTasks
void
KDiskDeviceJob::SetCompletedTasks(int32 count)
{
	fCompletedTasks = count;
}

// CompletedTasks
int32
KDiskDeviceJob::CompletedTasks() const
{
	return fCompletedTasks;
}

// SetInterruptProperties
void
KDiskDeviceJob::SetInterruptProperties(uint32 properties)
{
	fInterruptProperties = properties;
}

// InterruptProperties
uint32
KDiskDeviceJob::InterruptProperties() const
{
	return fInterruptProperties;
}

// GetTaskDescription
void
KDiskDeviceJob::GetTaskDescription(char *description)
{
	if (!description)
		return;
	if (fDescription)
		strcpy(description, fDescription);
	else
		description[0] = '\0';
}

// UpdateProgress
void
KDiskDeviceJob::UpdateProgress(float progress)
{
	if (fProgress == progress)
		return;
	fProgress = progress;
	// TODO: notification
}

// UpdateExtraProgress
void
KDiskDeviceJob::UpdateExtraProgress(const char *info)
{
	// TODO:...
}

// Progress
float
KDiskDeviceJob::Progress() const
{
	return fProgress;
}

// GetInfo
status_t
KDiskDeviceJob::GetInfo(user_disk_device_job_info *info)
{
	if (!info)
		return B_BAD_VALUE;
	info->id = ID();
	info->type = Type();
	info->partition = PartitionID();
	if (Description())
		strcpy(info->description, Description());
	else
		info->description[0] = '\0';
	return B_OK;
}

// GetProgressInfo
status_t
KDiskDeviceJob::GetProgressInfo(disk_device_job_progress_info *info)
{
	if (!info)
		return B_BAD_VALUE;
	info->status = Status();
	info->interrupt_properties = InterruptProperties();
	info->task_count = TaskCount();
	info->completed_tasks = CompletedTasks();
	info->current_task_progress = Progress();
	GetTaskDescription(info->current_task_description);
	return B_OK;
}

// _NextID
disk_job_id
KDiskDeviceJob::_NextID()
{
	return atomic_add(&fNextID, 1);
}

// fNextID
disk_job_id KDiskDeviceJob::fNextID = 0;

