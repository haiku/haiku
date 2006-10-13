/*
 * Copyright 2003-2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold, bonefish@users.sf.net
 */


#include <DiskDevicePrivate.h>
#include <DiskDevice.h>
#include <Partition.h>


PartitionFilter::~PartitionFilter()
{
}


//	#pragma mark - PartitionFilterVisitor

// constructor
PartitionFilterVisitor::PartitionFilterVisitor(BDiskDeviceVisitor *visitor,
											   PartitionFilter *filter)
	: BDiskDeviceVisitor(),
	  fVisitor(visitor),
	  fFilter(filter)
{
}

// Visit
bool
PartitionFilterVisitor::Visit(BDiskDevice *device)
{
	if (fFilter->Filter(device, 0))
		return fVisitor->Visit(device);
	return false;
}

// Visit
bool
PartitionFilterVisitor::Visit(BPartition *partition, int32 level)
{
	if (fFilter->Filter(partition, level))
		return fVisitor->Visit(partition, level);
	return false;
}


// #pragma mark -

// IDFinderVisitor

// constructor
IDFinderVisitor::IDFinderVisitor(int32 id)
	: BDiskDeviceVisitor(),
	  fID(id)
{
}

// Visit
bool
IDFinderVisitor::Visit(BDiskDevice *device)
{
	return (device->ID() == fID);
}

// Visit
bool
IDFinderVisitor::Visit(BPartition *partition, int32 level)
{
	return (partition->ID() == fID);
}

