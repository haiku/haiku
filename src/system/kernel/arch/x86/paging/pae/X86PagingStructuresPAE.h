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

			void				Init();

	virtual	void				Delete();

	static	void				StaticInit();
};


#endif	// B_HAIKU_PHYSICAL_BITS == 64


#endif	// KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_STRUCTURES_PAE_H
