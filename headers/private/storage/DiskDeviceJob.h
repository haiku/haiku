//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_JOB_H
#define _DISK_DEVICE_JOB_H

#include <String.h>

// disk device job types
enum {
	B_DISK_DEVICE_JOB_CREATE,
	B_DISK_DEVICE_JOB_DELETE,
	B_DISK_DEVICE_JOB_INITIALIZE,
	B_DISK_DEVICE_JOB_RESIZE,
	B_DISK_DEVICE_JOB_MOVE,
	B_DISK_DEVICE_JOB_DEFRAGMENT,
	B_DISK_DEVICE_JOB_REPAIR,
};

// disk device job statuses
enum {
	B_DISK_DEVICE_JOB_UNINITIALIZED,
	B_DISK_DEVICE_JOB_SCHEDULED,
	B_DISK_DEVICE_JOB_IN_PROGRESS,
	B_DISK_DEVICE_JOB_SUCCEEDED,
	B_DISK_DEVICE_JOB_FAILED,
	B_DISK_DEVICE_JOB_CANCELED,
};

// disk device job progress info
struct disk_device_progress_info {
	int32	task_count;
	int32	completed_tasks;
	float	current_task_progress;
	char	current_task_description[256];
};

// disk device job cancel properties
enum {
	B_DISK_DEVICE_JOB_CAN_CANCEL			= 0x01,
	B_DISK_DEVICE_JOB_STOP_ON_CANCEL		= 0x02,
	B_DISK_DEVICE_JOB_REVERSE_ON_CANCEL		= 0x04,
};

class BDiskDeviceJob {
public:
	disk_job_id ID() const;
	uint32 Type() const;	
	float Progress() const;		// [0.0, 1.0]
	uint32 Status() const;	
	const char *Description() const;
	status_t GetProgressInfo(disk_devive_progress_info *info) const;
	uint32 CancelProperties() const;
	
	partition_id PartitionID() const;

private:
	disk_job_id		fJobID;
	uint32			fType;
	partition_id	fPartitionID;
	BString			fDescription;
};

#endif	// _DISK_DEVICE_JOB_H
