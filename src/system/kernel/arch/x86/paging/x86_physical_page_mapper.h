/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_X86_PHYSICAL_PAGE_MAPPER_H
#define KERNEL_ARCH_X86_PAGING_X86_PHYSICAL_PAGE_MAPPER_H


#include <vm/VMTranslationMap.h>


struct kernel_args;


class TranslationMapPhysicalPageMapper {
public:
	virtual						~TranslationMapPhysicalPageMapper();

	virtual	void				Delete() = 0;

	virtual	void*				GetPageTableAt(phys_addr_t physicalAddress) = 0;
		// Must be invoked with thread pinned to current CPU.
};


class X86PhysicalPageMapper : public VMPhysicalPageMapper {
public:
	virtual						~X86PhysicalPageMapper();

	virtual	status_t			CreateTranslationMapPhysicalPageMapper(
									TranslationMapPhysicalPageMapper** _mapper)
										= 0;

	virtual	void*				InterruptGetPageTableAt(
									phys_addr_t physicalAddress) = 0;
};


#endif	// KERNEL_ARCH_X86_PAGING_X86_PHYSICAL_PAGE_MAPPER_H
