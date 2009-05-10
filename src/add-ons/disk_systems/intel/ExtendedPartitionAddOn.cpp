/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "ExtendedPartitionAddOn.h"

#include <new>
#include <stdio.h>

#include <DiskDeviceTypes.h>
#include <MutablePartition.h>
#include <PartitioningInfo.h>

#include <AutoDeleter.h>

#include "IntelDiskSystem.h"

//#define TRACE_EXTENDED_PARTITION_ADD_ON
#undef TRACE
#ifdef TRACE_EXTENDED_PARTITION_ADD_ON
# define TRACE(x...) printf(x)
#else
# define TRACE(x...) do {} while (false)
#endif


using std::nothrow;


static const uint32 kDiskSystemFlags =
	0
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING
//	| B_DISK_SYSTEM_SUPPORTS_RESIZING
//	| B_DISK_SYSTEM_SUPPORTS_MOVING
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
//	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME

//	| B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
//	| B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_NAME
	| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_PARAMETERS
	| B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD
//	| B_DISK_SYSTEM_SUPPORTS_NAME
;


// #pragma mark - ExtendedPartitionAddOn


// constructor
ExtendedPartitionAddOn::ExtendedPartitionAddOn()
	: BDiskSystemAddOn(kPartitionTypeIntelExtended, kDiskSystemFlags)
{
}


// destructor
ExtendedPartitionAddOn::~ExtendedPartitionAddOn()
{
}


// CreatePartitionHandle
status_t
ExtendedPartitionAddOn::CreatePartitionHandle(BMutablePartition* partition,
	BPartitionHandle** _handle)
{
	ExtendedPartitionHandle* handle
		= new(nothrow) ExtendedPartitionHandle(partition);
	if (!handle)
		return B_NO_MEMORY;

	status_t error = handle->Init();
	if (error != B_OK) {
		delete handle;
		return error;
	}

	*_handle = handle;
	return B_OK;
}


// CanInitialize
bool
ExtendedPartitionAddOn::CanInitialize(const BMutablePartition* partition)
{
	// If it's big enough, we can initialize it.
	return partition->Size() >= 2 * partition->BlockSize();
}


// GetInitializationParameterEditor
status_t
ExtendedPartitionAddOn::GetInitializationParameterEditor(
	const BMutablePartition* partition, BDiskDeviceParameterEditor** editor)
{
	// Nothing to edit, really.
	*editor = NULL;
	return B_OK;
}


// ValidateInitialize
status_t
ExtendedPartitionAddOn::ValidateInitialize(const BMutablePartition* partition,
	BString* name, const char* parameters)
{
	if (!CanInitialize(partition)
		|| parameters != NULL && strlen(parameters) > 0) {
		return B_BAD_VALUE;
	}

	// we don't support a content name
	if (name != NULL)
		name->Truncate(0);

	return B_OK;
}


// Initialize
status_t
ExtendedPartitionAddOn::Initialize(BMutablePartition* partition,
	const char* name, const char* parameters, BPartitionHandle** _handle)
{
	if (!CanInitialize(partition)
		|| name != NULL && strlen(name) > 0
		|| parameters != NULL && strlen(parameters) > 0) {
		return B_BAD_VALUE;
	}

	// create the handle
	ExtendedPartitionHandle* handle
		= new(nothrow) ExtendedPartitionHandle(partition);
	if (!handle)
		return B_NO_MEMORY;
	ObjectDeleter<ExtendedPartitionHandle> handleDeleter(handle);

	// init the partition
	status_t error = partition->SetContentType(Name());
	if (error != B_OK)
		return error;
	// TODO: The content type could as well be set by the caller.

	partition->SetContentName(NULL);
	partition->SetContentParameters(NULL);
	partition->SetContentSize(
		sector_align(partition->Size(), partition->BlockSize()));

	*_handle = handleDeleter.Detach();

	return B_OK;
}


// #pragma mark - ExtendedPartitionHandle


// constructor
ExtendedPartitionHandle::ExtendedPartitionHandle(BMutablePartition* partition)
	: BPartitionHandle(partition)
{
}


// destructor
ExtendedPartitionHandle::~ExtendedPartitionHandle()
{
}


// Init
status_t
ExtendedPartitionHandle::Init()
{
	// initialize the extended partition from the mutable partition

	BMutablePartition* partition = Partition();

	// our parent has already set the child cookie to the primary partition.
	fPrimaryPartition = (PrimaryPartition*)partition->ChildCookie();
	if (!fPrimaryPartition)
		return B_BAD_VALUE;

	if (!fPrimaryPartition->IsExtended())
		return B_BAD_VALUE;

	// init the child partitions
	int32 count = partition->CountChildren();
	for (int32 i = 0; i < count; i++) {
		BMutablePartition* child = partition->ChildAt(i);
		PartitionType type;
		if (!type.SetType(child->Type()))
			return B_BAD_VALUE;

		// TODO: Get these from the parameters.
		bool active = false;
		off_t ptsOffset = 0;

		LogicalPartition* logical = new(nothrow) LogicalPartition;
		if (!logical)
			return B_NO_MEMORY;

		logical->SetTo(child->Offset(), child->Size(), type.Type(), active,
			ptsOffset, fPrimaryPartition);

		child->SetChildCookie(logical);
	}

	return B_OK;
}


// SupportedOperations
uint32
ExtendedPartitionHandle::SupportedOperations(uint32 mask)
{
	uint32 flags = B_DISK_SYSTEM_SUPPORTS_INITIALIZING;

	// creating child
	if (mask & B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD) {
		BPartitioningInfo info;
		if (GetPartitioningInfo(&info) == B_OK
			&& info.CountPartitionableSpaces() > 1) {
			flags |= B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD;
		}
	}

	return flags;
}


// SupportedChildOperations
uint32
ExtendedPartitionHandle::SupportedChildOperations(
	const BMutablePartition* child, uint32 mask)
{
	return B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD;
}


// GetNextSupportedType
status_t
ExtendedPartitionHandle::GetNextSupportedType(const BMutablePartition* child,
	int32* cookie, BString* type)
{
	TRACE("%p->ExtendedPartitionHandle::GetNextSupportedType(child: %p, "
		"cookie: %ld)\n", this, child, *cookie);

	if (*cookie != 0)
		return B_ENTRY_NOT_FOUND;
	*cookie = *cookie + 1;
	*type = kPartitionTypeIntelLogical;
	return B_OK;
}


// GetPartitioningInfo
status_t
ExtendedPartitionHandle::GetPartitioningInfo(BPartitioningInfo* info)
{
	// NOTE stippi: At first I tried to use the fPrimaryPartition
	// here but it does not return any LogicalPartitions. What
	// happens now is probably what used to happen before, though
	// I don't understand *where*, since this handle type never
	// implemented this virtual function.

	// init to the full size (minus the first sector)
	BMutablePartition* partition = Partition();
	off_t offset = partition->Offset();// + SECTOR_SIZE;
	off_t size = partition->Size();//- SECTOR_SIZE;
	status_t error = info->SetTo(offset, size);
	if (error != B_OK)
		return error;

	// exclude the space of the existing logical partitions
	int32 count = partition->CountChildren();
printf("%ld logical partitions\n", count);
	for (int32 i = 0; i < count; i++) {
		BMutablePartition* child = partition->ChildAt(i);
		// TODO: Does this correctly account for the partition table
		// sectors? Preceeding each logical partition should be a
		// sector for the partition table entry. Those entries form
		// the linked list of "inner extended partition" + "real partition"
		// Following the logic above (copied from PartitionMapAddOn),
		// the outer size includes the first sector and is therefor
		// what we need here.
		error = info->ExcludeOccupiedSpace(child->Offset(), child->Size());
printf("  %ld: offset = %lld (relative: %lld), size = %lld\n", i,
child->Offset(), child->Offset() - offset, child->Size());
		if (error != B_OK)
			return error;
	}
info->PrintToStream();

	return B_OK;
}


// GetChildCreationParameterEditor
status_t
ExtendedPartitionHandle::GetChildCreationParameterEditor(const char* type,
	BDiskDeviceParameterEditor** editor)
{
	// TODO: We actually need an editor here.
	*editor = NULL;
	return B_OK;
}


// ValidateCreateChild
status_t
ExtendedPartitionHandle::ValidateCreateChild(off_t* _offset, off_t* _size,
	const char* typeString, BString* name, const char* parameters)
{
	// check type
	if (!typeString || strcmp(typeString, kPartitionTypeIntelLogical) != 0)
		return B_BAD_VALUE;

	// check name
	if (name)
		name->Truncate(0);

	// check parameters
	// TODO:...

return B_NOT_SUPPORTED;
//	// check the free space situation
//	BPartitioningInfo info;
//	status_t error = GetPartitioningInfo(&info);
//	if (error != B_OK)
//		return error;
//
//	// any space in the partition at all?
//	int32 spacesCount = info.CountPartitionableSpaces();
//	if (spacesCount == 0)
//		return B_BAD_VALUE;
//
//	// check offset and size
//	off_t offset = sector_align(*_offset);
//	off_t size = sector_align(*_size);
//		// TODO: Rather round size up?
//	off_t end = offset + size;
//
//	// get the first partitionable space the requested interval intersects with
//	int32 spaceIndex = -1;
//	int32 closestSpaceIndex = -1;
//	off_t closestSpaceDistance = 0;
//	for (int32 i = 0; i < spacesCount; i++) {
//		off_t spaceOffset, spaceSize;
//		info.GetPartitionableSpaceAt(i, &spaceOffset, &spaceSize);
//		off_t spaceEnd = spaceOffset + spaceSize;
//
//		if (spaceOffset >= offset && spaceOffset < end
//			|| offset >= spaceOffset && offset < spaceEnd) {
//			spaceIndex = i;
//			break;
//		}
//
//		off_t distance;
//		if (offset < spaceOffset)
//			distance = spaceOffset - end;
//		else
//			distance = spaceEnd - offset;
//
//		if (closestSpaceIndex == -1 || distance < closestSpaceDistance) {
//			closestSpaceIndex = i;
//			closestSpaceDistance = distance;
//		}
//	}
//
//	// get the space we found
//	off_t spaceOffset, spaceSize;
//	info.GetPartitionableSpaceAt(
//		spaceIndex >= 0 ? spaceIndex : closestSpaceIndex, &spaceOffset,
//		&spaceSize);
//	off_t spaceEnd = spaceOffset + spaceSize;
//
//	// If the requested intervald doesn't intersect with any space yet, move
//	// it, so that it does.
//	if (spaceIndex < 0) {
//		spaceIndex = closestSpaceIndex;
//		if (offset < spaceOffset) {
//			offset = spaceOffset;
//			end = offset + size;
//		} else {
//			end = spaceEnd;
//			offset = end - size;
//		}
//	}
//
//	// move/shrink the interval, so that it fully lies within the space
//	if (offset < spaceOffset) {
//		offset = spaceOffset;
//		end = offset + size;
//		if (end > spaceEnd) {
//			end = spaceEnd;
//			size = end - offset;
//		}
//	} else if (end > spaceEnd) {
//		end = spaceEnd;
//		offset = end - size;
//		if (offset < spaceOffset) {
//			offset = spaceOffset;
//			size = end - offset;
//		}
//	}
//
//	*_offset = offset;
//	*_size = size;
//
//	return B_OK;
}


// CreateChild
status_t
ExtendedPartitionHandle::CreateChild(off_t offset, off_t size,
	const char* typeString, const char* name, const char* parameters,
	BMutablePartition** _child)
{
	// check type
	if (!typeString || strcmp(typeString, kPartitionTypeIntelLogical) != 0)
		return B_BAD_VALUE;

	// check name
	if (name && strlen(name) > 0)
		return B_BAD_VALUE;

	// check parameters
	// TODO:...

return B_NOT_SUPPORTED;
//	// offset properly aligned?
//	if (offset != sector_align(offset) || size != sector_align(size))
//		return B_BAD_VALUE;
//
//	// check the free space situation
//	BPartitioningInfo info;
//	status_t error = GetPartitioningInfo(&info);
//	if (error != B_OK)
//		return error;
//
//	bool foundSpace = false;
//	off_t end = offset + size;
//	int32 spacesCount = info.CountPartitionableSpaces();
//	for (int32 i = 0; i < spacesCount; i++) {
//		off_t spaceOffset, spaceSize;
//		info.GetPartitionableSpaceAt(i, &spaceOffset, &spaceSize);
//		off_t spaceEnd = spaceOffset + spaceSize;
//
//		if (offset >= spaceOffset && end <= spaceEnd) {
//			foundSpace = true;
//			break;
//		}
//	}
//
//	if (!foundSpace)
//		return B_BAD_VALUE;
//
//	// everything looks good, do it
//
//	// create the child
//	// (Note: the primary partition index is indeed the child index, since
//	// we picked the first empty primary partition.)
//	BMutablePartition* partition = Partition();
//	BMutablePartition* child;
//	error = partition->CreateChild(primary->Index(), typeString, NULL,
//		parameters, &child);
//	if (error != B_OK)
//		return error;
//
//	// init the child
//	child->SetOffset(offset);
//	child->SetSize(size);
//	child->SetBlockSize(SECTOR_SIZE);
//	//child->SetFlags(0);
//	child->SetChildCookie(primary);
//
//	// init the primary partition
//	bool active = false;
//		// TODO: Get from parameters!
//	primary->SetTo(offset, size, type.Type(), active);
//
//// TODO: If the child is an extended partition, we should trigger its
//// initialization.
//
//	*_child = child;
//	return B_OK;
}


