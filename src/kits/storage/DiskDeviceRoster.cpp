//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <DiskDeviceRoster.h>

// constructor
BDiskDeviceRoster::BDiskDeviceRoster()
{
}

// destructor
BDiskDeviceRoster::~BDiskDeviceRoster()
{
}

// GetNextDevice
status_t
BDiskDeviceRoster::GetNextDevice(BDiskDevice *device)
{
	return B_ERROR;	// not implemented
}

// Rewind
status_t
BDiskDeviceRoster::Rewind()
{
	return B_ERROR;	// not implemented
}

// VisitEachDevice
bool
BDiskDeviceRoster::VisitEachDevice(BDiskDeviceVisitor *visitor,
								   BDiskDevice *device)
{
	return false;	// not implemented
}

// VisitEachPartition
bool
BDiskDeviceRoster::VisitEachPartition(BDiskDeviceVisitor *visitor,
									  BDiskDevice *device,
									  BPartition **partition)
{
	return false;	// not implemented
}

// Traverse
bool
BDiskDeviceRoster::Traverse(BDiskDeviceVisitor *visitor)
{
	return false;	// not implemented
}

// VisitEachMountedPartition
bool
BDiskDeviceRoster::VisitEachMountedPartition(BDiskDeviceVisitor *visitor,
											 BDiskDevice *device,
											 BPartition **partition)
{
	return false;	// not implemented
}

// VisitEachMountablePartition
bool
BDiskDeviceRoster::VisitEachMountablePartition(BDiskDeviceVisitor *visitor,
											   BDiskDevice *device,
											   BPartition **partition)
{
	return false;	// not implemented
}

// VisitEachInitializablePartition
bool
BDiskDeviceRoster::VisitEachInitializablePartition(BDiskDeviceVisitor *visitor,
												   BDiskDevice *device,
												   BPartition **partition)
{
	return false;	// not implemented
}

// GetDeviceWithID
status_t
BDiskDeviceRoster::GetDeviceWithID(int32 id, BDiskDevice *device) const
{
	return B_ERROR;	// not implemented
}

// GetSessionWithID
status_t
BDiskDeviceRoster::GetSessionWithID(int32 id, BDiskDevice *device,
									BSession **session) const
{
	return B_ERROR;	// not implemented
}

// GetPartitionWithID
status_t
BDiskDeviceRoster::GetPartitionWithID(int32 id, BDiskDevice *device,
									  BPartition **partition) const
{
}

// StartWatching
status_t
BDiskDeviceRoster::StartWatching(BMessenger target, uint32 eventMask)
{
	return B_ERROR;	// not implemented
}

// StopWatching
status_t
BDiskDeviceRoster::StopWatching(BMessenger target)
{
	return B_ERROR;	// not implemented
}

