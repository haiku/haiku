/*
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "BFSAddOn.h"

#include <new>

#include <List.h>

#include <DiskDeviceTypes.h>
#include <MutablePartition.h>

#include <AutoDeleter.h>

#include "bfs.h"
#include "bfs_disk_system.h"


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
	| B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING
//	| B_DISK_SYSTEM_SUPPORTS_DEFRAGMENTING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_CHECKING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_REPAIRING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_RESIZING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_MOVING_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME_WHILE_MOUNTED
//	| B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS_WHILE_MOUNTED
;


// #pragma mark - BFSAddOn


// constructor
BFSAddOn::BFSAddOn()
	: BDiskSystemAddOn(kPartitionTypeBFS, kDiskSystemFlags)
{
}


// destructor
BFSAddOn::~BFSAddOn()
{
}


// CreatePartitionHandle
status_t
BFSAddOn::CreatePartitionHandle(BMutablePartition* partition,
	BPartitionHandle** _handle)
{
	BFSPartitionHandle* handle = new(nothrow) BFSPartitionHandle(partition);
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
BFSAddOn::CanInitialize(const BMutablePartition* partition)
{
	// TODO: Check partition size.
	return true;
}


// GetInitializationParameterEditor
status_t
BFSAddOn::GetInitializationParameterEditor(const BMutablePartition* partition,
	BDiskDeviceParameterEditor** editor)
{
	// TODO: Implement!
	*editor = NULL;
	return B_OK;
}


// ValidateInitialize
status_t
BFSAddOn::ValidateInitialize(const BMutablePartition* partition, BString* name,
	const char* parameterString)
{
	if (!CanInitialize(partition) || !name)
		return B_BAD_VALUE;

	// validate name

	// truncate, if it is too long
	if (name->Length() >= BFS_DISK_NAME_LENGTH)
		name->Truncate(BFS_DISK_NAME_LENGTH - 1);

	// replace '/' by '-'
	name->ReplaceAll('/', '-');

	// check parameters
	initialize_parameters parameters;
	status_t error = parse_initialize_parameters(parameterString, parameters);
	if (error != B_OK)
		return error;

	return B_OK;
}


// Initialize
status_t
BFSAddOn::Initialize(BMutablePartition* partition, const char* name,
	const char* parameterString, BPartitionHandle** _handle)
{
	if (!CanInitialize(partition) || check_volume_name(name) != B_OK)
		return B_BAD_VALUE;

	initialize_parameters parameters;
	status_t error = parse_initialize_parameters(parameterString, parameters);
	if (error != B_OK)
		return error;

	// create the handle
	BFSPartitionHandle* handle = new(nothrow) BFSPartitionHandle(partition);
	if (!handle)
		return B_NO_MEMORY;
	ObjectDeleter<BFSPartitionHandle> handleDeleter(handle);

	// init the partition
	error = partition->SetContentType(Name());
	if (error != B_OK)
		return error;
// TODO: The content type could as well be set by the caller.

	partition->SetContentName(name);
	partition->SetContentParameters(parameterString);
	uint32 blockSize = parameters.blockSize;
	partition->SetBlockSize(blockSize);
	partition->SetContentSize(partition->Size() / blockSize * blockSize);
	partition->Changed(B_PARTITION_CHANGED_INITIALIZATION);

	*_handle = handleDeleter.Detach();

	return B_OK;
}


// #pragma mark - BFSPartitionHandle


// constructor
BFSPartitionHandle::BFSPartitionHandle(BMutablePartition* partition)
	: BPartitionHandle(partition)
{
}


// destructor
BFSPartitionHandle::~BFSPartitionHandle()
{
}


// Init
status_t
BFSPartitionHandle::Init()
{
// TODO: Check parameters.
	return B_OK;
}


// #pragma mark -


// get_disk_system_add_ons
status_t
get_disk_system_add_ons(BList* addOns)
{
	BFSAddOn* addOn = new(nothrow) BFSAddOn;
	if (!addOn)
		return B_NO_MEMORY;

	if (!addOns->AddItem(addOn)) {
		delete addOn;
		return B_NO_MEMORY;
	}

	return B_OK;
}
