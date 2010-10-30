/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_ARM_PAGING_32_BIT_ARM_PAGING_STRUCTURES_32_BIT_H
#define KERNEL_ARCH_ARM_PAGING_32_BIT_ARM_PAGING_STRUCTURES_32_BIT_H


#include "paging/32bit/paging.h"
#include "paging/ARMPagingStructures.h"


struct ARMPagingStructures32Bit : ARMPagingStructures {
	page_directory_entry*		pgdir_virt;

								ARMPagingStructures32Bit();
	virtual						~ARMPagingStructures32Bit();

			void				Init(page_directory_entry* virtualPageDir,
									 phys_addr_t physicalPageDir,
									 page_directory_entry* kernelPageDir);

	virtual	void				Delete();

	static	void				StaticInit();
	static	void				UpdateAllPageDirs(int index,
									page_directory_entry entry);
};


#endif	// KERNEL_ARCH_ARM_PAGING_32_BIT_ARM_PAGING_STRUCTURES_32_BIT_H
