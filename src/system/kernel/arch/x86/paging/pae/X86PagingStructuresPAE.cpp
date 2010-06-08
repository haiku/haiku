/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "paging/pae/X86PagingStructuresPAE.h"

#include <KernelExport.h>


#if B_HAIKU_PHYSICAL_BITS == 64


X86PagingStructuresPAE::X86PagingStructuresPAE()
{
}


X86PagingStructuresPAE::~X86PagingStructuresPAE()
{
}


void
X86PagingStructuresPAE::Init()
{
// TODO: Implement!
	panic("unsupported");
}


void
X86PagingStructuresPAE::Delete()
{
// TODO: Implement!
	panic("unsupported");
}


/*static*/ void
X86PagingStructuresPAE::StaticInit()
{
// TODO: Implement!
	panic("unsupported");
}


#endif	// B_HAIKU_PHYSICAL_BITS == 64
