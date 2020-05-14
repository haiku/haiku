/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_X86_VM_TRANSLATION_MAP_H
#define KERNEL_ARCH_X86_X86_VM_TRANSLATION_MAP_H


#include <vm/VMTranslationMap.h>


#if __GNUC__ < 4
#define final
#endif


#define PAGE_INVALIDATE_CACHE_SIZE 64


struct X86PagingStructures;
class TranslationMapPhysicalPageMapper;


struct X86VMTranslationMap : VMTranslationMap {
								X86VMTranslationMap();
	virtual						~X86VMTranslationMap();

			status_t			Init(bool kernel);

	virtual	bool				Lock() final;
	virtual	void				Unlock() final;

	virtual	addr_t				MappedSize() const final;

	virtual	void				Flush() final;

	virtual	X86PagingStructures* PagingStructures() const = 0;

	inline	void				InvalidatePage(addr_t address);

protected:
			TranslationMapPhysicalPageMapper* fPageMapper;
			int					fInvalidPagesCount;
			addr_t				fInvalidPages[PAGE_INVALIDATE_CACHE_SIZE];
			bool				fIsKernelMap;
};


void
X86VMTranslationMap::InvalidatePage(addr_t address)
{
	if (fInvalidPagesCount < PAGE_INVALIDATE_CACHE_SIZE)
		fInvalidPages[fInvalidPagesCount] = address;

	fInvalidPagesCount++;
}


#endif	// KERNEL_ARCH_X86_X86_VM_TRANSLATION_MAP_H
