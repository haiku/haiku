//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <DiskDevice.h>

// constructor
BDiskDevice::BDiskDevice()
{
}

// destructor
BDiskDevice::~BDiskDevice()
{
}

// CountSessions
int32
BDiskDevice::CountSessions() const
{
	return 0;	// not implemented
}

// SessionAt
BSession *
BDiskDevice::SessionAt(int32 index) const
{
	return NULL;	// not implemented
}

// CountPartitions
int32
BDiskDevice::CountPartitions() const
{
	return 0;	// not implemented
}

// BlockSize
int32
BDiskDevice::BlockSize() const
{
	return 0;	// not implemented
}

// DevicePath
const char *
BDiskDevice::DevicePath() const
{
	return NULL;	// not implemented
}

// GetName
void
BDiskDevice::GetName(BString *name, bool includeBusID, bool includeLUN) const
{
}

// GetName
void
BDiskDevice::GetName(char *name, bool includeBusID, bool includeLUN) const
{
}

// IsReadOnly
bool
BDiskDevice::IsReadOnly() const
{
	return false;	// not implemented
}

// IsRemovable
bool
BDiskDevice::IsRemovable() const
{
	return false;	// not implemented
}

// HasMedia
bool
BDiskDevice::HasMedia() const
{
	return false;	// not implemented
}

// IsFloppy
bool
BDiskDevice::IsFloppy() const
{
	return false;	// not implemented
}

// Type
uint8
BDiskDevice::Type() const
{
	return 0;	// not implemented
}

// UniqueID
int32
BDiskDevice::UniqueID() const
{
	return -1;	// not implemented
}

// Eject
status_t
BDiskDevice::Eject()
{
	return B_ERROR;	// not implemented
}

// LowLevelFormat
status_t
BDiskDevice::LowLevelFormat()
{
	return B_ERROR;	// not implemented
}

// Update
status_t
BDiskDevice::Update()
{
	return B_ERROR;	// not implemented
}

// VisitEachSession
BSession *
BDiskDevice::VisitEachSession(BDiskDeviceVisitor *visitor)
{
	return NULL;	// not implemented
}

// VisitEachPartition
BPartition *
BDiskDevice::VisitEachPartition(BDiskDeviceVisitor *visitor)
{
	return NULL;	// not implemented
}

// Traverse
bool
BDiskDevice::Traverse(BDiskDeviceVisitor *visitor)
{
	return false;	// not implemented
}

// copy constructor
BDiskDevice::BDiskDevice(const BDiskDevice &)
{
}

// =
BDiskDevice &
BDiskDevice::operator=(const BDiskDevice &)
{
	return *this;
}

