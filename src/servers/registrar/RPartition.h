//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef PARTITION_H
#define PARTITION_H

#include <disk_scanner.h>
#include <SupportDefs.h>

class RDiskDevice;
class RDiskDeviceList;
class RSession;

class RPartition {
public:
	RPartition();
	~RPartition();

	status_t SetTo(int fd, const extended_partition_info *partitionInfo);
	void Unset();

	void SetSession(RSession *session) { fSession = session; }
	RDiskDeviceList *DeviceList() const;
	RDiskDevice *Device() const;
	RSession *Session() const { return fSession; }

	int32 ID() const { return fID; }
	int32 ChangeCounter() const { return fChangeCounter; }

	const extended_partition_info *Info() const { return &fInfo; }

	void Dump() const;

private:
	static int32 _NextID();

private:
	RSession				*fSession;
	int32					fID;
	int32					fChangeCounter;
	extended_partition_info	fInfo;

	static vint32			fNextID;
};

#endif	// PARTITION_H
