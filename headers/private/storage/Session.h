//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _SESSION_H
#define _SESSION_H

#include <DiskDeviceVisitor.h>
#include <disk_scanner.h>
#include <ObjectList.h>
#include <Point.h>
#include <String.h>

class BDiskDevice;
class BPartition;

extern const char *B_INTEL_PARTITIONING;

class BSession {
public:
	~BSession();

	BDiskDevice *Device() const;

	off_t Offset() const;
	off_t Size() const;
	int32 BlockSize() const;

	int32 CountPartitions() const;
	BPartition *PartitionAt(int32 index) const;

	int32 Index() const;

	uint32 Flags() const;
	bool IsAudio() const;
	bool IsData() const;
	bool IsVirtual() const;

	const char *PartitioningSystem() const;

	int32 UniqueID() const;

	BPartition *VisitEachPartition(BDiskDeviceVisitor *visitor);

	status_t GetPartitioningParameters(const char *partitioningSystem,
									   BString *parameters,
									   BPoint dialogCenter = BPoint(-1, -1),
									   bool *cancelled = NULL);
	status_t Partition(const char *partitioningSystem, const char *parameters);
	status_t Partition(const char *partitioningSystem,
					   BPoint dialogCenter = BPoint(-1, -1),
					   bool *cancelled = NULL);

	static status_t GetPartitioningSystemList(BObjectList<BString> *list);

private:
	BSession();
	BSession(const BSession &);
	BSession &operator=(const BSession &);

	void _Unset();
	status_t _Unarchive(BMessage *archive);

	void _SetDevice(BDiskDevice *device);

	bool _AddPartition(BPartition *partition);

private:
	friend class BDiskDevice;

	BDiskDevice				*fDevice;
	BObjectList<BPartition>	fPartitions;
	int32					fUniqueID;
	int32					fChangeCounter;
	int32					fIndex;
// TODO: Only offset, size and flags are used. Cleanup!
	session_info			fInfo;
	BString					fPartitioningSystem;
};

#endif	// _SESSION_H
