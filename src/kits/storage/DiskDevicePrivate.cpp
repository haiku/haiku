//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <DiskDevicePrivate.h>
#include <DiskDevice.h>
#include <Partition.h>

// PartitionFilterVisitor

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

