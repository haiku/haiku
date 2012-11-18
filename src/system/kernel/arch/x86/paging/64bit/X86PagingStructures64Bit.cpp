/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include "paging/64bit/X86PagingStructures64Bit.h"

#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <int.h>

#include "paging/64bit/X86PagingMethod64Bit.h"


X86PagingStructures64Bit::X86PagingStructures64Bit()
	:
	fVirtualPML4(NULL)
{
}


X86PagingStructures64Bit::~X86PagingStructures64Bit()
{
	// Free the PML4.
	free(fVirtualPML4);
}


void
X86PagingStructures64Bit::Init(uint64* virtualPML4, phys_addr_t physicalPML4)
{
	fVirtualPML4 = virtualPML4;
	pgdir_phys = physicalPML4;
}


void
X86PagingStructures64Bit::Delete()
{
	if (are_interrupts_enabled())
		delete this;
	else
		deferred_delete(this);
}

