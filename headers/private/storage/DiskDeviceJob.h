//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_JOB_H
#define _DISK_DEVICE_JOB_H

// disk device job types
enum {
	B_DISK_DEVICE_JOB_CREATE,
	B_DISK_DEVICE_JOB_DELETE,
	B_DISK_DEVICE_JOB_INITIALIZE,
	B_DISK_DEVICE_JOB_RESIZE,
	B_DISK_DEVICE_JOB_MOVE,
	B_DISK_DEVICE_JOB_DEFRAGMENT,
	B_DISK_DEVICE_JOB_REPAIR,
}

class BDiskDeviceJob {
public:
	int32 ID() const;
	uint32 Type() const;	
	uint8 Progress() const;		// 0 to 100
	bool Finished() const;	
	const char *Description() const;
	
	BPartition* Partition() const;
private:
	int32 fPartitionID;
	int32 fJobID;
};

#endif	// _DISK_DEVICE_JOB_H
