//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <DiskDevicePrivate.h>
#include <DiskDevice.h>
#include <Entry.h>
#include <Partition.h>
#include <Path.h>
#include <String.h>

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


// #pragma mark -

// get_unique_partition_mount_point
status_t
BPrivate::get_unique_partition_mount_point(BPartition *partition,
	BPath *mountPoint)
{
	if (!partition || !mountPoint)
		return B_BAD_VALUE;

	// get the volume name
	const char *volumeName = partition->ContentName();
	if (volumeName || strlen(volumeName) == 0)
		volumeName = partition->Name();
	if (!volumeName || strlen(volumeName) == 0)
		volumeName = "unnamed volume";

	// construct a path name from the volume name
	// replace '/'s and prepend a '/'
	BString mountPointPath(volumeName);
	mountPointPath.ReplaceAll('/', '-');
	mountPointPath.Insert("/", 0);

	// make the name unique
	BString basePath(mountPointPath);
	int counter = 1;
	while (true) {
		BEntry entry;
		status_t error = entry.SetTo(mountPointPath.String());
		if (error != B_OK)
			return error;

		if (!entry.Exists())
			break;
		mountPointPath = basePath;
		mountPointPath << counter;
		counter++;
	}

	return mountPoint->SetTo(mountPointPath.String());
}

