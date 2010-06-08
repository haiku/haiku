/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_32_BIT_X86_PAGING_METHOD_32_BIT_H
#define KERNEL_ARCH_X86_PAGING_32_BIT_X86_PAGING_METHOD_32_BIT_H


#include "paging/32bit/paging.h"
#include "paging/X86PagingMethod.h"
#include "paging/X86PagingStructures.h"


class TranslationMapPhysicalPageMapper;
class X86PhysicalPageMapper;


class X86PagingMethod32Bit : public X86PagingMethod {
public:
								X86PagingMethod32Bit();
	virtual						~X86PagingMethod32Bit();

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

	inline	page_table_entry*	PageHole() const
									{ return fPageHole; }
	inline	page_directory_entry* PageHolePageDir() const
									{ return fPageHolePageDir; }
	inline	uint32				KernelPhysicalPageDirectory() const
									{ return fKernelPhysicalPageDirectory; }
	inline	page_directory_entry* KernelVirtualPageDirectory() const
									{ return fKernelVirtualPageDirectory; }
	inline	X86PhysicalPageMapper* PhysicalPageMapper() const
									{ return fPhysicalPageMapper; }
	inline	TranslationMapPhysicalPageMapper* KernelPhysicalPageMapper() const
									{ return fKernelPhysicalPageMapper; }

	static	X86PagingMethod32Bit* Method();

private:
			struct PhysicalPageSlotPool;

private:
			page_table_entry*	fPageHole;
			page_directory_entry* fPageHolePageDir;
			uint32				fKernelPhysicalPageDirectory;
			page_directory_entry* fKernelVirtualPageDirectory;

			X86PhysicalPageMapper* fPhysicalPageMapper;
			TranslationMapPhysicalPageMapper* fKernelPhysicalPageMapper;
};


/*static*/ inline X86PagingMethod32Bit*
X86PagingMethod32Bit::Method()
{
	return static_cast<X86PagingMethod32Bit*>(gX86PagingMethod);
}


#endif	// KERNEL_ARCH_X86_PAGING_32_BIT_X86_PAGING_METHOD_32_BIT_H
