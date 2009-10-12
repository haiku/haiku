/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tomas Kucera, kucerat@centrum.cz
 */


#include "PartitionLocker.h"


//	#pragma mark - PartitionLocker


PartitionLocker::PartitionLocker(partition_id partitionID)
	:
	fDevice(NULL),
	fPartitionID(partitionID)
{
}


PartitionLocker::~PartitionLocker()
{
}


bool
PartitionLocker::IsLocked() const
{
	return fDevice != NULL;
}


partition_id
PartitionLocker::PartitionId() const
{
	return fPartitionID;
}


//	#pragma mark - PartitionReadLocker


PartitionReadLocker::PartitionReadLocker(partition_id partitionID)
	:
	PartitionLocker(partitionID)
{
	fDevice = read_lock_disk_device(partitionID);
}


PartitionReadLocker::~PartitionReadLocker()
{
	if (IsLocked())
		read_unlock_disk_device(PartitionId());
}


//	#pragma mark - PartitionWriteLocker


PartitionWriteLocker::PartitionWriteLocker(partition_id partitionID)
	:
	PartitionLocker(partitionID)
{
	fDevice = write_lock_disk_device(partitionID);
}


PartitionWriteLocker::~PartitionWriteLocker()
{
	if (IsLocked())
		write_unlock_disk_device(PartitionId());
}
