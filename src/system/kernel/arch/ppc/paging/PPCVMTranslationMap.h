/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_PPC_PPC_VM_TRANSLATION_MAP_H
#define KERNEL_ARCH_PPC_PPC_VM_TRANSLATION_MAP_H


#include <vm/VMTranslationMap.h>


#define PAGE_INVALIDATE_CACHE_SIZE 64


struct PPCPagingStructures;
class TranslationMapPhysicalPageMapper;


struct PPCVMTranslationMap : VMTranslationMap {
								PPCVMTranslationMap();
	virtual						~PPCVMTranslationMap();

			status_t			Init(bool kernel);

	virtual	bool 				Lock();
	virtual	void				Unlock();

	virtual	addr_t				MappedSize() const;

	virtual	void				Flush();

	virtual	PPCPagingStructures* PagingStructures() const = 0;

	inline	void				InvalidatePage(addr_t address);

	virtual status_t			RemapAddressRange(addr_t *_virtualAddress,
									size_t size, bool unmap) = 0;


	virtual void				ChangeASID() = 0;

protected:
			//X86:TranslationMapPhysicalPageMapper* fPageMapper;
			int					fInvalidPagesCount;
			addr_t				fInvalidPages[PAGE_INVALIDATE_CACHE_SIZE];
			bool				fIsKernelMap;
};


void
PPCVMTranslationMap::InvalidatePage(addr_t address)
{
	if (fInvalidPagesCount < PAGE_INVALIDATE_CACHE_SIZE)
		fInvalidPages[fInvalidPagesCount] = address;

	fInvalidPagesCount++;
}


#endif	// KERNEL_ARCH_PPC_PPC_VM_TRANSLATION_MAP_H
