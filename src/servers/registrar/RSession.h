//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef SESSION_H
#define SESSION_H

#include <disk_scanner.h>
#include <ObjectList.h>

#include "RChangeCounter.h"

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

	status_t PartitionLayoutChanged();

	int32 ID() const { return fID; }
	int32 ChangeCounter() const { return fChangeCounter.Count(); }
	void Changed();

	int32 Index() const;

	status_t AddPartition(int fd, const extended_partition_info *partitionInfo,
						  uint32 cause);
	bool AddPartition(RPartition *partition, uint32 cause);
	bool RemovePartition(int32 index, uint32 cause);
	bool RemovePartition(RPartition *partition, uint32 cause);
	int32 CountPartitions() const { return fPartitions.CountItems(); }
	RPartition *PartitionAt(int32 index) const
		{ return fPartitions.ItemAt(index); }
	int32 IndexOfPartition(const RPartition *partition) const
		{ return fPartitions.IndexOf(partition); }

	const session_info *Info() const { return &fInfo; }
	off_t Offset() const { return fInfo.offset; }
	off_t Size() const { return fInfo.size; }

	status_t Update(const session_info *sessionInfo);

	status_t Archive(BMessage *archive) const;

	void Dump() const;

private:

	status_t _RescanPartitions(int fd, uint32 cause);
	static int32 _NextID();

private:
	BObjectList<RPartition>	fPartitions;
	RDiskDevice				*fDevice;
	int32					fID;
	RChangeCounter			fChangeCounter;
	session_info			fInfo;
	char					fPartitioningSystem[B_FILE_NAME_LENGTH];

	static vint32			fNextID;
};

#endif	// SESSION_H
