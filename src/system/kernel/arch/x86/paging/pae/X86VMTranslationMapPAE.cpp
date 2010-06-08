/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "paging/pae/X86VMTranslationMapPAE.h"

#include <vm/VMAddressSpace.h>
#include <vm/VMCache.h>

#include "paging/pae/X86PagingMethodPAE.h"
#include "paging/pae/X86PagingStructuresPAE.h"
#include "paging/x86_physical_page_mapper.h"


//#define TRACE_X86_VM_TRANSLATION_MAP_PAE
#ifdef TRACE_X86_VM_TRANSLATION_MAP_PAE
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


#if B_HAIKU_PHYSICAL_BITS == 64


X86VMTranslationMapPAE::X86VMTranslationMapPAE()
	:
	fPagingStructures(NULL)
{
}


X86VMTranslationMapPAE::~X86VMTranslationMapPAE()
{
// TODO: Implement!
}


status_t
X86VMTranslationMapPAE::Init(bool kernel)
{
	TRACE("X86VMTranslationMapPAE::Init()\n");

	X86VMTranslationMap::Init(kernel);

// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


size_t
X86VMTranslationMapPAE::MaxPagesNeededToMap(addr_t start, addr_t end) const
{
// TODO: Implement!
	panic("unsupported");
	return 0;
}


status_t
X86VMTranslationMapPAE::Map(addr_t va, phys_addr_t pa, uint32 attributes,
	uint32 memoryType, vm_page_reservation* reservation)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


status_t
X86VMTranslationMapPAE::Unmap(addr_t start, addr_t end)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


/*!	Caller must have locked the cache of the page to be unmapped.
	This object shouldn't be locked.
*/
status_t
X86VMTranslationMapPAE::UnmapPage(VMArea* area, addr_t address,
	bool updatePageQueue)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


void
X86VMTranslationMapPAE::UnmapPages(VMArea* area, addr_t base, size_t size,
	bool updatePageQueue)
{
// TODO: Implement!
	panic("unsupported");
}


void
X86VMTranslationMapPAE::UnmapArea(VMArea* area, bool deletingAddressSpace,
	bool ignoreTopCachePageFlags)
{
// TODO: Implement!
	panic("unsupported");
}


status_t
X86VMTranslationMapPAE::Query(addr_t va, phys_addr_t *_physical,
	uint32 *_flags)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


status_t
X86VMTranslationMapPAE::QueryInterrupt(addr_t va, phys_addr_t *_physical,
	uint32 *_flags)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


status_t
X86VMTranslationMapPAE::Protect(addr_t start, addr_t end, uint32 attributes,
	uint32 memoryType)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


status_t
X86VMTranslationMapPAE::ClearFlags(addr_t va, uint32 flags)
{
// TODO: Implement!
	panic("unsupported");
	return B_UNSUPPORTED;
}


bool
X86VMTranslationMapPAE::ClearAccessedAndModified(VMArea* area, addr_t address,
	bool unmapIfUnaccessed, bool& _modified)
{
// TODO: Implement!
	panic("unsupported");
	return false;
}


X86PagingStructures*
X86VMTranslationMapPAE::PagingStructures() const
{
	return fPagingStructures;
}


#endif	// B_HAIKU_PHYSICAL_BITS == 64
