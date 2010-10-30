/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_ARM_PAGING_32_BIT_ARM_VM_TRANSLATION_MAP_32_BIT_H
#define KERNEL_ARCH_ARM_PAGING_32_BIT_ARM_VM_TRANSLATION_MAP_32_BIT_H


#include "paging/ARMVMTranslationMap.h"


struct ARMPagingStructures32Bit;


struct ARMVMTranslationMap32Bit : ARMVMTranslationMap {
								ARMVMTranslationMap32Bit();
	virtual						~ARMVMTranslationMap32Bit();

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

	virtual	ARMPagingStructures* PagingStructures() const;
	inline	ARMPagingStructures32Bit* PagingStructures32Bit() const
									{ return fPagingStructures; }

private:
			ARMPagingStructures32Bit* fPagingStructures;
};


#endif	// KERNEL_ARCH_ARM_PAGING_32_BIT_ARM_VM_TRANSLATION_MAP_32_BIT_H
