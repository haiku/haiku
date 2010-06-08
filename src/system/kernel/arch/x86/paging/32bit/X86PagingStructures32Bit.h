/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_32_BIT_X86_PAGING_STRUCTURES_32_BIT_H
#define KERNEL_ARCH_X86_PAGING_32_BIT_X86_PAGING_STRUCTURES_32_BIT_H


#include "paging/32bit/paging.h"
#include "paging/X86PagingStructures.h"


struct X86PagingStructures32Bit : X86PagingStructures {
	page_directory_entry*		pgdir_virt;

								X86PagingStructures32Bit();
	virtual						~X86PagingStructures32Bit();

			void				Init(page_directory_entry* virtualPageDir,
									 phys_addr_t physicalPageDir,
									 page_directory_entry* kernelPageDir);

	virtual	void				Delete();

	static	void				StaticInit();
	static	void				UpdateAllPageDirs(int index,
									page_directory_entry entry);
};


#endif	// KERNEL_ARCH_X86_PAGING_32_BIT_X86_PAGING_STRUCTURES_32_BIT_H
