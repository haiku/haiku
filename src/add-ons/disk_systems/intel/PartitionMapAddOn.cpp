/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */


#include "CreationParameterEditor.h"
#include "PartitionMapAddOn.h"

#include <new>
#include <stdio.h>

#include <DiskDeviceTypes.h>
#include <MutablePartition.h>
#include <PartitioningInfo.h>

#include <AutoDeleter.h>

#include "IntelDiskSystem.h"


//#define TRACE_PARTITION_MAP_ADD_ON
#undef TRACE
#ifdef TRACE_PARTITION_MAP_ADD_ON
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) do {} while (false)
#endif


using std::nothrow;


static const uint32 kDiskSystemFlags =
	0
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING
	| B_DISK_SYSTEM_SUPPORTS_RESIZING
	| B_DISK_SYSTEM_SUPPORTS_MOVING
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
//	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME

	| B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_NAME
	| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_PARAMETERS
	| B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD
	| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD
//	| B_DISK_SYSTEM_SUPPORTS_NAME
;


// #pragma mark - PartitionMapAddOn


PartitionMapAddOn::PartitionMapAddOn()
	:
	BDiskSystemAddOn(kPartitionTypeIntel, kDiskSystemFlags)
{
}


PartitionMapAddOn::~PartitionMapAddOn()
{
}


status_t
PartitionMapAddOn::CreatePartitionHandle(BMutablePartition* partition,
	BPartitionHandle** _handle)
{
	PartitionMapHandle* handle = new(nothrow) PartitionMapHandle(partition);
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


bool
PartitionMapAddOn::CanInitialize(const BMutablePartition* partition)
{
	// If it's big enough, we can initialize it.
	return partition->Size() >= 2 * partition->BlockSize();
}


status_t
PartitionMapAddOn::GetInitializationParameterEditor(
	const BMutablePartition* partition, BPartitionParameterEditor** editor)
{
	// Nothing to edit, really.
	*editor = NULL;
	return B_OK;
}


status_t
PartitionMapAddOn::ValidateInitialize(const BMutablePartition* partition,
	BString* name, const char* parameters)
{
	if (!CanInitialize(partition)
		|| (parameters != NULL && parameters[0] != '\0')) {
		return B_BAD_VALUE;
	}

	// we don't support a content name
	if (name != NULL)
		name->Truncate(0);

	return B_OK;
}


status_t
PartitionMapAddOn::Initialize(BMutablePartition* partition, const char* name,
	const char* parameters, BPartitionHandle** _handle)
{
	if (!CanInitialize(partition)
		|| (name != NULL && name[0] != '\0')
		|| (parameters != NULL && parameters[0] != '\0')) {
		return B_BAD_VALUE;
	}

	// create the handle
	PartitionMapHandle* handle = new(nothrow) PartitionMapHandle(partition);
	if (!handle)
		return B_NO_MEMORY;
	ObjectDeleter<PartitionMapHandle> handleDeleter(handle);

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


// #pragma mark - PartitionMapHandle


PartitionMapHandle::PartitionMapHandle(BMutablePartition* partition)
	:
	BPartitionHandle(partition)
{
}


PartitionMapHandle::~PartitionMapHandle()
{
}


status_t
PartitionMapHandle::Init()
{
	// initialize the partition map from the mutable partition

	BMutablePartition* partition = Partition();

	int32 count = partition->CountChildren();
	if (count > 4)
		return B_BAD_VALUE;

	int32 extendedCount = 0;

	for (int32 i = 0; i < count; i++) {
		BMutablePartition* child = partition->ChildAt(i);
		PartitionType type;
		if (!type.SetType(child->Type()))
			return B_BAD_VALUE;

		// only one extended partition is allowed
		if (type.IsExtended()) {
			if (++extendedCount > 1)
				return B_BAD_VALUE;
		}

		// TODO: Get these from the parameters.
		int32 index = i;
		bool active = false;

		PrimaryPartition* primary = fPartitionMap.PrimaryPartitionAt(index);
		primary->SetTo(child->Offset(), child->Size(), type.Type(), active,
			partition->BlockSize());

		child->SetChildCookie(primary);
	}

	// The extended partition (if any) is initialized by
	// ExtendedPartitionHandle::Init().

	return B_OK;
}


uint32
PartitionMapHandle::SupportedOperations(uint32 mask)
{
	BMutablePartition* partition = Partition();

	uint32 flags = B_DISK_SYSTEM_SUPPORTS_RESIZING
		| B_DISK_SYSTEM_SUPPORTS_MOVING
		| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
		| B_DISK_SYSTEM_SUPPORTS_INITIALIZING;

	// creating child
	if ((mask & B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD) != 0) {
		BPartitioningInfo info;
		if (partition->CountChildren() < 4
			&& GetPartitioningInfo(&info) == B_OK
			&& info.CountPartitionableSpaces() > 1) {
			flags |= B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD;
		}
	}

	return flags;
}


uint32
PartitionMapHandle::SupportedChildOperations(const BMutablePartition* child,
	uint32 mask)
{
	return B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
		| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD;
}


status_t
PartitionMapHandle::GetNextSupportedType(const BMutablePartition* child,
	int32* cookie, BString* type)
{
	TRACE("%p->PartitionMapHandle::GetNextSupportedType(child: %p, "
		"cookie: %ld)\n", this, child, *cookie);

	int32 index = *cookie;
	const partition_type* nextType;
	while (true) {
		nextType = fPartitionMap.GetNextSupportedPartitionType(index);
		if (nextType == NULL)
			return B_ENTRY_NOT_FOUND;
		index++;
		if (nextType->used)
			break;
	}

	if (!nextType)
		return B_ENTRY_NOT_FOUND;

	type->SetTo(nextType->name);
	*cookie = index;

	return B_OK;
}


status_t
PartitionMapHandle::GetPartitioningInfo(BPartitioningInfo* info)
{
	// init to the full size (minus the first sector)
	off_t size = Partition()->ContentSize();
	status_t error = info->SetTo(Partition()->BlockSize(),
		size - Partition()->BlockSize());
	if (error != B_OK)
		return error;

	// exclude the space of the existing partitions
	for (int32 i = 0; i < 4; i++) {
		PrimaryPartition* primary = fPartitionMap.PrimaryPartitionAt(i);
		if (!primary->IsEmpty()) {
			error = info->ExcludeOccupiedSpace(primary->Offset(),
				primary->Size());
			if (error != B_OK)
				return error;
		}
	}

	return B_OK;
}


status_t
PartitionMapHandle::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	*editor = NULL;
	if (type == B_CREATE_PARAMETER_EDITOR) {
		try {
			*editor = new PrimaryPartitionEditor();
		} catch (std::bad_alloc) {
			return B_NO_MEMORY;
		}
		return B_OK;
	}
	return B_NOT_SUPPORTED;
}


status_t
PartitionMapHandle::ValidateCreateChild(off_t* _offset, off_t* _size,
	const char* typeString, BString* name, const char* parameters)
{
	// check type
	PartitionType type;
	if (!type.SetType(typeString) || type.IsEmpty())
		return B_BAD_VALUE;

	if (type.IsExtended() && fPartitionMap.ExtendedPartitionIndex() >= 0) {
		// There can only be a single extended partition
		return B_BAD_VALUE;
	}

	// check name
	if (name)
		name->Truncate(0);

	// check parameters
	void* handle = parse_driver_settings_string(parameters);
	if (handle == NULL)
		return B_ERROR;
	get_driver_boolean_parameter(handle, "active", false, true);
	delete_driver_settings(handle);

	// do we have a spare primary partition?
	if (fPartitionMap.CountNonEmptyPrimaryPartitions() == 4)
		return B_BAD_VALUE;

	// check the free space situation
	BPartitioningInfo info;
	status_t error = GetPartitioningInfo(&info);
	if (error != B_OK)
		return error;

	// any space in the partition at all?
	int32 spacesCount = info.CountPartitionableSpaces();
	if (spacesCount == 0)
		return B_BAD_VALUE;

	// check offset and size
	off_t offset = sector_align(*_offset, Partition()->BlockSize());
	off_t size = sector_align(*_size, Partition()->BlockSize());
		// TODO: Rather round size up?
	off_t end = offset + size;

	// get the first partitionable space the requested interval intersects with
	int32 spaceIndex = -1;
	int32 closestSpaceIndex = -1;
	off_t closestSpaceDistance = 0;
	for (int32 i = 0; i < spacesCount; i++) {
		off_t spaceOffset, spaceSize;
		info.GetPartitionableSpaceAt(i, &spaceOffset, &spaceSize);
		off_t spaceEnd = spaceOffset + spaceSize;

		if ((spaceOffset >= offset && spaceOffset < end)
			|| (offset >= spaceOffset && offset < spaceEnd)) {
			spaceIndex = i;
			break;
		}

		off_t distance;
		if (offset < spaceOffset)
			distance = spaceOffset - end;
		else
			distance = spaceEnd - offset;

		if (closestSpaceIndex == -1 || distance < closestSpaceDistance) {
			closestSpaceIndex = i;
			closestSpaceDistance = distance;
		}
	}

	// get the space we found
	off_t spaceOffset, spaceSize;
	info.GetPartitionableSpaceAt(
		spaceIndex >= 0 ? spaceIndex : closestSpaceIndex, &spaceOffset,
		&spaceSize);
	off_t spaceEnd = spaceOffset + spaceSize;

	// If the requested intervald doesn't intersect with any space yet, move
	// it, so that it does.
	if (spaceIndex < 0) {
		spaceIndex = closestSpaceIndex;
		if (offset < spaceOffset) {
			offset = spaceOffset;
			end = offset + size;
		} else {
			end = spaceEnd;
			offset = end - size;
		}
	}

	// move/shrink the interval, so that it fully lies within the space
	if (offset < spaceOffset) {
		offset = spaceOffset;
		end = offset + size;
		if (end > spaceEnd) {
			end = spaceEnd;
			size = end - offset;
		}
	} else if (end > spaceEnd) {
		end = spaceEnd;
		offset = end - size;
		if (offset < spaceOffset) {
			offset = spaceOffset;
			size = end - offset;
		}
	}

	*_offset = offset;
	*_size = size;

	return B_OK;
}


status_t
PartitionMapHandle::CreateChild(off_t offset, off_t size,
	const char* typeString, const char* name, const char* parameters,
	BMutablePartition** _child)
{
	// check type
	PartitionType type;
	if (!type.SetType(typeString) || type.IsEmpty())
		return B_BAD_VALUE;
	if (type.IsExtended() && fPartitionMap.ExtendedPartitionIndex() >= 0)
		return B_BAD_VALUE;

	// check name
	if (name && *name != '\0')
		return B_BAD_VALUE;

	// check parameters
	void* handle = parse_driver_settings_string(parameters);
	if (handle == NULL)
		return B_ERROR;

	bool active = get_driver_boolean_parameter(handle, "active", false, true);
	delete_driver_settings(handle);

	// get a spare primary partition
	PrimaryPartition* primary = NULL;
	for (int32 i = 0; i < 4; i++) {
		if (fPartitionMap.PrimaryPartitionAt(i)->IsEmpty()) {
			primary = fPartitionMap.PrimaryPartitionAt(i);
			break;
		}
	}
	if (!primary)
		return B_BAD_VALUE;

	// offset properly aligned?
	if (offset != sector_align(offset, Partition()->BlockSize())
		|| size != sector_align(size, Partition()->BlockSize()))
		return B_BAD_VALUE;

	// check the free space situation
	BPartitioningInfo info;
	status_t error = GetPartitioningInfo(&info);
	if (error != B_OK)
		return error;

	bool foundSpace = false;
	off_t end = offset + size;
	int32 spacesCount = info.CountPartitionableSpaces();
	for (int32 i = 0; i < spacesCount; i++) {
		off_t spaceOffset, spaceSize;
		info.GetPartitionableSpaceAt(i, &spaceOffset, &spaceSize);
		off_t spaceEnd = spaceOffset + spaceSize;

		if (offset >= spaceOffset && end <= spaceEnd) {
			foundSpace = true;
			break;
		}
	}

	if (!foundSpace)
		return B_BAD_VALUE;

	// create the child
	// (Note: the primary partition index is indeed the child index, since
	// we picked the first empty primary partition.)
	BMutablePartition* partition = Partition();
	BMutablePartition* child;
	error = partition->CreateChild(primary->Index(), typeString, name,
		parameters, &child);
	if (error != B_OK)
		return error;

	// init the child
	child->SetOffset(offset);
	child->SetSize(size);
	child->SetBlockSize(partition->BlockSize());
	//child->SetFlags(0);
	child->SetChildCookie(primary);

	// init the primary partition
	primary->SetTo(offset, size, type.Type(), active, partition->BlockSize());

	*_child = child;
	return B_OK;
}


status_t
PartitionMapHandle::DeleteChild(BMutablePartition* child)
{
	BMutablePartition* parent = child->Parent();
	status_t error = parent->DeleteChild(child);

	return error;
}
