/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <new>

#include <DiskDeviceDefs.h>
#include <DiskSystemAddOn.h>

#include <AutoDeleter.h>
#include <MutablePartition.h>

#include "checksumfs.h"


static const uint32 kDiskSystemFlags =
    0
//  | B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME
//  | B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS
    | B_DISK_SYSTEM_SUPPORTS_INITIALIZING
    | B_DISK_SYSTEM_SUPPORTS_CONTENT_NAME
//  | B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_NAME_WHILE_MOUNTED
//  | B_DISK_SYSTEM_SUPPORTS_SETTING_CONTENT_PARAMETERS_WHILE_MOUNTED
;


class CheckSumFSAddOn : public BDiskSystemAddOn {
public:
								CheckSumFSAddOn();

	virtual	status_t			CreatePartitionHandle(
									BMutablePartition* partition,
									BPartitionHandle** _handle);

	virtual	bool				CanInitialize(
									const BMutablePartition* partition);
	virtual	status_t			GetInitializationParameterEditor(
									const BMutablePartition* partition,
									BPartitionParameterEditor** _editor);
	virtual	status_t			ValidateInitialize(
									const BMutablePartition* partition,
									BString* name, const char* parameters);
	virtual	status_t			Initialize(BMutablePartition* partition,
									const char* name, const char* parameters,
									BPartitionHandle** _handle);
};


class CheckSumFSPartitionHandle : public BPartitionHandle {
public:
								CheckSumFSPartitionHandle(
									BMutablePartition* partition);
								~CheckSumFSPartitionHandle();

			status_t			Init();

	virtual	uint32				SupportedOperations(uint32 mask);
};


// #pragma mark - CheckSumFSAddOn


CheckSumFSAddOn::CheckSumFSAddOn()
	:
	BDiskSystemAddOn(CHECK_SUM_FS_PRETTY_NAME, kDiskSystemFlags)
{
}


status_t
CheckSumFSAddOn::CreatePartitionHandle(BMutablePartition* partition,
	BPartitionHandle** _handle)
{
debug_printf("CheckSumFSAddOn::CreatePartitionHandle()\n");
	CheckSumFSPartitionHandle* handle
		= new(std::nothrow) CheckSumFSPartitionHandle(partition);
	if (handle == NULL)
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
CheckSumFSAddOn::CanInitialize(const BMutablePartition* partition)
{
debug_printf("CheckSumFSAddOn::CanInitialize()\n");
	return (uint64)partition->Size() >= kCheckSumFSMinSize;
}


status_t
CheckSumFSAddOn::GetInitializationParameterEditor(
	const BMutablePartition* partition, BPartitionParameterEditor** _editor)
{
debug_printf("CheckSumFSAddOn::GetInitializationParameterEditor()\n");
	*_editor = NULL;
	return B_OK;
}


status_t
CheckSumFSAddOn::ValidateInitialize(const BMutablePartition* partition,
	BString* name, const char* parameters)
{
debug_printf("CheckSumFSAddOn::ValidateInitialize()\n");
	if (!CanInitialize(partition) || name == NULL)
		return B_BAD_VALUE;

	// truncate name, if too long
	if ((uint64)name->Length() >= kCheckSumFSNameLength)
		name->Truncate(kCheckSumFSNameLength - 1);

	// replace '/' by '-'
	name->ReplaceAll('/', '-');

	return B_OK;
}


status_t
CheckSumFSAddOn::Initialize(BMutablePartition* partition, const char* name,
	const char* parameters, BPartitionHandle** _handle)
{
debug_printf("CheckSumFSAddOn::Initialize()\n");
	if (!CanInitialize(partition) || name == NULL
		|| strlen(name) >= kCheckSumFSNameLength) {
		return B_BAD_VALUE;
	}

	CheckSumFSPartitionHandle* handle
		= new(std::nothrow) CheckSumFSPartitionHandle(partition);
	if (handle == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<CheckSumFSPartitionHandle> handleDeleter(handle);

	status_t error = partition->SetContentType(Name());
	if (error != B_OK)
		return error;

	partition->SetContentName(name);
	partition->SetContentParameters(parameters);
	partition->SetBlockSize(B_PAGE_SIZE);
	partition->SetContentSize(partition->Size() / B_PAGE_SIZE * B_PAGE_SIZE);
	partition->Changed(B_PARTITION_CHANGED_INITIALIZATION);

	*_handle = handleDeleter.Detach();

	return B_OK;
}


// #pragma mark - CheckSumFSPartitionHandle


CheckSumFSPartitionHandle::CheckSumFSPartitionHandle(
	BMutablePartition* partition)
	:
	BPartitionHandle(partition)
{
}


CheckSumFSPartitionHandle::~CheckSumFSPartitionHandle()
{
}


status_t
CheckSumFSPartitionHandle::Init()
{
	return B_OK;
}


uint32
CheckSumFSPartitionHandle::SupportedOperations(uint32 mask)
{
	return kDiskSystemFlags & mask;
}



// #pragma mark -


status_t
get_disk_system_add_ons(BList* addOns)
{
debug_printf("checksumfs: get_disk_system_add_ons()\n");
    CheckSumFSAddOn* addOn = new(std::nothrow) CheckSumFSAddOn;
    if (addOn == NULL)
        return B_NO_MEMORY;

    if (!addOns->AddItem(addOn)) {
        delete addOn;
        return B_NO_MEMORY;
    }

debug_printf("checksumfs: get_disk_system_add_ons() done ok\n");
    return B_OK;
}
