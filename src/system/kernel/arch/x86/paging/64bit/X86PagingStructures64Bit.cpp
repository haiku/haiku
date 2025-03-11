/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include "paging/64bit/X86PagingStructures64Bit.h"

#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <interrupts.h>

#include "paging/64bit/X86PagingMethod64Bit.h"


X86PagingStructures64Bit::X86PagingStructures64Bit()
	:
	fVirtualPMLTop(NULL)
{
}


X86PagingStructures64Bit::~X86PagingStructures64Bit()
{
	// Free the PMLTop.
	free(fVirtualPMLTop);
}


void
X86PagingStructures64Bit::Init(uint64* virtualPMLTop,
	phys_addr_t physicalPMLTop)
{
	fVirtualPMLTop = virtualPMLTop;
	pgdir_phys = physicalPMLTop;
}


void
X86PagingStructures64Bit::Delete()
{
	if (are_interrupts_enabled())
		delete this;
	else
		deferred_delete(this);
}

