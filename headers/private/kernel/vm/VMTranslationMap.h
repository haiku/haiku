/*
 * Copyright 2002-2010, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef KERNEL_VM_VM_TRANSLATION_MAP_H
#define KERNEL_VM_VM_TRANSLATION_MAP_H


#include <kernel.h>
#include <lock.h>

#include <vm/VMArea.h>


struct kernel_args;
struct vm_page_reservation;


struct VMTranslationMap {
								VMTranslationMap();
	virtual						~VMTranslationMap();

	virtual	bool	 			Lock() = 0;
	virtual	void				Unlock() = 0;

	virtual	addr_t				MappedSize() const = 0;
	virtual	size_t				MaxPagesNeededToMap(addr_t start,
									addr_t end) const = 0;

	virtual	status_t			Map(addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									vm_page_reservation* reservation) = 0;
	virtual	status_t			Unmap(addr_t start, addr_t end) = 0;

	virtual	status_t			DebugMarkRangePresent(addr_t start, addr_t end,
									bool markPresent);

	// map not locked
	virtual	status_t			UnmapPage(VMArea* area, addr_t address,
									bool updatePageQueue) = 0;
	virtual	void				UnmapPages(VMArea* area, addr_t base,
									size_t size, bool updatePageQueue);
	virtual	void				UnmapArea(VMArea* area,
									bool deletingAddressSpace,
									bool ignoreTopCachePageFlags);

	virtual	status_t			Query(addr_t virtualAddress,
									phys_addr_t* _physicalAddress,
									uint32* _flags) = 0;
	virtual	status_t			QueryInterrupt(addr_t virtualAddress,
									phys_addr_t* _physicalAddress,
									uint32* _flags) = 0;

	virtual	status_t			Protect(addr_t base, addr_t top,
									uint32 attributes, uint32 memoryType) = 0;
			status_t			ProtectPage(VMArea* area, addr_t address,
									uint32 attributes);
			status_t			ProtectArea(VMArea* area,
									uint32 attributes);

	virtual	status_t			ClearFlags(addr_t virtualAddress,
									uint32 flags) = 0;

	virtual	bool				ClearAccessedAndModified(
									VMArea* area, addr_t address,
									bool unmapIfUnaccessed,
									bool& _modified) = 0;

	virtual	void				Flush() = 0;

protected:
			void				PageUnmapped(VMArea* area,
									page_num_t pageNumber, bool accessed,
									bool modified, bool updatePageQueue);
			void				UnaccessedPageUnmapped(VMArea* area,
									page_num_t pageNumber);

protected:
			recursive_lock		fLock;
			int32				fMapCount;
};


struct VMPhysicalPageMapper {
								VMPhysicalPageMapper();
	virtual						~VMPhysicalPageMapper();

	// get/put virtual address for physical page -- will be usuable on all CPUs
	// (usually more expensive than the *_current_cpu() versions)
	virtual	status_t			GetPage(phys_addr_t physicalAddress,
									addr_t* _virtualAddress,
									void** _handle) = 0;
	virtual	status_t			PutPage(addr_t virtualAddress,
									void* handle) = 0;

	// get/put virtual address for physical page -- thread must be pinned the
	// whole time
	virtual	status_t			GetPageCurrentCPU(
									phys_addr_t physicalAddress,
									addr_t* _virtualAddress,
									void** _handle) = 0;
	virtual	status_t			PutPageCurrentCPU(addr_t virtualAddress,
									void* _handle) = 0;

	// get/put virtual address for physical in KDL
	virtual	status_t			GetPageDebug(phys_addr_t physicalAddress,
									addr_t* _virtualAddress,
									void** _handle) = 0;
	virtual	status_t			PutPageDebug(addr_t virtualAddress,
									void* handle) = 0;

	// memory operations on pages
	virtual	status_t			MemsetPhysical(phys_addr_t address, int value,
									phys_size_t length) = 0;
	virtual	status_t			MemcpyFromPhysical(void* to, phys_addr_t from,
									size_t length, bool user) = 0;
	virtual	status_t			MemcpyToPhysical(phys_addr_t to,
									const void* from, size_t length,
									bool user) = 0;
	virtual	void				MemcpyPhysicalPage(phys_addr_t to,
									phys_addr_t from) = 0;
};



inline status_t
VMTranslationMap::ProtectPage(VMArea* area, addr_t address, uint32 attributes)
{
	return Protect(address, address + B_PAGE_SIZE - 1, attributes,
		area->MemoryType());
}


#include <vm/VMArea.h>
inline status_t
VMTranslationMap::ProtectArea(VMArea* area, uint32 attributes)
{
	return Protect(area->Base(), area->Base() + area->Size() - 1, attributes,
		area->MemoryType());
}


#include <arch/vm_translation_map.h>

#endif	/* KERNEL_VM_VM_TRANSLATION_MAP_H */
