/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_METHOD_PAE_H
#define KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_METHOD_PAE_H


#include "paging/pae/paging.h"
#include "paging/X86PagingMethod.h"
#include "paging/X86PagingStructures.h"


#if B_HAIKU_PHYSICAL_BITS == 64


class TranslationMapPhysicalPageMapper;
class X86PhysicalPageMapper;


class X86PagingMethodPAE : public X86PagingMethod {
public:
								X86PagingMethodPAE();
	virtual						~X86PagingMethodPAE();

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

	inline	X86PhysicalPageMapper* PhysicalPageMapper() const
									{ return fPhysicalPageMapper; }
	inline	TranslationMapPhysicalPageMapper* KernelPhysicalPageMapper() const
									{ return fKernelPhysicalPageMapper; }

	static	X86PagingMethodPAE*	Method();

private:
			struct PhysicalPageSlotPool;
			friend struct PhysicalPageSlotPool;

private:
			X86PhysicalPageMapper* fPhysicalPageMapper;
			TranslationMapPhysicalPageMapper* fKernelPhysicalPageMapper;
};


/*static*/ inline X86PagingMethodPAE*
X86PagingMethodPAE::Method()
{
	return static_cast<X86PagingMethodPAE*>(gX86PagingMethod);
}


#endif	// B_HAIKU_PHYSICAL_BITS == 64


#endif	// KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_METHOD_PAE_H
