/*
 * Copyright 2007, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <new>

#include <List.h>

#include <AutoDeleter.h>

#include "ExtendedPartitionAddOn.h"
#include "PartitionMapAddOn.h"


using std::nothrow;


// get_disk_system_add_ons
status_t
get_disk_system_add_ons(BList* addOns)
{
	PartitionMapAddOn* partitionMapAddOn = new(nothrow) PartitionMapAddOn;
	ExtendedPartitionAddOn* extendedPartitionAddOn
		= new(nothrow) ExtendedPartitionAddOn;

	ObjectDeleter<PartitionMapAddOn> mapAddOnDeleter(partitionMapAddOn);
	ObjectDeleter<ExtendedPartitionAddOn> extendedAddOnDeleter(
		extendedPartitionAddOn);

	BList list;
	if (!partitionMapAddOn || !extendedPartitionAddOn
		|| !list.AddItem(partitionMapAddOn)
		|| !list.AddItem(extendedPartitionAddOn)
		|| !addOns->AddList(&list)) {
		return B_NO_MEMORY;
	}

	mapAddOnDeleter.Detach();
	extendedAddOnDeleter.Detach();

	return B_OK;
}
