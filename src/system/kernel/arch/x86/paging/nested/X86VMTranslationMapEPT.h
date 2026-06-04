/*
 * Copyright 2024, Daniel Martin, dalmemail@gmail.com
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_NESTED_X86_VM_TRANSLATION_MAP_EPT_H
#define KERNEL_ARCH_X86_PAGING_NESTED_X86_VM_TRANSLATION_MAP_EPT_H


#include "paging/X86VMTranslationMap.h"
#include "paging/64bit/X86PagingStructures64Bit.h"


struct X86VMTranslationMapEPT final : X86VMTranslationMap {
								X86VMTranslationMapEPT();
	virtual						~X86VMTranslationMapEPT();

			status_t			Init();

			void				SetFlushCallback(void (**callback)(void*), void* arg);
	virtual	void				Flush() final;

	virtual	size_t				MaxPagesNeededToMap(addr_t start,
									addr_t end) const;

	virtual	status_t			Map(addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									vm_page_reservation* reservation);
	virtual	status_t			Unmap(addr_t start, addr_t end);

	virtual	status_t			UnmapPage(VMArea* area, addr_t address,
									bool updatePageQueue,
									bool deletingAddressSpace, uint32* _flags);
	virtual	void				UnmapPages(VMArea* area, addr_t base,
									size_t size, bool updatePageQueue,
									bool deletingAddressSpace);

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

	virtual	X86PagingStructures* PagingStructures() const;

private:
	X86PagingStructures64Bit*	fPagingStructures;

	void (**fFlushCallback)(void*);
	void* fFlushCallbackArg;
};


#endif	// KERNEL_ARCH_X86_PAGING_NESTED_X86_VM_TRANSLATION_MAP_EPT_H
