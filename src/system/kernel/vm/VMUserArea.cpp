/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the NewOS License.
 */


#include "VMUserArea.h"

#include <heap.h>
#include <vm/vm_priv.h>


VMUserArea::VMUserArea(VMAddressSpace* addressSpace, uint32 wiring,
	uint32 protection)
	:
	VMArea(addressSpace, wiring, protection)
{
}


VMUserArea::~VMUserArea()
{
}


/*static*/ VMUserArea*
VMUserArea::Create(VMAddressSpace* addressSpace, const char* name,
	uint32 wiring, uint32 protection)
{
	VMUserArea* area = new(nogrow) VMUserArea(addressSpace, wiring,
		protection);
	if (area == NULL)
		return NULL;

	if (area->Init(name) != B_OK) {
		delete area;
		return NULL;
	}

	return area;
}


/*static*/ VMUserArea*
VMUserArea::CreateReserved(VMAddressSpace* addressSpace, uint32 flags)
{
	VMUserArea* area = new(nogrow) VMUserArea(addressSpace, 0, 0);
	if (area != NULL)
		area->id = RESERVED_AREA_ID;
	return area;
}
