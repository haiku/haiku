/*
 * Copyright 2015, François Revol <revol@free.fr>
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2012, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com
 *
 * Distributed under the terms of the MIT License.
 */


#include "FATAddOn.h"
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

static const uint32 kDiskSystemFlags =
	0
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
;

#define TRACE printf

FATAddOn::FATAddOn()
	: BDiskSystemAddOn(kPartitionTypeFAT32, kDiskSystemFlags)
{
}


FATAddOn::~FATAddOn()
{
}


status_t
FATAddOn::CreatePartitionHandle(BMutablePartition* partition,
	BPartitionHandle** _handle)
{
	FATPartitionHandle* handle = new(nothrow) FATPartitionHandle(partition);
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
FATAddOn::CanInitialize(const BMutablePartition* partition)
{
	return true;
}


status_t
FATAddOn::ValidateInitialize(const BMutablePartition* partition, BString* name,
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
FATAddOn::Initialize(BMutablePartition* partition, const char* name,
	const char* parameterString, BPartitionHandle** _handle)
{
	if (!CanInitialize(partition) || name == NULL)
		return B_BAD_VALUE;

	FATPartitionHandle* handle = new(nothrow) FATPartitionHandle(partition);
	if (!handle)
		return B_NO_MEMORY;
	ObjectDeleter<FATPartitionHandle> handleDeleter(handle);

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
FATAddOn::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	*editor = NULL;
	if (type == B_INITIALIZE_PARAMETER_EDITOR) {
		try {
			*editor = new InitializeFATEditor();
		} catch (std::bad_alloc) {
			return B_NO_MEMORY;
		}
		return B_OK;
	}
	return B_NOT_SUPPORTED;
}


FATPartitionHandle::FATPartitionHandle(BMutablePartition* partition)
	: BPartitionHandle(partition)
{
}


FATPartitionHandle::~FATPartitionHandle()
{
}


status_t
FATPartitionHandle::Init()
{
	return B_OK;
}


uint32
FATPartitionHandle::SupportedOperations(uint32 mask)
{
	return kDiskSystemFlags & mask;
}


status_t
get_disk_system_add_ons(BList* addOns)
{
	FATAddOn* addOn = new(nothrow) FATAddOn;
	if (!addOn)
		return B_NO_MEMORY;

	if (!addOns->AddItem(addOn)) {
		delete addOn;
		return B_NO_MEMORY;
	}
	return B_OK;
}
