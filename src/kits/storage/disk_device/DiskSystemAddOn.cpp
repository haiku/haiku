/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <DiskSystemAddOn.h>

#include <DiskDeviceDefs.h>
#include <Errors.h>


// #pragma mark - BDiskSystemAddOn


BDiskSystemAddOn::BDiskSystemAddOn(const char* name, uint32 flags)
	:
	fName(name),
	fFlags(flags)
{
}


BDiskSystemAddOn::~BDiskSystemAddOn()
{
}


const char*
BDiskSystemAddOn::Name() const
{
	return fName.String();
}


uint32
BDiskSystemAddOn::Flags() const
{
	return fFlags;
}


bool
BDiskSystemAddOn::CanInitialize(const BMutablePartition* partition)
{
	return false;
}


status_t
BDiskSystemAddOn::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	return B_NOT_SUPPORTED;
}


status_t
BDiskSystemAddOn::ValidateInitialize(const BMutablePartition* partition,
	BString* name, const char* parameters)
{
	return B_BAD_VALUE;
}


status_t
BDiskSystemAddOn::Initialize(BMutablePartition* partition, const char* name,
	const char* parameters, BPartitionHandle** handle)
{
	return B_NOT_SUPPORTED;
}


status_t
BDiskSystemAddOn::GetTypeForContentType(const char* contentType, BString* type)
{
	return B_NOT_SUPPORTED;
}


bool
BDiskSystemAddOn::IsSubSystemFor(const BMutablePartition* child)
{
	return false;
}


// #pragma mark - BPartitionHandle


BPartitionHandle::BPartitionHandle(BMutablePartition* partition)
	:
	fPartition(partition)
{
}


BPartitionHandle::~BPartitionHandle()
{
}


BMutablePartition*
BPartitionHandle::Partition() const
{
	return fPartition;
}


uint32
BPartitionHandle::SupportedOperations(uint32 mask)
{
	return 0;
}


uint32
BPartitionHandle::SupportedChildOperations(const BMutablePartition* child,
	uint32 mask)
{
	return 0;
}


bool
BPartitionHandle::SupportsInitializingChild(const BMutablePartition* child,
	const char* diskSystem)
{
	return false;
}


status_t
BPartitionHandle::GetNextSupportedType(const BMutablePartition* child,
	int32* cookie, BString* type)
{
	return B_ENTRY_NOT_FOUND;
}


status_t
BPartitionHandle::GetPartitioningInfo(BPartitioningInfo* info)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::Defragment()
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::Repair(bool checkOnly)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::ValidateResize(off_t* size)
{
	return B_BAD_VALUE;
}


status_t
BPartitionHandle::ValidateResizeChild(const BMutablePartition* child,
	off_t* size)
{
	return B_BAD_VALUE;
}


status_t
BPartitionHandle::Resize(off_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::ResizeChild(BMutablePartition* child, off_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::ValidateMove(off_t* offset)
{
	// Usually moving a disk system is a no-op for the content disk system,
	// so we default to true here.
	return B_OK;
}


status_t
BPartitionHandle::ValidateMoveChild(const BMutablePartition* child,
	off_t* offset)
{
	return B_BAD_VALUE;
}


status_t
BPartitionHandle::Move(off_t offset)
{
	// Usually moving a disk system is a no-op for the content disk system,
	// so we default to OK here.
	return B_OK;
}


status_t
BPartitionHandle::MoveChild(BMutablePartition* child, off_t offset)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::ValidateSetContentName(BString* name)
{
	return B_BAD_VALUE;
}


status_t
BPartitionHandle::ValidateSetName(const BMutablePartition* child,
	BString* name)
{
	return B_BAD_VALUE;
}


status_t
BPartitionHandle::SetContentName(const char* name)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::SetName(BMutablePartition* child, const char* name)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::ValidateSetType(const BMutablePartition* child,
	const char* type)
{
	return B_BAD_VALUE;
}


status_t
BPartitionHandle::SetType(BMutablePartition* child, const char* type)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::GetContentParameterEditor(BPartitionParameterEditor** editor)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::ValidateSetContentParameters(const char* parameters)
{
	return B_BAD_VALUE;
}


status_t
BPartitionHandle::ValidateSetParameters(const BMutablePartition* child,
	const char* parameters)
{
	return B_BAD_VALUE;
}


status_t
BPartitionHandle::SetContentParameters(const char* parameters)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::SetParameters(BMutablePartition* child,
	const char* parameters)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::ValidateCreateChild(off_t* offset, off_t* size,
	const char* type, BString* name, const char* parameters)
{
	return B_BAD_VALUE;
}


status_t
BPartitionHandle::CreateChild(off_t offset, off_t size, const char* type,
	const char* name, const char* parameters, BMutablePartition** child)
{
	return B_NOT_SUPPORTED;
}


status_t
BPartitionHandle::DeleteChild(BMutablePartition* child)
{
	return B_NOT_SUPPORTED;
}
