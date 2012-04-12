/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_M68K_PAGING_040_M68K_PAGING_STRUCTURES_040_H
#define KERNEL_ARCH_M68K_PAGING_040_M68K_PAGING_STRUCTURES_040_H


#include "paging/040/paging.h"
#include "paging/M68KPagingStructures.h"


struct M68KPagingStructures040 : M68KPagingStructures {
	page_root_entry*		pgroot_virt;

								M68KPagingStructures040();
	virtual						~M68KPagingStructures040();

			void				Init(page_root_entry* virtualPageRoot,
									 phys_addr_t physicalPageRoot,
									 page_root_entry* kernelPageRoot);

	virtual	void				Delete();

	static	void				StaticInit();
	static	void				UpdateAllPageDirs(int index,
									page_directory_entry entry);
};


#endif	// KERNEL_ARCH_M68K_PAGING_040_M68K_PAGING_STRUCTURES_040_H
