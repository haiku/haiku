/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_X86_VM_TRANSLATION_MAP_H
#define KERNEL_ARCH_X86_X86_VM_TRANSLATION_MAP_H


#include <vm/VMTranslationMap.h>


#define PAGE_INVALIDATE_CACHE_SIZE 64


struct X86VMTranslationMap : VMTranslationMap {
								X86VMTranslationMap();
	virtual						~X86VMTranslationMap();

			status_t			Init(bool kernel);

	inline	vm_translation_map_arch_info* ArchData() const
									{ return fArchData; }
	inline	uint32				PhysicalPageDir() const
									{ return fArchData->pgdir_phys; }

	virtual	status_t			InitPostSem();

	virtual	bool 				Lock();
	virtual	void				Unlock();

	virtual	addr_t				MappedSize() const;
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

	virtual	void				Flush();

protected:
			vm_translation_map_arch_info* fArchData;
			TranslationMapPhysicalPageMapper* fPageMapper;
			int					fInvalidPagesCount;
			addr_t				fInvalidPages[PAGE_INVALIDATE_CACHE_SIZE];
};


#endif	// KERNEL_ARCH_X86_X86_VM_TRANSLATION_MAP_H
