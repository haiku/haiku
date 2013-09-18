/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "paging/pae/X86PagingStructuresPAE.h"

#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <int.h>

#include "paging/pae/X86PagingMethodPAE.h"


#if B_HAIKU_PHYSICAL_BITS == 64


X86PagingStructuresPAE::X86PagingStructuresPAE()
	:
	fPageDirPointerTable(NULL)
{
	memset(fVirtualPageDirs, 0, sizeof(fVirtualPageDirs));
}


X86PagingStructuresPAE::~X86PagingStructuresPAE()
{
	// free the user page dirs
	free(fVirtualPageDirs[0]);
		// There's one contiguous allocation for 0 and 1.

	// free the PDPT page
	if (fPageDirPointerTable != NULL) {
		X86PagingMethodPAE::Method()->Free32BitPage(fPageDirPointerTable,
			pgdir_phys, fPageDirPointerTableHandle);
	}
}


void
X86PagingStructuresPAE::Init(
	pae_page_directory_pointer_table_entry* virtualPDPT,
	phys_addr_t physicalPDPT, void* pdptHandle,
	pae_page_directory_entry* const* virtualPageDirs,
	const phys_addr_t* physicalPageDirs)
{
	fPageDirPointerTable = virtualPDPT;
	pgdir_phys = physicalPDPT;
	fPageDirPointerTableHandle = pdptHandle;
	memcpy(fVirtualPageDirs, virtualPageDirs, sizeof(fVirtualPageDirs));
	memcpy(fPhysicalPageDirs, physicalPageDirs, sizeof(fPhysicalPageDirs));
}


void
X86PagingStructuresPAE::Delete()
{
	if (are_interrupts_enabled())
		delete this;
	else
		deferred_delete(this);
}


#endif	// B_HAIKU_PHYSICAL_BITS == 64
