/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_M68K_PAGING_M68K_PHYSICAL_PAGE_MAPPER_LARGE_MEMORY_H
#define KERNEL_ARCH_M68K_PAGING_M68K_PHYSICAL_PAGE_MAPPER_LARGE_MEMORY_H


#include <OS.h>

#include <util/DoublyLinkedList.h>


class TranslationMapPhysicalPageMapper;
class M68KPhysicalPageMapper;
struct kernel_args;


namespace M68KLargePhysicalPageMapper {


struct PhysicalPageSlotPool;


struct PhysicalPageSlot {
	PhysicalPageSlot*			next;
	PhysicalPageSlotPool*		pool;
	addr_t						address;

	inline	void				Map(phys_addr_t physicalAddress);
};


struct PhysicalPageSlotPool : DoublyLinkedListLinkImpl<PhysicalPageSlotPool> {

	virtual						~PhysicalPageSlotPool();

	inline	bool				IsEmpty() const;

	inline	PhysicalPageSlot*	GetSlot();
	inline	void				PutSlot(PhysicalPageSlot* slot);

	virtual	status_t			AllocatePool(PhysicalPageSlotPool*& _pool) = 0;
	virtual	void				Map(phys_addr_t physicalAddress,
									addr_t virtualAddress) = 0;

protected:
			PhysicalPageSlot*	fSlots;
};


}


status_t large_memory_physical_page_ops_init(kernel_args* args,
	M68KLargePhysicalPageMapper::PhysicalPageSlotPool* initialPool,
	M68KPhysicalPageMapper*& _pageMapper,
	TranslationMapPhysicalPageMapper*& _kernelPageMapper);


#endif	// KERNEL_ARCH_M68K_PAGING_M68K_PHYSICAL_PAGE_MAPPER_LARGE_MEMORY_H
