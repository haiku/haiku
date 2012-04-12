/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_M68K_M68K_VM_TRANSLATION_MAP_H
#define KERNEL_ARCH_M68K_M68K_VM_TRANSLATION_MAP_H


#include <vm/VMTranslationMap.h>


#define PAGE_INVALIDATE_CACHE_SIZE 64


struct M68KPagingStructures;
class TranslationMapPhysicalPageMapper;


struct M68KVMTranslationMap : VMTranslationMap {
								M68KVMTranslationMap();
	virtual						~M68KVMTranslationMap();

			status_t			Init(bool kernel);

	virtual	bool 				Lock();
	virtual	void				Unlock();

	virtual	addr_t				MappedSize() const;

	virtual	void				Flush();

	virtual	M68KPagingStructures* PagingStructures() const = 0;

	inline	void				InvalidatePage(addr_t address);

protected:
			TranslationMapPhysicalPageMapper* fPageMapper;
			int					fInvalidPagesCount;
			addr_t				fInvalidPages[PAGE_INVALIDATE_CACHE_SIZE];
			bool				fIsKernelMap;
};


void
M68KVMTranslationMap::InvalidatePage(addr_t address)
{
	if (fInvalidPagesCount < PAGE_INVALIDATE_CACHE_SIZE)
		fInvalidPages[fInvalidPagesCount] = address;

	fInvalidPagesCount++;
}


#endif	// KERNEL_ARCH_M68K_M68K_VM_TRANSLATION_MAP_H
