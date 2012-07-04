/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Distributed under the terms of the MIT License.
 */


#include "paging/64bit/X86PagingMethod64Bit.h"

#include <stdlib.h>
#include <string.h>

#include <boot/kernel_args.h>
#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/VMAddressSpace.h>

//#include "paging/64bit/X86PagingStructures64Bit.h"
//#include "paging/64bit/X86VMTranslationMap64Bit.h"
#include "paging/x86_physical_page_mapper.h"
#include "paging/x86_physical_page_mapper_mapped.h"


#define TRACE_X86_PAGING_METHOD_64BIT
#ifdef TRACE_X86_PAGING_METHOD_64BIT
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


// #pragma mark - X86PagingMethod64Bit


X86PagingMethod64Bit::X86PagingMethod64Bit()
{
}


X86PagingMethod64Bit::~X86PagingMethod64Bit()
{
}


status_t
X86PagingMethod64Bit::Init(kernel_args* args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	return B_ERROR;
}


status_t
X86PagingMethod64Bit::InitPostArea(kernel_args* args)
{
	return B_ERROR;
}


status_t
X86PagingMethod64Bit::CreateTranslationMap(bool kernel, VMTranslationMap** _map)
{
	return B_ERROR;
}


status_t
X86PagingMethod64Bit::MapEarly(kernel_args* args, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	phys_addr_t (*get_free_page)(kernel_args*))
{
	return B_ERROR;
}


bool
X86PagingMethod64Bit::IsKernelPageAccessible(addr_t virtualAddress,
	uint32 protection)
{
	return true;
}

