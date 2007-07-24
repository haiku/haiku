/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tomas Kucera, kucerat@centrum.cz
 */

#include "PartitionLocker.h"

//	#pragma mark - PartitionLocker

// constructor
PartitionLocker::PartitionLocker(partition_id partitionID)
	: device_(NULL),
	  partitionID_(partitionID)
{	
}

// destructor
PartitionLocker::~PartitionLocker()
{
}

// IsLocked
bool
PartitionLocker::IsLocked() const
{
	return device_;
}

// PartitionId
partition_id
PartitionLocker::PartitionId() const
{
	return partitionID_;
}


//	#pragma mark - PartitionReadLocker


// constructor
PartitionReadLocker::PartitionReadLocker(partition_id partitionID)
	: PartitionLocker(partitionID)
{
	device_ = read_lock_disk_device(partitionID);
}

// destructor
PartitionReadLocker::~PartitionReadLocker()
{
	if (IsLocked())
		read_unlock_disk_device(PartitionId());
}


//	#pragma mark - PartitionWriteLocker


// constructor
PartitionWriteLocker::PartitionWriteLocker(partition_id partitionID)
	: PartitionLocker(partitionID)
{
	device_ = write_lock_disk_device(partitionID);
}

// destructor
PartitionWriteLocker::~PartitionWriteLocker()
{
	if (IsLocked())
		write_unlock_disk_device(PartitionId());
}
