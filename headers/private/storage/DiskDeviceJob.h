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

class BDiskDeviceJob {
public:
	uint32 ID() const;
	uint32 Type() const;	
	float Progress() const;		// [0.0, 1.0]
	uint32 Status() const;	
	const char *Description() const;
	
	BPartition* Partition() const;
private:
	uint32 fJobID;
	uint32 fType;
	uint32 fPartitionID;
	uint32 fStatus;
	BString fDescription;
};

#endif	// _DISK_DEVICE_JOB_H
