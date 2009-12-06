/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the NewOS License.
 */


#include "VMKernelArea.h"

#include <heap.h>
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
	uint32 wiring, uint32 protection)
{
	VMKernelArea* area = new(nogrow) VMKernelArea(addressSpace, wiring,
		protection);
	if (area == NULL)
		return NULL;

	if (area->Init(name) != B_OK) {
		delete area;
		return NULL;
	}

	return area;
}
