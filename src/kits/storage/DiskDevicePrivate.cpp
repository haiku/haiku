//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <DiskDevicePrivate.h>
#include <DiskDevice.h>
#include <RegistrarDefs.h>
#include <RosterPrivate.h>
#include <Messenger.h>
#include <Partition.h>
#include <Session.h>

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
	return false;
}

// Visit
bool
PartitionFilterVisitor::Visit(BSession *session)
{
	return false;
}

// Visit
bool
PartitionFilterVisitor::Visit(BPartition *partition)
{
	if (fFilter->Filter(partition))
		return fVisitor->Visit(partition);
	return false;
}


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
	return (device->UniqueID() == fID);
}

// Visit
bool
IDFinderVisitor::Visit(BSession *session)
{
	return (session->UniqueID() == fID);
}

// Visit
bool
IDFinderVisitor::Visit(BPartition *partition)
{
	return (partition->UniqueID() == fID);
}


// get_disk_device_messenger
status_t
BPrivate::get_disk_device_messenger(BMessenger *messenger)
{
	status_t error = (messenger ? B_OK : B_BAD_VALUE);
	if (error == B_OK) {
		BMessage request(B_REG_GET_DISK_DEVICE_MESSENGER);
		BMessage reply;
		error = BRoster::Private().SendTo(&request, &reply, false);
		if (error == B_OK && reply.what == B_REG_SUCCESS)
			error = reply.FindMessenger("messenger", messenger);
		else
			error = B_ERROR;
	}
	return error;
}

