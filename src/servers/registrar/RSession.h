//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef SESSION_H
#define SESSION_H

#include <disk_scanner.h>
#include <ObjectList.h>

class RDiskDevice;
class RDiskDeviceList;
class RPartition;

class RSession {
public:
	RSession();
	~RSession();

	status_t SetTo(int fd, const session_info *sessionInfo);
	void Unset();

	void SetDevice(RDiskDevice *device) { fDevice = device; }
	RDiskDeviceList *DeviceList() const;
	RDiskDevice *Device() const { return fDevice; }

	int32 ID() const { return fID; }
	int32 ChangeCounter() const { return fChangeCounter; }

	bool AddPartition(RPartition *partition);
	bool RemovePartition(int32 index);
	bool RemovePartition(RPartition *partition);
	int32 CountPartitions() const { return fPartitions.CountItems(); }
	RPartition *PartitionAt(int32 index) const
		{ return fPartitions.ItemAt(index); }

	const session_info *Info() const { return &fInfo; }

	void Dump() const;

private:
	static int32 _NextID();

private:
	BObjectList<RPartition>	fPartitions;
	RDiskDevice				*fDevice;
	int32					fID;
	int32					fChangeCounter;
	session_info			fInfo;
	char					fPartitioningSystem[B_FILE_NAME_LENGTH];

	static vint32			fNextID;
};

#endif	// SESSION_H
