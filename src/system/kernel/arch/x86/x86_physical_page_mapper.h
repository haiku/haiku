/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_X86_PHYSICAL_PAGE_MAPPER_H
#define _KERNEL_ARCH_X86_PHYSICAL_PAGE_MAPPER_H

#include <SupportDefs.h>


struct kernel_args;
struct page_table_entry;
struct vm_translation_map_ops;


class TranslationMapPhysicalPageMapper {
public:
	virtual						~TranslationMapPhysicalPageMapper();

	virtual	void				Delete() = 0;

	virtual	page_table_entry*	GetPageTableAt(addr_t physicalAddress) = 0;
		// Must be invoked with thread pinned to current CPU.
};


class PhysicalPageMapper {
public:
	virtual						~PhysicalPageMapper();

	virtual	status_t			InitPostArea(kernel_args* args) = 0;

	virtual	status_t			CreateTranslationMapPhysicalPageMapper(
									TranslationMapPhysicalPageMapper** _mapper)
										= 0;

	virtual	page_table_entry*	InterruptGetPageTableAt(
									addr_t physicalAddress) = 0;
};

extern PhysicalPageMapper* gPhysicalPageMapper;
extern TranslationMapPhysicalPageMapper* gKernelPhysicalPageMapper;


status_t large_memory_physical_page_ops_init(kernel_args* args,
	vm_translation_map_ops* ops);


#endif	// _KERNEL_ARCH_X86_PHYSICAL_PAGE_MAPPER_H
