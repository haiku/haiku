//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_JOB_H
#define _DISK_DEVICE_JOB_H

#include <DiskDeviceDefs.h>
#include <String.h>

struct user_disk_device_job_info;

class BDiskDeviceJob {
public:
	BDiskDeviceJob();
	~BDiskDeviceJob();

	status_t SetTo(disk_job_id id);
	void Unset();
	status_t InitCheck() const;

	disk_job_id ID() const;
	uint32 Type() const;
	partition_id PartitionID() const;
	const char *Description() const;

	uint32 Status() const;	
	float Progress() const;		// [0.0, 1.0]
	status_t GetProgressInfo(disk_device_job_progress_info *info) const;
	uint32 InterruptProperties() const;

	status_t Cancel(bool reverse = false);
	status_t Pause();
	
private:
	status_t _SetTo(user_disk_device_job_info *info);

	friend class BDiskDeviceRoster;

	disk_job_id		fID;
	uint32			fType;
	partition_id	fPartitionID;
	BString			fDescription;
};

#endif	// _DISK_DEVICE_JOB_H
