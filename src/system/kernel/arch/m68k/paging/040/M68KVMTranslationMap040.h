/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_M68K_PAGING_040_M68K_VM_TRANSLATION_MAP_040_H
#define KERNEL_ARCH_M68K_PAGING_040_M68K_VM_TRANSLATION_MAP_040_H


#include "paging/M68KVMTranslationMap.h"

struct M68KPagingStructures040;


struct M68KVMTranslationMap040 : M68KVMTranslationMap {
								M68KVMTranslationMap040();
	virtual						~M68KVMTranslationMap040();

			status_t			Init(bool kernel);

	virtual	size_t				MaxPagesNeededToMap(addr_t start,
									addr_t end) const;

	virtual	status_t			Map(addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									vm_page_reservation* reservation);
	virtual	status_t			Unmap(addr_t start, addr_t end);

	virtual	status_t			UnmapPage(VMArea* area, addr_t address,
									bool updatePageQueue);
	virtual	void				UnmapPages(VMArea* area, addr_t base,
									size_t size, bool updatePageQueue);
	virtual	void				UnmapArea(VMArea* area,
									bool deletingAddressSpace,
									bool ignoreTopCachePageFlags);

	virtual	status_t			Query(addr_t virtualAddress,
									phys_addr_t* _physicalAddress,
									uint32* _flags);
	virtual	status_t			QueryInterrupt(addr_t virtualAddress,
									phys_addr_t* _physicalAddress,
									uint32* _flags);

	virtual	status_t			Protect(addr_t base, addr_t top,
									uint32 attributes, uint32 memoryType);

	virtual	status_t			ClearFlags(addr_t virtualAddress,
									uint32 flags);

	virtual	bool				ClearAccessedAndModified(
									VMArea* area, addr_t address,
									bool unmapIfUnaccessed,
									bool& _modified);

	virtual	M68KPagingStructures* PagingStructures() const;
	inline	M68KPagingStructures040* PagingStructures040() const
									{ return fPagingStructures; }

	void*						MapperGetPageTableAt(
									phys_addr_t physicalAddress,
									bool indirect=false);

private:
			M68KPagingStructures040* fPagingStructures;
};


#endif	// KERNEL_ARCH_M68K_PAGING_040_M68K_VM_TRANSLATION_MAP_040_H
