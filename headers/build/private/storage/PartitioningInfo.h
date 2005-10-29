//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _PARTITIONING_INFO_H
#define _PARTITIONING_INFO_H

#include <DiskDeviceDefs.h>

struct partitionable_space_data;

class BPartitioningInfo {
public:
	BPartitioningInfo();
	virtual ~BPartitioningInfo();

	void Unset();

	partition_id PartitionID() const;

	status_t GetPartitionableSpaceAt(int32 index, off_t *offset,
									 off_t *size) const;
	int32 CountPartitionableSpaces() const;

private:
	status_t _SetTo(partition_id partition, int32 changeCounter);

	friend class BPartition;

	partition_id				fPartitionID;
	partitionable_space_data	*fSpaces;
	int32						fCount;
};

#endif	// _PARTITIONING_INFO_H
