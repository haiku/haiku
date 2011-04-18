/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <DiskSystemAddOn.h>

#include <DiskDeviceDefs.h>
#include <Errors.h>


// #pragma mark - BDiskSystemAddOn


// constructor
BDiskSystemAddOn::BDiskSystemAddOn(const char* name, uint32 flags)
	:
	fName(name),
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
BDiskSystemAddOn::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	return B_NOT_SUPPORTED;
}


// ValidateInitialize
status_t
BDiskSystemAddOn::ValidateInitialize(const BMutablePartition* partition,
	BString* name, const char* parameters)
{
	return B_BAD_VALUE;
}


// Initialize
status_t
BDiskSystemAddOn::Initialize(BMutablePartition* partition, const char* name,
	const char* parameters, BPartitionHandle** handle)
{
	return B_NOT_SUPPORTED;
}


// GetTypeForContentType
status_t
BDiskSystemAddOn::GetTypeForContentType(const char* contentType, BString* type)
{
	return B_NOT_SUPPORTED;
}


// IsSubSystemFor
bool
BDiskSystemAddOn::IsSubSystemFor(const BMutablePartition* child)
{
	return false;
}


// #pragma mark - BPartitionHandle


// constructor
BPartitionHandle::BPartitionHandle(BMutablePartition* partition)
	:
	fPartition(partition)
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


// GetNextSupportedType
status_t
BPartitionHandle::GetNextSupportedType(const BMutablePartition* child,
	int32* cookie, BString* type)
{
	return B_ENTRY_NOT_FOUND;
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
status_t
BPartitionHandle::ValidateResize(off_t* size)
{
	return B_BAD_VALUE;
}


// ValidateResizeChild
status_t
BPartitionHandle::ValidateResizeChild(const BMutablePartition* child,
	off_t* size)
{
	return B_BAD_VALUE;
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
status_t
BPartitionHandle::ValidateMove(off_t* offset)
{
	// Usually moving a disk system is a no-op for the content disk system,
	// so we default to true here.
	return B_OK;
}


// ValidateMoveChild
status_t
BPartitionHandle::ValidateMoveChild(const BMutablePartition* child,
	off_t* offset)
{
	return B_BAD_VALUE;
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
status_t
BPartitionHandle::ValidateSetContentName(BString* name)
{
	return B_BAD_VALUE;
}


// ValidateSetName
status_t
BPartitionHandle::ValidateSetName(const BMutablePartition* child,
	BString* name)
{
	return B_BAD_VALUE;
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
status_t
BPartitionHandle::ValidateSetType(const BMutablePartition* child,
	const char* type)
{
	return B_BAD_VALUE;
}


// SetType
status_t
BPartitionHandle::SetType(BMutablePartition* child, const char* type)
{
	return B_NOT_SUPPORTED;
}


// GetContentParameterEditor
status_t
BPartitionHandle::GetContentParameterEditor(BPartitionParameterEditor** editor)
{
	return B_NOT_SUPPORTED;
}


// GetParameterEditor
status_t
BPartitionHandle::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	return B_NOT_SUPPORTED;
}


// ValidateSetContentParameters
status_t
BPartitionHandle::ValidateSetContentParameters(const char* parameters)
{
	return B_BAD_VALUE;
}


// ValidateSetParameters
status_t
BPartitionHandle::ValidateSetParameters(const BMutablePartition* child,
	const char* parameters)
{
	return B_BAD_VALUE;
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


// ValidateCreateChild
status_t
BPartitionHandle::ValidateCreateChild(off_t* offset, off_t* size,
	const char* type, BString* name, const char* parameters)
{
	return B_BAD_VALUE;
}


// CreateChild
status_t
BPartitionHandle::CreateChild(off_t offset, off_t size, const char* type,
	const char* name, const char* parameters, BMutablePartition** child)
{
	return B_NOT_SUPPORTED;
}


// DeleteChild
status_t
BPartitionHandle::DeleteChild(BMutablePartition* child)
{
	return B_NOT_SUPPORTED;
}
