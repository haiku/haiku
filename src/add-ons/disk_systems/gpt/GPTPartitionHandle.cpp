/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "GPTPartitionHandle.h"

#include <new>
#include <stdio.h>

#include <DiskDeviceTypes.h>
#include <MutablePartition.h>
#include <PartitioningInfo.h>
#include <PartitionParameterEditor.h>

#include <AutoDeleter.h>

#include "gpt_known_guids.h"


//#define TRACE_GPT_PARTITION_HANDLE
#undef TRACE
#ifdef TRACE_GPT_PARTITION_HANDLE
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) do {} while (false)
#endif


GPTPartitionHandle::GPTPartitionHandle(BMutablePartition* partition)
	:
	BPartitionHandle(partition)
{
}


GPTPartitionHandle::~GPTPartitionHandle()
{
}


status_t
GPTPartitionHandle::Init()
{
	return B_OK;
}


uint32
GPTPartitionHandle::SupportedOperations(uint32 mask)
{
	uint32 flags = B_DISK_SYSTEM_SUPPORTS_RESIZING
		| B_DISK_SYSTEM_SUPPORTS_MOVING
		| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
		| B_DISK_SYSTEM_SUPPORTS_INITIALIZING;

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
GPTPartitionHandle::SupportedChildOperations(const BMutablePartition* child,
	uint32 mask)
{
	return B_DISK_SYSTEM_SUPPORTS_RESIZING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_MOVING_CHILD
		| B_DISK_SYSTEM_SUPPORTS_SETTING_TYPE
		| B_DISK_SYSTEM_SUPPORTS_DELETING_CHILD;
}


status_t
GPTPartitionHandle::GetNextSupportedType(const BMutablePartition* child,
	int32* cookie, BString* type)
{
	int32 index = *cookie;
	TRACE("GPTPartitionHandle::GetNextSupportedType(child: %p, cookie: %ld)\n",
		child, index);

	if (index >= int32(sizeof(kTypeMap) / sizeof(kTypeMap[0])))
		return B_ENTRY_NOT_FOUND;

	type->SetTo(kTypeMap[index].type);
	*cookie = index + 1;

	return B_OK;
}


status_t
GPTPartitionHandle::GetPartitioningInfo(BPartitioningInfo* info)
{
	// init to the full size (minus the first sector)
	off_t size = Partition()->ContentSize();
	status_t status = info->SetTo(Partition()->BlockSize(),
		size - Partition()->BlockSize());
	if (status != B_OK)
		return status;

	// Exclude the space of the existing partitions
	size_t count = Partition()->CountChildren();
	for (size_t index = 0; index < count; index++) {
		BMutablePartition* child = Partition()->ChildAt(index);
		status = info->ExcludeOccupiedSpace(child->Offset(), child->Size());
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
GPTPartitionHandle::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	*editor = NULL;
	if (type == B_CREATE_PARAMETER_EDITOR) {
		try {
			*editor = new BPartitionParameterEditor();
		} catch (std::bad_alloc) {
			return B_NO_MEMORY;
		}
		return B_OK;
	}
	return B_NOT_SUPPORTED;
}


status_t
GPTPartitionHandle::ValidateCreateChild(off_t* _offset, off_t* _size,
	const char* typeString, BString* name, const char* parameters)
{
	return B_BAD_VALUE;
}


status_t
GPTPartitionHandle::CreateChild(off_t offset, off_t size,
	const char* typeString, const char* name, const char* parameters,
	BMutablePartition** _child)
{
	return B_BAD_VALUE;
}


status_t
GPTPartitionHandle::DeleteChild(BMutablePartition* child)
{
	BMutablePartition* parent = child->Parent();
	return parent->DeleteChild(child);
}
