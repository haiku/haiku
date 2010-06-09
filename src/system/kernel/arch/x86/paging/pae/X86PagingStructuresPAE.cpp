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
	panic("X86PagingStructuresPAE::Init(): not implemented");
}


void
X86PagingStructuresPAE::Delete()
{
// TODO: Implement!
	panic("X86PagingStructuresPAE::Delete(): not implemented");
}


/*static*/ void
X86PagingStructuresPAE::StaticInit()
{
// TODO: Implement!
	panic("X86PagingStructuresPAE::StaticInit(): not implemented");
}


#endif	// B_HAIKU_PHYSICAL_BITS == 64
