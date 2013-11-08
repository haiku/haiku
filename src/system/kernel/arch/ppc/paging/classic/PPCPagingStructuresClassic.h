/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_PPC_PAGING_CLASSIC_PPC_PAGING_STRUCTURES_CLASSIC_H
#define KERNEL_ARCH_PPC_PAGING_CLASSIC_PPC_PAGING_STRUCTURES_CLASSIC_H


//#include "paging/classic/paging.h"
#include <arch_mmu.h>
#include "paging/PPCPagingStructures.h"


struct PPCPagingStructuresClassic : PPCPagingStructures {
//	page_directory_entry*		pgdir_virt;

								PPCPagingStructuresClassic();
	virtual						~PPCPagingStructuresClassic();

			void				Init(/*page_directory_entry* virtualPageDir,
									 phys_addr_t physicalPageDir,
									 page_directory_entry* kernelPageDir,*/
									page_table_entry_group *pageTable);

	virtual	void				Delete();

	static	void				StaticInit();
	static	void				UpdateAllPageDirs(int index,
									page_table_entry_group entry);
};


#endif	// KERNEL_ARCH_PPC_PAGING_CLASSIC_PPC_PAGING_STRUCTURES_CLASSIC_H
