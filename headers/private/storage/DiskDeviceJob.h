//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_JOB_H
#define _DISK_DEVICE_JOB_H

#include <DiskDeviceDefs.h>
#include <String.h>

class BDiskDeviceJob {
public:
	disk_job_id ID() const;
	uint32 Type() const;	
	partition_id PartitionID() const;

	float Progress() const;		// [0.0, 1.0]
	uint32 Status() const;	
	const char *Description() const;
	status_t GetProgressInfo(disk_device_job_progress_info *info) const;
	uint32 InterruptProperties() const;

	status_t Cancel(bool reverse = false);
	status_t Pause();
	
private:
	disk_job_id		fJobID;
	uint32			fType;
	partition_id	fPartitionID;
	BString			fDescription;
};

#endif	// _DISK_DEVICE_JOB_H
