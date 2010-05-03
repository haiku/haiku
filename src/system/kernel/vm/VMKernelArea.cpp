/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
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
	uint32 wiring, uint32 protection, uint32 allocationFlags)
{
	VMKernelArea* area = new(malloc_flags(allocationFlags)) VMKernelArea(
		addressSpace, wiring, protection);
	if (area == NULL)
		return NULL;

	if (area->Init(name, allocationFlags) != B_OK) {
		area->~VMKernelArea();
		free_etc(area, allocationFlags);
		return NULL;
	}

	return area;
}
