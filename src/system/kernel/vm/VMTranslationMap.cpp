/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <vm/VMTranslationMap.h>

#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMArea.h>


// #pragma mark - VMTranslationMap


VMTranslationMap::VMTranslationMap()
	:
	fMapCount(0)
{
	recursive_lock_init(&fLock, "translation map");
}


VMTranslationMap::~VMTranslationMap()
{
	recursive_lock_destroy(&fLock);
}


/*!	Unmaps a range of pages of an area.

	The default implementation just iterates over all virtual pages of the
	range and calls UnmapPage(). This is obviously not particularly efficient.
*/
void
VMTranslationMap::UnmapPages(VMArea* area, addr_t base, size_t size,
	bool updatePageQueue)
{
	ASSERT(base % B_PAGE_SIZE == 0);
	ASSERT(size % B_PAGE_SIZE == 0);

	addr_t address = base;
	addr_t end = address + size;
#if DEBUG_PAGE_ACCESS
	for (; address != end; address += B_PAGE_SIZE) {
		phys_addr_t physicalAddress;
		uint32 flags;
		if (Query(address, &physicalAddress, &flags) == B_OK
			&& (flags & PAGE_PRESENT) != 0) {
			vm_page* page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
			if (page != NULL) {
				DEBUG_PAGE_ACCESS_START(page);
				UnmapPage(area, address, updatePageQueue);
				DEBUG_PAGE_ACCESS_END(page);
			} else
				UnmapPage(area, address, updatePageQueue);
		}
	}
#else
	for (; address != end; address += B_PAGE_SIZE)
		UnmapPage(area, address, updatePageQueue);
#endif
}


/*!	Unmaps all of an area's pages.
	If \a deletingAddressSpace is \c true, the address space the area belongs to
	is in the process of being destroyed and isn't used by anyone anymore. For
	some architectures this can be used for optimizations (e.g. not unmapping
	pages or at least not needing to invalidate TLB entries).
	If \a ignoreTopCachePageFlags is \c true, the area is in the process of
	being destroyed and its top cache is otherwise unreferenced. I.e. all mapped
	pages that live in the top cache area going to be freed and the page
	accessed and modified flags don't need to be propagated.

	The default implementation just iterates over all virtual pages of the
	area and calls UnmapPage(). This is obviously not particularly efficient.
*/
void
VMTranslationMap::UnmapArea(VMArea* area, bool deletingAddressSpace,
	bool ignoreTopCachePageFlags)
{
	addr_t address = area->Base();
	addr_t end = address + area->Size();
#if DEBUG_PAGE_ACCESS
	for (; address != end; address += B_PAGE_SIZE) {
		phys_addr_t physicalAddress;
		uint32 flags;
		if (Query(address, &physicalAddress, &flags) == B_OK
			&& (flags & PAGE_PRESENT) != 0) {
			vm_page* page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
			if (page != NULL) {
				DEBUG_PAGE_ACCESS_START(page);
				UnmapPage(area, address, true);
				DEBUG_PAGE_ACCESS_END(page);
			} else
				UnmapPage(area, address, true);
		}
	}
#else
	for (; address != end; address += B_PAGE_SIZE)
		UnmapPage(area, address, true);
#endif
}


// #pragma mark - VMPhysicalPageMapper


VMPhysicalPageMapper::VMPhysicalPageMapper()
{
}


VMPhysicalPageMapper::~VMPhysicalPageMapper()
{
}
