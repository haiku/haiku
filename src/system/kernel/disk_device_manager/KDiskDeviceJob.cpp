/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 *		Lubos Kulic <lubos@radical.ed>
 */

#include <stdio.h>

#include "ddm_userland_interface.h"

#include <util/kernel_cpp.h>
#include <KDiskDeviceJob.h>
#include <KDiskDeviceUtils.h>

// debugging
//#define DBG(x)
#define DBG(x) x
#define OUT dprintf

// constructor
/**
	Creates a new job
	
	\param type actual type of the job (see DiskDeviceDefs.h for details)
	\param partitionID the partition/device on which the action should be
		executed
	\param scopeID partition/device which is the highest in the hierarchy (i.e.
		closest to the root) that can be affected by the action - every
		descendant of this partition is marked busy and all ancestors are marked
		descendant-busy
*/
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

/** \fn status_t KDiskDevice::Do()
	Do the actual work of the job.

	- is supposed to be implemented in descendants
	- doesn't have any parameter - every operation needs different ones
		-> they're passed to the constructor
	- the implementations will 
		- check the parameters given in constructor (e.g. if given partition
			exists...)
		- check whether the partition has needed disk system (its own or
			parent - depends on the operation)
		- using the disk system, validate the operation for given params
		- finally execute the action

	\return B_OK when everything went OK, some error otherwise
*/

// _NextID
disk_job_id
KDiskDeviceJob::_NextID()
{
	return atomic_add(&fNextID, 1);
}

// fNextID
disk_job_id KDiskDeviceJob::fNextID = 0;


/**
	Checks if there's any descendant which is not busy/descendant-busy.

	- the condition of busy descendant is common for many disk device operations ->
			many jobs can use this

	\param partition the root of checked subtree of the whole partition
		hierarchy
*/
bool
KDiskDeviceJob::IsPartitionNotBusy(KPartition* partition)
{
	if( !partition ) {
		return false; 
	}

	struct IsNotBusyVisitor	: KPartitionVisitor {
		virtual bool VisitPre(KPartition* partition)
		{
			return !(partition->IsBusy() || partition->IsDescendantBusy());
		}
	};
	IsNotBusyVisitor notBusyVisitor;
	
	return partition->VisitEachDescendant(&notBusyVisitor);
}
