//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#include <Session.h>

const char *B_INTEL_PARTITION_STYLE = "intel";

// constructor
BSession::BSession()
{
}

// destructor
BSession::~BSession()
{
}

// Device
BDiskDevice *
BSession::Device() const
{
	return NULL;	// not implemented
}

// Offset
off_t
BSession::Offset() const
{
	return 0;	// not implemented
}

// Size
off_t
BSession::Size() const
{
	return 0;	// not implemented
}

// CountPartitions
int32
BSession::CountPartitions() const
{
	return 0;	// not implemented
}

// PartitionAt
BPartition *
BSession::PartitionAt(int32 index) const
{
	return NULL;	// not implemented
}

// Index
int32
BSession::Index() const
{
	return -1;	// not implemented
}

// Flags
uint32
BSession::Flags() const
{
	return 0;	// not implemented
}

// IsAudio
bool
BSession::IsAudio() const
{
	return false;	// not implemented
}

// IsData
bool
BSession::IsData() const
{
	return false;	// not implemented
}

// IsVirtual
bool
BSession::IsVirtual() const
{
	return false;	// not implemented
}

// PartitionStyle
const char *
BSession::PartitionStyle() const
{
	return NULL;	// not implemented
}

// UniqueID
int32
BSession::UniqueID() const
{
	return 0;	// not implemented
}

// EachPartition
BPartition *
BSession::EachPartition(BDiskDeviceVisitor *visitor)
{
	return NULL;	// not implemented
}

// GetPartitioningParameters
status_t
BSession::GetPartitioningParameters(const char *partitioningSystem,
									BPoint dialogCenter, BString *parameters,
									bool *cancelled)
{
	return B_ERROR;	// not implemented
}

// Partition
status_t
BSession::Partition(const char *partitioningSystem, const char *parameters)
{
	return B_ERROR;	// not implemented
}

// Partition
status_t
BSession::Partition(const char *partitioningSystem, BPoint dialogCenter,
					bool *cancelled)
{
	return B_ERROR;	// not implemented
}

// copy constructor
BSession::BSession(const BSession &)
{
}

// =
BSession &
BSession::operator=(const BSession &)
{
	return *this;	// not implemented
}

