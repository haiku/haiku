/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "paging/pae/X86PagingMethodPAE.h"

#include <stdlib.h>
#include <string.h>

#include "paging/pae/X86PagingStructuresPAE.h"
#include "paging/pae/X86VMTranslationMapPAE.h"
#include "paging/x86_physical_page_mapper.h"
#include "paging/x86_physical_page_mapper_large_memory.h"


//#define TRACE_X86_PAGING_METHOD_PAE
#ifdef TRACE_X86_PAGING_METHOD_PAE
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


#if B_HAIKU_PHYSICAL_BITS == 64


using X86LargePhysicalPageMapper::PhysicalPageSlot;


// #pragma mark - X86PagingMethodPAE::PhysicalPageSlotPool


struct X86PagingMethodPAE::PhysicalPageSlotPool
	: X86LargePhysicalPageMapper::PhysicalPageSlotPool {
public:
	virtual						~PhysicalPageSlotPool();

			status_t			InitInitial(kernel_args* args);
			status_t			InitInitialPostArea(kernel_args* args);

	virtual	status_t			AllocatePool(
									X86LargePhysicalPageMapper
										::PhysicalPageSlotPool*& _pool);
	virtual	void				Map(phys_addr_t physicalAddress,
									addr_t virtualAddress);

public:
	static	PhysicalPageSlotPool sInitialPhysicalPagePool;

private:
};


X86PagingMethodPAE::PhysicalPageSlotPool
	X86PagingMethodPAE::PhysicalPageSlotPool::sInitialPhysicalPagePool;


X86PagingMethodPAE::PhysicalPageSlotPool::~PhysicalPageSlotPool()
{
}


status_t
X86PagingMethodPAE::PhysicalPageSlotPool::InitInitial(kernel_args* args)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


status_t
X86PagingMethodPAE::PhysicalPageSlotPool::InitInitialPostArea(
	kernel_args* args)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


void
X86PagingMethodPAE::PhysicalPageSlotPool::Map(phys_addr_t physicalAddress,
	addr_t virtualAddress)
{
// TODO: Implement!
	panic("unsupported");
}


status_t
X86PagingMethodPAE::PhysicalPageSlotPool::AllocatePool(
	X86LargePhysicalPageMapper::PhysicalPageSlotPool*& _pool)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


// #pragma mark - X86PagingMethodPAE


X86PagingMethodPAE::X86PagingMethodPAE()
	:
	fPhysicalPageMapper(NULL),
	fKernelPhysicalPageMapper(NULL)
{
}


X86PagingMethodPAE::~X86PagingMethodPAE()
{
}


status_t
X86PagingMethodPAE::Init(kernel_args* args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


status_t
X86PagingMethodPAE::InitPostArea(kernel_args* args)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


status_t
X86PagingMethodPAE::CreateTranslationMap(bool kernel, VMTranslationMap** _map)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


status_t
X86PagingMethodPAE::MapEarly(kernel_args* args, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	phys_addr_t (*get_free_page)(kernel_args*))
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


bool
X86PagingMethodPAE::IsKernelPageAccessible(addr_t virtualAddress,
	uint32 protection)
{
// TODO: Implement!
	return false;
}


#endif	// B_HAIKU_PHYSICAL_BITS == 64
