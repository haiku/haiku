/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_PPC_PAGING_CLASSIC_PPC_VM_TRANSLATION_MAP_CLASSIC_H
#define KERNEL_ARCH_PPC_PAGING_CLASSIC_PPC_VM_TRANSLATION_MAP_CLASSIC_H


#include "paging/PPCVMTranslationMap.h"
#include <arch_mmu.h>


struct PPCPagingStructuresClassic;


struct PPCVMTranslationMapClassic : PPCVMTranslationMap {
								PPCVMTranslationMapClassic();
	virtual						~PPCVMTranslationMapClassic();

			status_t			Init(bool kernel);

	inline	int					VSIDBase() const	{ return fVSIDBase; }

	virtual void				ChangeASID();

			page_table_entry*	LookupPageTableEntry(addr_t virtualAddress);
			bool				RemovePageTableEntry(addr_t virtualAddress);

	virtual	size_t				MaxPagesNeededToMap(addr_t start,
									addr_t end) const;

	virtual	status_t			Map(addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									vm_page_reservation* reservation);
	virtual	status_t			Unmap(addr_t start, addr_t end);

	virtual status_t			RemapAddressRange(addr_t *_virtualAddress,
									size_t size, bool unmap);

	virtual	status_t			DebugMarkRangePresent(addr_t start, addr_t end,
									bool markPresent);

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

	virtual	PPCPagingStructures* PagingStructures() const;
	inline	PPCPagingStructuresClassic* PagingStructuresClassic() const
									{ return fPagingStructures; }

private:
			PPCPagingStructuresClassic* fPagingStructures;
			//XXX:move to fPagingStructures?
			int					fVSIDBase;
};


#endif	// KERNEL_ARCH_PPC_PAGING_CLASSIC_PPC_VM_TRANSLATION_MAP_CLASSIC_H
