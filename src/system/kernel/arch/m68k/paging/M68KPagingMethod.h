/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_M68K_PAGING_M68K_PAGING_METHOD_H
#define KERNEL_ARCH_M68K_PAGING_M68K_PAGING_METHOD_H


#include <SupportDefs.h>


struct kernel_args;
struct VMPhysicalPageMapper;
struct VMTranslationMap;


class M68KPagingMethod {
public:
	virtual						~M68KPagingMethod();

	virtual	status_t			Init(kernel_args* args,
									VMPhysicalPageMapper** _physicalPageMapper)
										= 0;
	virtual	status_t			InitPostArea(kernel_args* args) = 0;

	virtual	status_t			CreateTranslationMap(bool kernel,
									VMTranslationMap** _map) = 0;

	virtual	status_t			MapEarly(kernel_args* args,
									addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint8 attributes,
									phys_addr_t (*get_free_page)(kernel_args*))
										= 0;

	virtual	bool				IsKernelPageAccessible(addr_t virtualAddress,
									uint32 protection) = 0;

	virtual void				SetPageRoot(uint32 pageRoot) = 0;
};


extern M68KPagingMethod* gM68KPagingMethod;


#endif	// KERNEL_ARCH_M68K_PAGING_M68K_PAGING_METHOD_H
