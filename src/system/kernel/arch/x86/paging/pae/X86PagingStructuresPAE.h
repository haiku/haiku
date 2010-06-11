/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_STRUCTURES_PAE_H
#define KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_STRUCTURES_PAE_H


#include "paging/pae/paging.h"
#include "paging/X86PagingStructures.h"


#if B_HAIKU_PHYSICAL_BITS == 64


struct X86PagingStructuresPAE : X86PagingStructures {
								X86PagingStructuresPAE();
	virtual						~X86PagingStructuresPAE();

			void				Init(pae_page_directory_pointer_table_entry*
										virtualPDPT,
									 phys_addr_t physicalPDPT, void* pdptHandle,
									 pae_page_directory_entry* const*
									 	virtualPageDirs,
									 const phys_addr_t* physicalPageDirs);

	virtual	void				Delete();

			pae_page_directory_entry* const* VirtualPageDirs() const
									{ return fVirtualPageDirs; }

private:
			pae_page_directory_pointer_table_entry* fPageDirPointerTable;
			void*				fPageDirPointerTableHandle;
			pae_page_directory_entry* fVirtualPageDirs[4];
			phys_addr_t			fPhysicalPageDirs[4];
};


#endif	// B_HAIKU_PHYSICAL_BITS == 64


#endif	// KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_STRUCTURES_PAE_H
