/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2012, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com
 *
 * Distributed under the terms of the MIT License.
 */


#include "NTFSAddOn.h"
#include "InitializeParameterEditor.h"

#include <new>

#include <Directory.h>
#include <List.h>
#include <Path.h>
#include <Volume.h>

#include <DiskDeviceTypes.h>
#include <MutablePartition.h>

#include <AutoDeleter.h>
#include <StringForSize.h>

#include <debug.h>
#include <stdio.h>

#ifdef ASSERT
#	undef ASSERT
#endif


using std::nothrow;

#define kPartitionTypeNTFS "NT File System"

static const uint32 kDiskSystemFlags =
	0
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
;

#define TRACE printf

NTFSAddOn::NTFSAddOn()
	: BDiskSystemAddOn(kPartitionTypeNTFS, kDiskSystemFlags)
{
}


NTFSAddOn::~NTFSAddOn()
{
}


status_t
NTFSAddOn::CreatePartitionHandle(BMutablePartition* partition,
	BPartitionHandle** _handle)
{
	NTFSPartitionHandle* handle = new(nothrow) NTFSPartitionHandle(partition);
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
NTFSAddOn::CanInitialize(const BMutablePartition* partition)
{
	return true;
}


status_t
NTFSAddOn::ValidateInitialize(const BMutablePartition* partition, BString* name,
	const char* parameterString)
{
	if (!CanInitialize(partition) || !name)
		return B_BAD_VALUE;

	if (name->Length() >= MAX_PATH)
		name->Truncate(MAX_PATH - 1);
		
	name->ReplaceAll('/', '-');
	
	return B_OK;
}


status_t
NTFSAddOn::Initialize(BMutablePartition* partition, const char* name,
	const char* parameterString, BPartitionHandle** _handle)
{
	if (!CanInitialize(partition) || name == NULL)
		return B_BAD_VALUE;

	NTFSPartitionHandle* handle = new(nothrow) NTFSPartitionHandle(partition);
	if (!handle)
		return B_NO_MEMORY;
	ObjectDeleter<NTFSPartitionHandle> handleDeleter(handle);

	status_t error = partition->SetContentType(Name());
	if (error != B_OK)
		return error;

	partition->SetContentName(name);
	partition->SetContentParameters(parameterString);
	uint32 blockSize = 4096;
	partition->SetBlockSize(blockSize);
	partition->SetContentSize(partition->Size() / blockSize * blockSize);
	partition->Changed(B_PARTITION_CHANGED_INITIALIZATION);

	*_handle = handleDeleter.Detach();
	return B_OK;
}


status_t
NTFSAddOn::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	*editor = NULL;
	if (type == B_INITIALIZE_PARAMETER_EDITOR) {
		try {
			*editor = new InitializeNTFSEditor();
		} catch (std::bad_alloc) {
			return B_NO_MEMORY;
		}
		return B_OK;
	}
	return B_NOT_SUPPORTED;
}


NTFSPartitionHandle::NTFSPartitionHandle(BMutablePartition* partition)
	: BPartitionHandle(partition)
{
}


NTFSPartitionHandle::~NTFSPartitionHandle()
{
}


status_t
NTFSPartitionHandle::Init()
{
	return B_OK;
}


uint32
NTFSPartitionHandle::SupportedOperations(uint32 mask)
{
	return kDiskSystemFlags & mask;
}


status_t
get_disk_system_add_ons(BList* addOns)
{
	NTFSAddOn* addOn = new(nothrow) NTFSAddOn;
	if (!addOn)
		return B_NO_MEMORY;

	if (!addOns->AddItem(addOn)) {
		delete addOn;
		return B_NO_MEMORY;
	}
	return B_OK;
}
