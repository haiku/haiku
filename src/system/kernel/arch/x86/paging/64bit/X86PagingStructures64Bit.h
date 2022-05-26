/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_STRUCTURES_64BIT_H
#define KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_STRUCTURES_64BIT_H


#include "paging/64bit/paging.h"
#include "paging/X86PagingStructures.h"


struct X86PagingStructures64Bit final : X86PagingStructures {
								X86PagingStructures64Bit();
	virtual						~X86PagingStructures64Bit();

			void				Init(uint64* virtualPMLTop,
									phys_addr_t physicalPMLTop);

	virtual	void				Delete();

			uint64*				VirtualPMLTop()
									{ return fVirtualPMLTop; }

private:
			uint64*				fVirtualPMLTop;
};


#endif	// KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_STRUCTURES_64BIT_H
