/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_METHOD_64BIT_H
#define KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_METHOD_64BIT_H


#include <KernelExport.h>

#include <lock.h>
#include <vm/vm_types.h>

#include "paging/64bit/paging.h"
#include "paging/X86PagingMethod.h"
#include "paging/X86PagingStructures.h"


class TranslationMapPhysicalPageMapper;
class X86PhysicalPageMapper;


class X86PagingMethod64Bit : public X86PagingMethod {
public:
								X86PagingMethod64Bit();
	virtual						~X86PagingMethod64Bit();

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

};


#endif	// KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_METHOD_64BIT_H
