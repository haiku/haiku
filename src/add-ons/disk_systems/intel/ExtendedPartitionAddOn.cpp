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
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) do {} while (false)
#endif

#define PTS_OFFSET (63 * Partition()->BlockSize())


using std::nothrow;


static const uint32 kDiskSystemFlags =
	0
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING
//	| B_DISK_SYSTEM_SUPPORTS_RESIZING
//	| B_DISK_SYSTEM_SUPPORTS_MOVING
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
//	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
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


ExtendedPartitionAddOn::ExtendedPartitionAddOn()
	:
	BDiskSystemAddOn(kPartitionTypeIntelExtended, kDiskSystemFlags)
{
}


ExtendedPartitionAddOn::~ExtendedPartitionAddOn()
{
}


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


bool
ExtendedPartitionAddOn::CanInitialize(const BMutablePartition* partition)
{
	// If it's big enough, we can initialize it.
	return false;
}


status_t
ExtendedPartitionAddOn::ValidateInitialize(const BMutablePartition* partition,
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
ExtendedPartitionAddOn::Initialize(BMutablePartition* partition,
	const char* name, const char* parameters, BPartitionHandle** _handle)
{
	if (!CanInitialize(partition)
		|| (name != NULL && name[0] != '\0')
		|| (parameters != NULL && parameters[0] != '\0')) {
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


ExtendedPartitionHandle::ExtendedPartitionHandle(BMutablePartition* partition)
	:
	BPartitionHandle(partition)
{
}


ExtendedPartitionHandle::~ExtendedPartitionHandle()
{
}


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

		void* handle = parse_driver_settings_string(child->Parameters());
		if (handle == NULL)
			return B_ERROR;

		bool active = get_driver_boolean_parameter(
			handle, "active", false, true);

		off_t ptsOffset = 0;
		const char* buffer = get_driver_parameter(handle,
			"partition_table_offset", NULL, NULL);
		if (buffer != NULL)
			ptsOffset = strtoull(buffer, NULL, 10);
		else {
			delete_driver_settings(handle);
			return B_BAD_VALUE;
		}
		delete_driver_settings(handle);

		LogicalPartition* logical = new(nothrow) LogicalPartition;
		if (!logical)
			return B_NO_MEMORY;

		logical->SetTo(child->Offset(), child->Size(), type.Type(), active,
			ptsOffset, fPrimaryPartition);

		child->SetChildCookie(logical);
	}

	return B_OK;
}


uint32
ExtendedPartitionHandle::SupportedOperations(uint32 mask)
{
	uint32 flags = 0;

	// creating child
	if ((mask & B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD) != 0) {
		BPartitioningInfo info;
		if (GetPartitioningInfo(&info) == B_OK
			&& info.CountPartitionableSpaces() > 1) {
			flags |= B_DISK_SYSTEM_SUPPORTS_CREATING_CHILD;
		}
	}

	return flags;
}


uint32
ExtendedPartitionHandle::SupportedChildOperations(
	const BMutablePartition* child, uint32 mask)
{
	return B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD;
}


status_t
ExtendedPartitionHandle::GetNextSupportedType(const BMutablePartition* child,
	int32* cookie, BString* type)
{
	int32 index = *cookie;
	const partition_type* nextType;
	PartitionMap partitionMap;
	while (true) {
		nextType = partitionMap.GetNextSupportedPartitionType(index);
		if (nextType == NULL)
			return B_ENTRY_NOT_FOUND;
		index++;
		if (nextType->used
			&& strcmp(nextType->name, kPartitionTypeIntelExtended) != 0)
			break;
	}

	if (!nextType)
		return B_ENTRY_NOT_FOUND;

	type->SetTo(nextType->name);
	*cookie = index;

	return B_OK;
}


status_t
ExtendedPartitionHandle::GetPartitioningInfo(BPartitioningInfo* info)
{
	// init to the full size (minus the first PTS_OFFSET)
	BMutablePartition* partition = Partition();
	off_t offset = partition->Offset() + PTS_OFFSET;
	off_t size = partition->Size() - PTS_OFFSET;
	status_t error = info->SetTo(offset, size);
	if (error != B_OK)
		return error;

	// exclude the space of the existing logical partitions
	int32 count = partition->CountChildren();
	for (int32 i = 0; i < count; i++) {
		BMutablePartition* child = partition->ChildAt(i);
		error = info->ExcludeOccupiedSpace(child->Offset(),
			child->Size() + PTS_OFFSET + Partition()->BlockSize());
		if (error != B_OK)
			return error;

		LogicalPartition* logical = (LogicalPartition*)child->ChildCookie();
		if (logical == NULL)
			return B_BAD_VALUE;
		error = info->ExcludeOccupiedSpace(
			logical->PartitionTableOffset(),
				PTS_OFFSET + Partition()->BlockSize());
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


status_t
ExtendedPartitionHandle::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	*editor = NULL;
	return B_NOT_SUPPORTED;
}


status_t
ExtendedPartitionHandle::ValidateCreateChild(off_t* _offset, off_t* _size,
	const char* typeString, BString* name, const char* parameters)
{
	// check type
	if (!typeString)
		return B_BAD_VALUE;

	// check name
	if (name)
		name->Truncate(0);

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
ExtendedPartitionHandle::CreateChild(off_t offset, off_t size,
	const char* typeString, const char* name, const char* _parameters,
	BMutablePartition** _child)
{
	// check type
	PartitionType type;
	if (!type.SetType(typeString) || type.IsEmpty())
		return B_BAD_VALUE;

	// check name
	if (name != NULL && name[0] != '\0')
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

	BString parameters(_parameters);
	parameters << "partition_table_offset " << offset - PTS_OFFSET << " ;\n";
	// everything looks good, create the child
	BMutablePartition* child;
	error = Partition()->CreateChild(-1, typeString,
		NULL, parameters.String(), &child);
	if (error != B_OK)
		return error;

	// init the child
	child->SetOffset(offset);
	child->SetSize(size);
	child->SetBlockSize(Partition()->BlockSize());
	//child->SetFlags(0);
	child->SetChildCookie(Partition());

	*_child = child;
	return B_OK;
}


status_t
ExtendedPartitionHandle::DeleteChild(BMutablePartition* child)
{
	BMutablePartition* parent = child->Parent();
	status_t error = parent->DeleteChild(child);

	return error;
}
