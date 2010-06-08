/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_32_BIT_X86_PAGING_METHOD_32_BIT_H
#define KERNEL_ARCH_X86_PAGING_32_BIT_X86_PAGING_METHOD_32_BIT_H


#include "paging/32bit/paging.h"
#include "paging/X86PagingMethod.h"
#include "paging/X86PagingStructures.h"


struct X86PagingStructures32Bit : X86PagingStructures {
	page_directory_entry*		pgdir_virt;

								X86PagingStructures32Bit();
	virtual						~X86PagingStructures32Bit();

	virtual	void				Delete();
};


class X86PagingMethod32Bit : public X86PagingMethod {
public:
								X86PagingMethod32Bit();
	virtual						~X86PagingMethod32Bit();

	static	X86PagingMethod*	Create();

	virtual	status_t			Init(kernel_args* args,
									VMPhysicalPageMapper** _physicalPageMapper);
	virtual	status_t			InitPostArea(kernel_args* args);

	virtual	status_t			CreateTranslationMap(bool kernel,
									VMTranslationMap** _map);

	virtual	status_t			MapEarly(kernel_args* args,
									addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint8 attributes,
									phys_addr_t (*get_free_page)(kernel_args*));

	virtual	bool				IsKernelPageAccessible(addr_t virtualAddress,
									uint32 protection);

private:
			struct PhysicalPageSlotPool;
};


#endif	// KERNEL_ARCH_X86_PAGING_32_BIT_X86_PAGING_METHOD_32_BIT_H
