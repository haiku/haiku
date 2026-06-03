/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "paging/nested/X86VMTranslationMapRVI.h"

#include "paging/64bit/X86PagingStructures64Bit.h"


//#define TRACE_X86_VM_TRANSLATION_MAP_RVI
#ifdef TRACE_X86_VM_TRANSLATION_MAP_RVI
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


// #pragma mark - X86VMTranslationMapRVI


X86VMTranslationMapRVI::X86VMTranslationMapRVI()
	:
	X86VMTranslationMap64Bit(false)
{
}


X86VMTranslationMapRVI::~X86VMTranslationMapRVI()
{
}


status_t
X86VMTranslationMapRVI::Init()
{
	TRACE("X86VMTranslationMapRVI::Init()\n");

	status_t status = X86VMTranslationMap64Bit::Init(false);
	if (status != B_OK)
		return status;

	// Clear the PMLTop, as we don't want the kernel mappings in there.
	memset(PagingStructures64Bit()->VirtualPMLTop(), 0, B_PAGE_SIZE);

	return B_OK;
}


status_t
X86VMTranslationMapRVI::Map(addr_t virtualAddress, phys_addr_t physicalAddress,
	uint32 attributes, uint32 memoryType, vm_page_reservation* reservation)
{
	// Always specify B_EXECUTE_AREA, so that the NX flag isn't set.
	attributes |= B_EXECUTE_AREA;

	return X86VMTranslationMap64Bit::Map(virtualAddress, physicalAddress,
		attributes, memoryType, reservation);
}


status_t
X86VMTranslationMapRVI::Protect(addr_t start, addr_t end, uint32 attributes,
	uint32 memoryType)
{
	ASSERT_UNREACHABLE();
	return B_ERROR;
}
