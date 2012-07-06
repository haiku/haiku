/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_STRUCTURES_64BIT_H
#define KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_STRUCTURES_64BIT_H


#include "paging/64bit/paging.h"
#include "paging/X86PagingStructures.h"


struct X86PagingStructures64Bit : X86PagingStructures {
								X86PagingStructures64Bit();
	virtual						~X86PagingStructures64Bit();

			void				Init(uint64* virtualPML4,
									phys_addr_t physicalPML4);

	virtual	void				Delete();

			uint64*				VirtualPML4()
									{ return fVirtualPML4; }

private:
			uint64*				fVirtualPML4;
};


#endif	// KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_STRUCTURES_64BIT_H
