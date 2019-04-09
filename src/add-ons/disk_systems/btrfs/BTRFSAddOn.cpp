/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2012, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com
 * Copyright 2019, Les De Ridder, les@lesderid.net
 *
 * Distributed under the terms of the MIT License.
 */


#include "BTRFSAddOn.h"
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

#include <cstdio>
#include <debug.h>

#include "btrfs.h"
#include "btrfs_disk_system.h"

#ifdef ASSERT
#	undef ASSERT
#endif


using std::nothrow;

static const uint32 kDiskSystemFlags =
	0
	| B_DISK_SYSTEM_SUPPORTS_INITIALIZING
	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING
//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING_WHILE_MOUNTED
;

//#define TRACE_BTRFS_ADD_ON
#undef TRACE
#ifdef TRACE_BTRFS_ADD_ON
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) do {} while (false)
#endif

BTRFSAddOn::BTRFSAddOn()
	: BDiskSystemAddOn(kPartitionTypeBTRFS, kDiskSystemFlags)
{
}


BTRFSAddOn::~BTRFSAddOn()
{
}


status_t
BTRFSAddOn::CreatePartitionHandle(BMutablePartition* partition,
	BPartitionHandle** _handle)
{
	BTRFSPartitionHandle* handle = new(nothrow) BTRFSPartitionHandle(partition);
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
BTRFSAddOn::CanInitialize(const BMutablePartition* partition)
{
	// We can initialize if the partition is large enough.

	//TODO(lesderid): Check min size
	return true;
}


status_t
BTRFSAddOn::ValidateInitialize(const BMutablePartition* partition,
	BString* name, const char* parameterString)
{
	if (!CanInitialize(partition) || !name)
		return B_BAD_VALUE;

	// truncate, if it is too long
	if (name->Length() >= BTRFS_LABEL_SIZE)
		name->Truncate(BTRFS_LABEL_SIZE - 1);

	// replace '/' and '\\' by '-'
	name->ReplaceAll('/', '-');
	name->ReplaceAll('\\', '-');

	// check parameters
	initialize_parameters parameters;
	status_t error = parse_initialize_parameters(parameterString, parameters);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
BTRFSAddOn::Initialize(BMutablePartition* partition, const char* name,
	const char* parameterString, BPartitionHandle** _handle)
{
	if (!CanInitialize(partition) || check_volume_name(name) != B_OK)
		return B_BAD_VALUE;

	initialize_parameters parameters;
	status_t error = parse_initialize_parameters(parameterString, parameters);
	if (error != B_OK)
		return error;

	BTRFSPartitionHandle* handle = new(nothrow) BTRFSPartitionHandle(partition);
	if (!handle)
		return B_NO_MEMORY;
	ObjectDeleter<BTRFSPartitionHandle> handleDeleter(handle);

	error = partition->SetContentType(Name());
	if (error != B_OK)
		return error;

	partition->SetContentName(name);
	partition->SetContentParameters(parameterString);
	uint32 blockSize = parameters.blockSize;
	partition->SetBlockSize(blockSize);
	partition->SetContentSize(partition->Size() / blockSize * blockSize);
	partition->Changed(B_PARTITION_CHANGED_INITIALIZATION);

	*_handle = handleDeleter.Detach();

	return B_OK;
}


status_t
BTRFSAddOn::GetParameterEditor(B_PARAMETER_EDITOR_TYPE type,
	BPartitionParameterEditor** editor)
{
	*editor = NULL;
	if (type == B_INITIALIZE_PARAMETER_EDITOR) {
		try {
			*editor = new InitializeBTRFSEditor();
		} catch (std::bad_alloc&) {
			return B_NO_MEMORY;
		}
		return B_OK;
	}
	return B_NOT_SUPPORTED;
}


BTRFSPartitionHandle::BTRFSPartitionHandle(BMutablePartition* partition)
	: BPartitionHandle(partition)
{
}


BTRFSPartitionHandle::~BTRFSPartitionHandle()
{
}


status_t
BTRFSPartitionHandle::Init()
{
	return B_OK;
}


uint32
BTRFSPartitionHandle::SupportedOperations(uint32 mask)
{
	return kDiskSystemFlags & mask;
}


status_t
BTRFSPartitionHandle::Repair(bool checkOnly)
{
	TRACE("BTRFSPartitionHandle::Repair(checkOnly=%d)\n", checkOnly);
	return B_NOT_SUPPORTED;
}


status_t
get_disk_system_add_ons(BList* addOns)
{
	BTRFSAddOn* addOn = new(nothrow) BTRFSAddOn;
	if (!addOn) {
		return B_NO_MEMORY;
	}

	if (!addOns->AddItem(addOn)) {
		delete addOn;
		return B_NO_MEMORY;
	}

	return B_OK;
}
