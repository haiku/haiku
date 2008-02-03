/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include "ExtendedPartitionAddOn.h"

#include <new>

#include <DiskDeviceTypes.h>
#include <MutablePartition.h>


using std::nothrow;


// #pragma mark - ExtendedPartitionAddOn


// constructor
ExtendedPartitionAddOn::ExtendedPartitionAddOn()
	: BDiskSystemAddOn(kPartitionTypeIntelExtended, 0)
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
