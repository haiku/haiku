/*
 * Copyright 2003-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@users.sf.net
 */
#ifndef _DISK_DEVICE_PRIVATE_H
#define _DISK_DEVICE_PRIVATE_H


#include <DiskDeviceDefs.h>
#include <DiskDeviceVisitor.h>

class BMessenger;
class BPath;


namespace BPrivate {

// PartitionFilter
class PartitionFilter {
public:
	virtual ~PartitionFilter();
	virtual bool Filter(BPartition *partition, int32 level) = 0;
};

// PartitionFilterVisitor
class PartitionFilterVisitor : public BDiskDeviceVisitor {
public:
	PartitionFilterVisitor(BDiskDeviceVisitor *visitor,
						   PartitionFilter *filter);

	virtual bool Visit(BDiskDevice *device);
	virtual bool Visit(BPartition *partition, int32 level);

private:
	BDiskDeviceVisitor	*fVisitor;
	PartitionFilter		*fFilter;
};

// IDFinderVisitor
class IDFinderVisitor : public BDiskDeviceVisitor {
public:
	IDFinderVisitor(partition_id id);

	virtual bool Visit(BDiskDevice *device);
	virtual bool Visit(BPartition *partition, int32 level);

private:
	partition_id		fID;
};

}	// namespace BPrivate

using BPrivate::PartitionFilter;
using BPrivate::PartitionFilterVisitor;
using BPrivate::IDFinderVisitor;

#endif	// _DISK_DEVICE_PRIVATE_H
