/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the NewOS License.
 */


#include "VMKernelArea.h"

#include <heap.h>
#include <slab/Slab.h>
#include <vm/vm_priv.h>


VMKernelArea::VMKernelArea(VMAddressSpace* addressSpace, uint32 wiring,
	uint32 protection)
	:
	VMArea(addressSpace, wiring, protection)
{
}


VMKernelArea::~VMKernelArea()
{
}


/*static*/ VMKernelArea*
VMKernelArea::Create(VMAddressSpace* addressSpace, const char* name,
	uint32 wiring, uint32 protection, ObjectCache* objectCache,
	uint32 allocationFlags)
{
	VMKernelArea* area = new(objectCache, allocationFlags) VMKernelArea(
		addressSpace, wiring, protection);
	if (area == NULL)
		return NULL;

	if (area->Init(name, allocationFlags) != B_OK) {
		object_cache_delete(objectCache, area);
		return NULL;
	}

	return area;
}
