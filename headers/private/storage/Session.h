// Session.h

#ifndef _SESSION_H
#define _SESSION_H

#include <DiskDeviceVisitor.h>
#include <fs_device.h>
#include <ObjectList.h>
#include <Point.h>
#include <String.h>

class BDiskDevice;
class BPartition;

extern const char *B_INTEL_PARTITION_STYLE;

class BSession {
public:
	BSession();
	~BSession();

	BDiskDevice *Device() const;

	off_t Offset() const;
	off_t Size() const;

	int32 CountPartitions() const;
	BPartition *PartitionAt(int32 index) const;

	int32 Index() const;

	uint32 Flags() const;
	bool IsAudio() const;
	bool IsData() const;
	bool IsVirtual() const;

	const char *PartitionStyle() const;

	int32 UniqueID() const;

	BPartition *EachPartition(BDiskDeviceVisitor *visitor);
		// return BPartition* if terminated early

	status_t GetPartitioningParameters(const char *partitioningSystem,
									   BPoint dialogCenter,
									   BString *parameters,
									   bool *cancelled = NULL);
	status_t Partition(const char *partitioningSystem, const char *parameters);
	status_t Partition(const char *partitioningSystem,
					   BPoint dialogCenter = BPoint(-1, -1),
					   bool *cancelled = NULL);

private:
	BSession(const BSession &);
	BSession &operator=(const BSession &);

private:
	BDiskDevice					*fDevice;
	BObjectList<BPartition *>	fPartitions;
	int32						fUniqueID;
	session_info				fInfo;
};

#endif	// _SESSION_H
