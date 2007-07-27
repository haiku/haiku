/*
 * Copyright 2003-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Tomas Kucera, kucerat@centrum.cz
 */

/*!
	\file PartitionLocker.h
	\ingroup intel_module
	\brief Structures for easy locking and automatic unlocking partitions.
 */

#ifndef _PARTITION_LOCKER_H
#define _PARTITION_LOCKER_H

#include <disk_device_manager.h>


class PartitionLocker {
public:
	PartitionLocker(partition_id partitionID);
	~PartitionLocker();

	bool IsLocked() const;
	partition_id PartitionId() const;

protected:
	const disk_device_data	*fDevice;

private:
	partition_id			fPartitionID;
};


/*!
  \brief Structure which locks given partition for reading.

  When this structure is going to be destroyed, it automatically unlocks
  that partition.
*/
class PartitionReadLocker : public PartitionLocker {
public:
	PartitionReadLocker(partition_id partitionID);
	~PartitionReadLocker();
};


/*!
  \brief Structure which locks given partition for writing.

  When this structure is going to be destroyed, it automatically unlocks
  that partition.
*/
class PartitionWriteLocker : public PartitionLocker {
public:
	PartitionWriteLocker(partition_id partitionID);
	~PartitionWriteLocker();
};


#endif	// _PARTITION_LOCKER_H
