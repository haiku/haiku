/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <DiskSystemAddOn.h>

#include <Errors.h>


// #pragma mark - BDiskSystemAddOn


// constructor
BDiskSystemAddOn::BDiskSystemAddOn(const char* name, uint32 flags)
	: fName(name),
	  fFlags(flags)
{
}


// destructor
BDiskSystemAddOn::~BDiskSystemAddOn()
{
}


// Name
const char*
BDiskSystemAddOn::Name() const
{
	return fName.String();
}


// Flags
uint32
BDiskSystemAddOn::Flags() const
{
	return fFlags;
}


// CanInitialize
bool
BDiskSystemAddOn::CanInitialize(const BMutablePartition* partition)
{
	return false;
}


// GetInitializationParameterEditor
status_t
BDiskSystemAddOn::GetInitializationParameterEditor(
	const BMutablePartition* partition, BDiskDeviceParameterEditor** editor)
{
	return B_NOT_SUPPORTED;
}


// ValidateInitialize
bool
BDiskSystemAddOn::ValidateInitialize(const BMutablePartition* partition,
	BString* name, const char* parameters)
{
	return false;
}


// Initialize
status_t
BDiskSystemAddOn::Initialize(BMutablePartition* partition, const char* name,
	const char* parameters, BPartitionHandle** handle)
{
	return B_NOT_SUPPORTED;
}


// #pragma mark - BPartitionHandle


// constructor
BPartitionHandle::BPartitionHandle(BMutablePartition* partition)
	: fPartition(partition)
{
}


// destructor
BPartitionHandle::~BPartitionHandle()
{
}


// Partition
BMutablePartition*
BPartitionHandle::Partition() const
{
	return fPartition;
}


// SupportedOperations
uint32
BPartitionHandle::SupportedOperations(uint32 mask)
{
	return 0;
}


// SupportedChildOperations
uint32
BPartitionHandle::SupportedChildOperations(const BMutablePartition* child,
	uint32 mask)
{
	return 0;
}


// SupportsInitializingChild
bool
BPartitionHandle::SupportsInitializingChild(const BMutablePartition* child,
	const char* diskSystem)
{
	return false;
}


// IsSubSystemFor
bool
BPartitionHandle::IsSubSystemFor(const BMutablePartition* child)
{
	return false;
}


// GetNextSupportedType
status_t
BPartitionHandle::GetNextSupportedType(const BMutablePartition* child,
	int32* cookie, BString* type)
{
	return B_ENTRY_NOT_FOUND;
}


// GetTypeForContentType
status_t
BPartitionHandle::GetTypeForContentType(const char* contentType, BString* type)
{
	return B_NOT_SUPPORTED;
}


// GetPartitioningInfo
status_t
BPartitionHandle::GetPartitioningInfo(BPartitioningInfo* info)
{
	return B_NOT_SUPPORTED;
}


// Defragment
status_t
BPartitionHandle::Defragment()
{
	return B_NOT_SUPPORTED;
}


// Repair
status_t
BPartitionHandle::Repair(bool checkOnly)
{
	return B_NOT_SUPPORTED;
}


// ValidateResize
bool
BPartitionHandle::ValidateResize(off_t* size)
{
	return false;
}


// ValidateResizeChild
bool
BPartitionHandle::ValidateResizeChild(const BMutablePartition* child,
	off_t* size)
{
	return false;
}


// Resize
status_t
BPartitionHandle::Resize(off_t size)
{
	return B_NOT_SUPPORTED;
}


// ResizeChild
status_t
BPartitionHandle::ResizeChild(BMutablePartition* child, off_t size)
{
	return B_NOT_SUPPORTED;
}


// ValidateMove
bool
BPartitionHandle::ValidateMove(off_t* offset)
{
	// Usually moving a disk system is a no-op for the content disk system,
	// so we default to true here.
	return true;
}


// ValidateMoveChild
bool
BPartitionHandle::ValidateMoveChild(const BMutablePartition* child,
	off_t* offset)
{
	return false;
}


// Move
status_t
BPartitionHandle::Move(off_t offset)
{
	// Usually moving a disk system is a no-op for the content disk system,
	// so we default to OK here.
	return B_OK;
}


// MoveChild
status_t
BPartitionHandle::MoveChild(BMutablePartition* child, off_t offset)
{
	return B_NOT_SUPPORTED;
}


// ValidateSetContentName
bool
BPartitionHandle::ValidateSetContentName(BString* name)
{
	return false;
}


// ValidateSetName
bool
BPartitionHandle::ValidateSetName(const BMutablePartition* child,
	BString* name)
{
	return false;
}


// SetContentName
status_t
BPartitionHandle::SetContentName(const char* name)
{
	return B_NOT_SUPPORTED;
}


// SetName
status_t
BPartitionHandle::SetName(BMutablePartition* child, const char* name)
{
	return B_NOT_SUPPORTED;
}


// ValidateSetType
bool
BPartitionHandle::ValidateSetType(const BMutablePartition* child,
	const char* type)
{
	return false;
}


// SetType
status_t
BPartitionHandle::SetType(BMutablePartition* child, const char* type)
{
	return B_NOT_SUPPORTED;
}


// GetContentParameterEditor
status_t
BPartitionHandle::GetContentParameterEditor(BDiskDeviceParameterEditor** editor)
{
	return B_NOT_SUPPORTED;
}


// GetParameterEditor
status_t
BPartitionHandle::GetParameterEditor(const BMutablePartition* child,
	BDiskDeviceParameterEditor** editor)
{
	return B_NOT_SUPPORTED;
}


// ValidateSetContentParameters
bool
BPartitionHandle::ValidateSetContentParameters(const char* parameters)
{
	return false;
}


// ValidateSetParameters
bool
BPartitionHandle::ValidateSetParameters(const BMutablePartition* child,
	const char* parameters)
{
	return false;
}


// SetContentParameters
status_t
BPartitionHandle::SetContentParameters(const char* parameters)
{
	return B_NOT_SUPPORTED;
}


// SetParameters
status_t
BPartitionHandle::SetParameters(BMutablePartition* child,
	const char* parameters)
{
	return B_NOT_SUPPORTED;
}


// GetChildCreationParameterEditor
status_t
BPartitionHandle::GetChildCreationParameterEditor(const char* type,
	BDiskDeviceParameterEditor** editor)
{
	return B_NOT_SUPPORTED;
}


// ValidateCreateChild
bool
BPartitionHandle::ValidateCreateChild(off_t* offset, off_t* size,
	const char* type, const char* parameters)
{
	return false;
}


// CreateChild
status_t
BPartitionHandle::CreateChild(off_t offset, off_t size, const char* type,
	const char* parameters, BMutablePartition** child)
{
	return B_NOT_SUPPORTED;
}


// DeleteChild
status_t
BPartitionHandle::DeleteChild(BMutablePartition* child)
{
	return B_NOT_SUPPORTED;
}
