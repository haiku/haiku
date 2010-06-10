/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "paging/pae/X86PagingStructuresPAE.h"

#include <string.h>

#include <KernelExport.h>


#if B_HAIKU_PHYSICAL_BITS == 64


X86PagingStructuresPAE::X86PagingStructuresPAE()
{
}


X86PagingStructuresPAE::~X86PagingStructuresPAE()
{
// TODO: Implement!
	panic("X86PagingStructuresPAE::~X86PagingStructuresPAE(): not implemented");
}


void
X86PagingStructuresPAE::Init(
	pae_page_directory_pointer_table_entry* virtualPDPT,
	phys_addr_t physicalPDPT, pae_page_directory_entry* const* virtualPageDirs,
	const phys_addr_t* physicalPageDirs)
{
	fPageDirPointerTable = virtualPDPT;
	pgdir_phys = physicalPDPT;
	memcpy(fVirtualPageDirs, virtualPageDirs, sizeof(fVirtualPageDirs));
	memcpy(fPhysicalPageDirs, physicalPageDirs, sizeof(fPhysicalPageDirs));
}


void
X86PagingStructuresPAE::Delete()
{
// TODO: Implement!
	panic("X86PagingStructuresPAE::Delete(): not implemented");
}


#endif	// B_HAIKU_PHYSICAL_BITS == 64
