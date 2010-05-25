/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_PHYSICAL_PAGE_MAPPER_H
#define _KERNEL_ARCH_X86_PHYSICAL_PAGE_MAPPER_H


#include <vm/VMTranslationMap.h>

#include "x86_paging.h"


struct kernel_args;
struct vm_translation_map_ops;


class TranslationMapPhysicalPageMapper {
public:
	virtual						~TranslationMapPhysicalPageMapper();

	virtual	void				Delete() = 0;

	virtual	page_table_entry*	GetPageTableAt(phys_addr_t physicalAddress) = 0;
		// Must be invoked with thread pinned to current CPU.
};


class X86PhysicalPageMapper : public VMPhysicalPageMapper {
public:
	virtual						~X86PhysicalPageMapper();

	virtual	status_t			InitPostArea(kernel_args* args) = 0;

	virtual	status_t			CreateTranslationMapPhysicalPageMapper(
									TranslationMapPhysicalPageMapper** _mapper)
										= 0;

	virtual	page_table_entry*	InterruptGetPageTableAt(
									phys_addr_t physicalAddress) = 0;
};


status_t large_memory_physical_page_ops_init(kernel_args* args,
	X86PhysicalPageMapper*& _pageMapper,
	TranslationMapPhysicalPageMapper*& _kernelPageMapper);


#endif	// _KERNEL_ARCH_X86_PHYSICAL_PAGE_MAPPER_H
