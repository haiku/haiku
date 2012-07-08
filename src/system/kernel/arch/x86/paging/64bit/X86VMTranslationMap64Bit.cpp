/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/64bit/X86VMTranslationMap64Bit.h"

#include <int.h>
#include <slab/Slab.h>
#include <thread.h>
#include <util/AutoLock.h>
#include <vm/vm_page.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMCache.h>

#include "paging/64bit/X86PagingMethod64Bit.h"
#include "paging/64bit/X86PagingStructures64Bit.h"
#include "paging/x86_physical_page_mapper.h"


//#define TRACE_X86_VM_TRANSLATION_MAP_64BIT
#ifdef TRACE_X86_VM_TRANSLATION_MAP_64BIT
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


// #pragma mark - X86VMTranslationMap64Bit


X86VMTranslationMap64Bit::X86VMTranslationMap64Bit()
	:
	fPagingStructures(NULL)
{
}


X86VMTranslationMap64Bit::~X86VMTranslationMap64Bit()
{
	TRACE("X86VMTranslationMap64Bit::~X86VMTranslationMap64Bit()\n");

	panic("X86VMTranslationMap64Bit::~X86VMTranslationMap64Bit: TODO");
}


status_t
X86VMTranslationMap64Bit::Init(bool kernel)
{
	TRACE("X86VMTranslationMap64Bit::Init()\n");

	X86VMTranslationMap::Init(kernel);

	fPagingStructures = new(std::nothrow) X86PagingStructures64Bit;
	if (fPagingStructures == NULL)
		return B_NO_MEMORY;

	X86PagingMethod64Bit* method = X86PagingMethod64Bit::Method();

	if (kernel) {
		// Get the page mapper.
		fPageMapper = method->KernelPhysicalPageMapper();

		// Kernel PML4 is already mapped.
		fPagingStructures->Init(method->KernelVirtualPML4(),
			method->KernelPhysicalPML4());
	} else {
		panic("X86VMTranslationMap64Bit::Init(): TODO");
	}

	return B_OK;
}


size_t
X86VMTranslationMap64Bit::MaxPagesNeededToMap(addr_t start, addr_t end) const
{
	// If start == 0, the actual base address is not yet known to the caller and
	// we shall assume the worst case, which is where the start address is the
	// last page covered by a PDPT.
	if (start == 0) {
		start = k64BitPDPTRange - B_PAGE_SIZE;
		end += start;
	}

	size_t requiredPDPTs = end / k64BitPDPTRange + 1
		- start / k64BitPDPTRange;
	size_t requiredPageDirs = end / k64BitPageDirectoryRange + 1
		- start / k64BitPageDirectoryRange;
	size_t requiredPageTables = end / k64BitPageTableRange + 1
		- start / k64BitPageTableRange;

	return requiredPDPTs + requiredPageDirs + requiredPageTables;
}


status_t
X86VMTranslationMap64Bit::Map(addr_t virtualAddress, phys_addr_t physicalAddress,
	uint32 attributes, uint32 memoryType, vm_page_reservation* reservation)
{
	TRACE("X86VMTranslationMap64Bit::Map(%#" B_PRIxADDR ", %#" B_PRIxPHYSADDR
		")\n", virtualAddress, physicalAddress);

	ThreadCPUPinner pinner(thread_get_current_thread());

	// Look up the page table for the virtual address, allocating new tables
	// if required. Shouldn't fail.
	uint64* entry = X86PagingMethod64Bit::PageTableEntryForAddress(
		fPagingStructures->VirtualPML4(), virtualAddress, fIsKernelMap,
		true, reservation, fPageMapper, fMapCount);
	ASSERT(entry != NULL);

	// The entry should not already exist.
	ASSERT_PRINT((*entry & X86_64_PTE_PRESENT) == 0,
		"virtual address: %#" B_PRIxADDR ", existing pte: %#" B_PRIx64,
		virtualAddress, *entry);

	// Fill in the table entry.
	X86PagingMethod64Bit::PutPageTableEntryInTable(entry, physicalAddress,
		attributes, memoryType, fIsKernelMap);

	// Note: We don't need to invalidate the TLB for this address, as previously
	// the entry was not present and the TLB doesn't cache those entries.

	fMapCount++;

	return 0;
}


status_t
X86VMTranslationMap64Bit::Unmap(addr_t start, addr_t end)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("X86VMTranslationMap64Bit::Unmap(%#" B_PRIxADDR ", %#" B_PRIxADDR
		")\n", start, end);

	ThreadCPUPinner pinner(thread_get_current_thread());

	do {
		uint64* pageTable = X86PagingMethod64Bit::PageTableForAddress(
			fPagingStructures->VirtualPML4(), start, fIsKernelMap, false,
			NULL, fPageMapper, fMapCount);
		if (pageTable == NULL) {
			// Move on to the next page table.
			start = ROUNDUP(start + 1, k64BitPageTableRange);
			continue;
		}

		for (uint32 index = start / B_PAGE_SIZE % k64BitTableEntryCount;
				index < k64BitTableEntryCount && start < end;
				index++, start += B_PAGE_SIZE) {
			if ((pageTable[index] & X86_64_PTE_PRESENT) == 0)
				continue;

			TRACE("X86VMTranslationMap64Bit::Unmap(): removing page %#"
				B_PRIxADDR " (%#" B_PRIxPHYSADDR ")\n", start,
				pageTable[index] & X86_64_PTE_ADDRESS_MASK);

			uint64 oldEntry = X86PagingMethod64Bit::ClearTableEntryFlags(
				&pageTable[index], X86_64_PTE_PRESENT);
			fMapCount--;

			if ((oldEntry & X86_64_PTE_ACCESSED) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flags was set, since only then the entry could have
				// been in any TLB.
				InvalidatePage(start);
			}
		}
	} while (start != 0 && start < end);

	return B_OK;
}


status_t
X86VMTranslationMap64Bit::DebugMarkRangePresent(addr_t start, addr_t end,
	bool markPresent)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("X86VMTranslationMap64Bit::DebugMarkRangePresent(%#" B_PRIxADDR
		", %#" B_PRIxADDR ")\n", start, end);

	ThreadCPUPinner pinner(thread_get_current_thread());

	do {
		uint64* pageTable = X86PagingMethod64Bit::PageTableForAddress(
			fPagingStructures->VirtualPML4(), start, fIsKernelMap, false,
			NULL, fPageMapper, fMapCount);
		if (pageTable == NULL) {
			// Move on to the next page table.
			start = ROUNDUP(start + 1, k64BitPageTableRange);
			continue;
		}

		for (uint32 index = start / B_PAGE_SIZE % k64BitTableEntryCount;
				index < k64BitTableEntryCount && start < end;
				index++, start += B_PAGE_SIZE) {
			if ((pageTable[index] & X86_64_PTE_PRESENT) == 0) {
				if (!markPresent)
					continue;

				X86PagingMethod64Bit::SetTableEntryFlags(&pageTable[index],
					X86_64_PTE_PRESENT);
			} else {
				if (markPresent)
					continue;

				uint64 oldEntry = X86PagingMethod64Bit::ClearTableEntryFlags(
					&pageTable[index], X86_64_PTE_PRESENT);

				if ((oldEntry & X86_64_PTE_ACCESSED) != 0) {
					// Note, that we only need to invalidate the address, if the
					// accessed flags was set, since only then the entry could
					// have been in any TLB.
					InvalidatePage(start);
				}
			}
		}
	} while (start != 0 && start < end);

	return B_OK;
}


status_t
X86VMTranslationMap64Bit::UnmapPage(VMArea* area, addr_t address,
	bool updatePageQueue)
{
	ASSERT(address % B_PAGE_SIZE == 0);

	TRACE("X86VMTranslationMap64Bit::UnmapPage(%#" B_PRIxADDR ")\n", address);

	ThreadCPUPinner pinner(thread_get_current_thread());

	// Look up the page table for the virtual address.
	uint64* entry = X86PagingMethod64Bit::PageTableEntryForAddress(
		fPagingStructures->VirtualPML4(), address, fIsKernelMap,
		false, NULL, fPageMapper, fMapCount);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	RecursiveLocker locker(fLock);

	uint64 oldEntry = X86PagingMethod64Bit::ClearTableEntry(entry);

	pinner.Unlock();

	if ((oldEntry & X86_64_PTE_PRESENT) == 0)
		return B_ENTRY_NOT_FOUND;

	fMapCount--;

	if ((oldEntry & X86_64_PTE_ACCESSED) != 0) {
		// Note, that we only need to invalidate the address, if the
		// accessed flags was set, since only then the entry could have been
		// in any TLB.
		InvalidatePage(address);

		Flush();

		// NOTE: Between clearing the page table entry and Flush() other
		// processors (actually even this processor with another thread of the
		// same team) could still access the page in question via their cached
		// entry. We can obviously lose a modified flag in this case, with the
		// effect that the page looks unmodified (and might thus be recycled),
		// but is actually modified.
		// In most cases this is harmless, but for vm_remove_all_page_mappings()
		// this is actually a problem.
		// Interestingly FreeBSD seems to ignore this problem as well
		// (cf. pmap_remove_all()), unless I've missed something.
	}

	locker.Detach();
		// PageUnmapped() will unlock for us

	PageUnmapped(area, (oldEntry & X86_64_PTE_ADDRESS_MASK) / B_PAGE_SIZE,
		(oldEntry & X86_64_PTE_ACCESSED) != 0,
		(oldEntry & X86_64_PTE_DIRTY) != 0, updatePageQueue);

	return B_OK;
}


void
X86VMTranslationMap64Bit::UnmapPages(VMArea* area, addr_t base, size_t size,
	bool updatePageQueue)
{
	if (size == 0)
		return;

	addr_t start = base;
	addr_t end = base + size - 1;

	TRACE("X86VMTranslationMap64Bit::UnmapPages(%p, %#" B_PRIxADDR ", %#"
		B_PRIxADDR ")\n", area, start, end);

	VMAreaMappings queue;

	RecursiveLocker locker(fLock);
	ThreadCPUPinner pinner(thread_get_current_thread());

	do {
		uint64* pageTable = X86PagingMethod64Bit::PageTableForAddress(
			fPagingStructures->VirtualPML4(), start, fIsKernelMap, false,
			NULL, fPageMapper, fMapCount);
		if (pageTable == NULL) {
			// Move on to the next page table.
			start = ROUNDUP(start + 1, k64BitPageTableRange);
			continue;
		}

		for (uint32 index = start / B_PAGE_SIZE % k64BitTableEntryCount;
				index < k64BitTableEntryCount && start < end;
				index++, start += B_PAGE_SIZE) {
			uint64 oldEntry = X86PagingMethod64Bit::ClearTableEntry(
				&pageTable[index]);
			if ((oldEntry & X86_64_PTE_PRESENT) == 0)
				continue;

			fMapCount--;

			if ((oldEntry & X86_64_PTE_ACCESSED) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flags was set, since only then the entry could have
				// been in any TLB.
				InvalidatePage(start);
			}

			if (area->cache_type != CACHE_TYPE_DEVICE) {
				// get the page
				vm_page* page = vm_lookup_page(
					(oldEntry & X86_64_PTE_ADDRESS_MASK) / B_PAGE_SIZE);
				ASSERT(page != NULL);

				DEBUG_PAGE_ACCESS_START(page);

				// transfer the accessed/dirty flags to the page
				if ((oldEntry & X86_64_PTE_ACCESSED) != 0)
					page->accessed = true;
				if ((oldEntry & X86_64_PTE_DIRTY) != 0)
					page->modified = true;

				// remove the mapping object/decrement the wired_count of the
				// page
				if (area->wiring == B_NO_LOCK) {
					vm_page_mapping* mapping = NULL;
					vm_page_mappings::Iterator iterator
						= page->mappings.GetIterator();
					while ((mapping = iterator.Next()) != NULL) {
						if (mapping->area == area)
							break;
					}

					ASSERT(mapping != NULL);

					area->mappings.Remove(mapping);
					page->mappings.Remove(mapping);
					queue.Add(mapping);
				} else
					page->DecrementWiredCount();

				if (!page->IsMapped()) {
					atomic_add(&gMappedPagesCount, -1);

					if (updatePageQueue) {
						if (page->Cache()->temporary)
							vm_page_set_state(page, PAGE_STATE_INACTIVE);
						else if (page->modified)
							vm_page_set_state(page, PAGE_STATE_MODIFIED);
						else
							vm_page_set_state(page, PAGE_STATE_CACHED);
					}
				}

				DEBUG_PAGE_ACCESS_END(page);
			}
		}

		Flush();
			// flush explicitly, since we directly use the lock
	} while (start != 0 && start < end);

	// TODO: As in UnmapPage() we can lose page dirty flags here. ATM it's not
	// really critical here, as in all cases this method is used, the unmapped
	// area range is unmapped for good (resized/cut) and the pages will likely
	// be freed.

	locker.Unlock();

	// free removed mappings
	bool isKernelSpace = area->address_space == VMAddressSpace::Kernel();
	uint32 freeFlags = CACHE_DONT_WAIT_FOR_MEMORY
		| (isKernelSpace ? CACHE_DONT_LOCK_KERNEL_SPACE : 0);
	while (vm_page_mapping* mapping = queue.RemoveHead())
		object_cache_free(gPageMappingsObjectCache, mapping, freeFlags);
}


void
X86VMTranslationMap64Bit::UnmapArea(VMArea* area, bool deletingAddressSpace,
	bool ignoreTopCachePageFlags)
{
	TRACE("X86VMTranslationMap64Bit::UnmapArea(%p)\n", area);

	if (area->cache_type == CACHE_TYPE_DEVICE || area->wiring != B_NO_LOCK) {
		X86VMTranslationMap64Bit::UnmapPages(area, area->Base(), area->Size(),
			true);
		return;
	}

	bool unmapPages = !deletingAddressSpace || !ignoreTopCachePageFlags;

	RecursiveLocker locker(fLock);
	ThreadCPUPinner pinner(thread_get_current_thread());

	VMAreaMappings mappings;
	mappings.MoveFrom(&area->mappings);

	for (VMAreaMappings::Iterator it = mappings.GetIterator();
			vm_page_mapping* mapping = it.Next();) {
		vm_page* page = mapping->page;
		page->mappings.Remove(mapping);

		VMCache* cache = page->Cache();

		bool pageFullyUnmapped = false;
		if (!page->IsMapped()) {
			atomic_add(&gMappedPagesCount, -1);
			pageFullyUnmapped = true;
		}

		if (unmapPages || cache != area->cache) {
			addr_t address = area->Base()
				+ ((page->cache_offset * B_PAGE_SIZE) - area->cache_offset);

			uint64* entry = X86PagingMethod64Bit::PageTableEntryForAddress(
				fPagingStructures->VirtualPML4(), address, fIsKernelMap,
				false, NULL, fPageMapper, fMapCount);
			if (entry == NULL) {
				panic("page %p has mapping for area %p (%#" B_PRIxADDR "), but "
					"has no page table", page, area, address);
				continue;
			}

			uint64 oldEntry = X86PagingMethod64Bit::ClearTableEntry(entry);

			if ((oldEntry & X86_64_PTE_PRESENT) == 0) {
				panic("page %p has mapping for area %p (%#" B_PRIxADDR "), but "
					"has no page table entry", page, area, address);
				continue;
			}

			// transfer the accessed/dirty flags to the page and invalidate
			// the mapping, if necessary
			if ((oldEntry & X86_64_PTE_ACCESSED) != 0) {
				page->accessed = true;

				if (!deletingAddressSpace)
					InvalidatePage(address);
			}

			if ((oldEntry & X86_64_PTE_DIRTY) != 0)
				page->modified = true;

			if (pageFullyUnmapped) {
				DEBUG_PAGE_ACCESS_START(page);

				if (cache->temporary)
					vm_page_set_state(page, PAGE_STATE_INACTIVE);
				else if (page->modified)
					vm_page_set_state(page, PAGE_STATE_MODIFIED);
				else
					vm_page_set_state(page, PAGE_STATE_CACHED);

				DEBUG_PAGE_ACCESS_END(page);
			}
		}

		fMapCount--;
	}

	Flush();
		// flush explicitely, since we directly use the lock

	locker.Unlock();

	bool isKernelSpace = area->address_space == VMAddressSpace::Kernel();
	uint32 freeFlags = CACHE_DONT_WAIT_FOR_MEMORY
		| (isKernelSpace ? CACHE_DONT_LOCK_KERNEL_SPACE : 0);
	while (vm_page_mapping* mapping = mappings.RemoveHead())
		object_cache_free(gPageMappingsObjectCache, mapping, freeFlags);
}


status_t
X86VMTranslationMap64Bit::Query(addr_t virtualAddress,
	phys_addr_t* _physicalAddress, uint32* _flags)
{
	*_flags = 0;
	*_physicalAddress = 0;

	ThreadCPUPinner pinner(thread_get_current_thread());

	// Look up the page table for the virtual address.
	uint64* pte = X86PagingMethod64Bit::PageTableEntryForAddress(
		fPagingStructures->VirtualPML4(), virtualAddress, fIsKernelMap,
		false, NULL, fPageMapper, fMapCount);
	if (pte == NULL)
		return B_OK;

	uint64 entry = *pte;

	*_physicalAddress = entry & X86_64_PTE_ADDRESS_MASK;

	// Translate the page state flags.
	if ((entry & X86_64_PTE_USER) != 0) {
		*_flags |= ((entry & X86_64_PTE_WRITABLE) != 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA;
	}

	*_flags |= ((entry & X86_64_PTE_WRITABLE) != 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & X86_64_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & X86_64_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((entry & X86_64_PTE_PRESENT) != 0 ? PAGE_PRESENT : 0);

	TRACE("X86VMTranslationMap64Bit::Query(%#" B_PRIxADDR ") -> %#"
		B_PRIxPHYSADDR " %#" B_PRIx32 " (pte: %p %#" B_PRIx64 ")\n",
		virtualAddress, *_physicalAddress, *_flags, pte, entry);

	return B_OK;
}


status_t
X86VMTranslationMap64Bit::QueryInterrupt(addr_t virtualAddress,
	phys_addr_t* _physicalAddress, uint32* _flags)
{
	*_flags = 0;
	*_physicalAddress = 0;

	ThreadCPUPinner pinner(thread_get_current_thread());

	// Look up the page table for the virtual address.
	// FIXME: PageTableEntryForAddress uses GetPageTableAt() rather than
	// InterruptGetPageTableAt(). This doesn't actually matter since in our
	// page mapper both functions are the same, but perhaps this should be
	// fixed for correctness.
	uint64* pte = X86PagingMethod64Bit::PageTableEntryForAddress(
		fPagingStructures->VirtualPML4(), virtualAddress, fIsKernelMap,
		false, NULL, fPageMapper, fMapCount);
	if (pte == NULL)
		return B_OK;

	uint64 entry = *pte;

	*_physicalAddress = entry & X86_64_PTE_ADDRESS_MASK;

	// Translate the page state flags.
	if ((entry & X86_64_PTE_USER) != 0) {
		*_flags |= ((entry & X86_64_PTE_WRITABLE) != 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA;
	}

	*_flags |= ((entry & X86_64_PTE_WRITABLE) != 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & X86_64_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & X86_64_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((entry & X86_64_PTE_PRESENT) != 0 ? PAGE_PRESENT : 0);

	TRACE("X86VMTranslationMap64Bit::QueryInterrupt(%#" B_PRIxADDR ") -> %#"
		B_PRIxPHYSADDR ":\n", virtualAddress, *_physicalAddress);

	return B_OK;
}


status_t
X86VMTranslationMap64Bit::Protect(addr_t start, addr_t end, uint32 attributes,
	uint32 memoryType)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("X86VMTranslationMap64Bit::Protect(%#" B_PRIxADDR ", %#" B_PRIxADDR
		", %#" B_PRIx32 ")\n", start, end, attributes);

	// compute protection flags
	uint64 newProtectionFlags = 0;
	if ((attributes & B_USER_PROTECTION) != 0) {
		newProtectionFlags = X86_64_PTE_USER;
		if ((attributes & B_WRITE_AREA) != 0)
			newProtectionFlags |= X86_64_PTE_WRITABLE;
	} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		newProtectionFlags = X86_64_PTE_WRITABLE;

	ThreadCPUPinner pinner(thread_get_current_thread());

	do {
		uint64* pageTable = X86PagingMethod64Bit::PageTableForAddress(
			fPagingStructures->VirtualPML4(), start, fIsKernelMap, false,
			NULL, fPageMapper, fMapCount);
		if (pageTable == NULL) {
			// Move on to the next page table.
			start = ROUNDUP(start + 1, k64BitPageTableRange);
			continue;
		}

		for (uint32 index = start / B_PAGE_SIZE % k64BitTableEntryCount;
				index < k64BitTableEntryCount && start < end;
				index++, start += B_PAGE_SIZE) {
			uint64 entry = pageTable[index];
			if ((entry & X86_64_PTE_PRESENT) == 0)
				continue;

			TRACE("X86VMTranslationMap64Bit::Protect(): protect page %#"
				B_PRIxADDR "\n", start);

			// set the new protection flags -- we want to do that atomically,
			// without changing the accessed or dirty flag
			uint64 oldEntry;
			while (true) {
				oldEntry = X86PagingMethod64Bit::TestAndSetTableEntry(
					&pageTable[index],
					(entry & ~(X86_64_PTE_PROTECTION_MASK
							| X86_64_PTE_MEMORY_TYPE_MASK))
						| newProtectionFlags
						| X86PagingMethod64Bit::MemoryTypeToPageTableEntryFlags(
							memoryType),
					entry);
				if (oldEntry == entry)
					break;
				entry = oldEntry;
			}

			if ((oldEntry & X86_64_PTE_ACCESSED) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flag was set, since only then the entry could have
				// been in any TLB.
				InvalidatePage(start);
			}
		}
	} while (start != 0 && start < end);

	return B_OK;
}


status_t
X86VMTranslationMap64Bit::ClearFlags(addr_t address, uint32 flags)
{
	TRACE("X86VMTranslationMap64Bit::ClearFlags(%#" B_PRIxADDR ", %#" B_PRIx32
		")\n", address, flags);

	ThreadCPUPinner pinner(thread_get_current_thread());

	uint64* entry = X86PagingMethod64Bit::PageTableEntryForAddress(
		fPagingStructures->VirtualPML4(), address, fIsKernelMap,
		false, NULL, fPageMapper, fMapCount);
	if (entry == NULL)
		return B_OK;

	uint64 flagsToClear = ((flags & PAGE_MODIFIED) ? X86_64_PTE_DIRTY : 0)
		| ((flags & PAGE_ACCESSED) ? X86_64_PTE_ACCESSED : 0);

	uint64 oldEntry = X86PagingMethod64Bit::ClearTableEntryFlags(entry,
		flagsToClear);

	if ((oldEntry & flagsToClear) != 0)
		InvalidatePage(address);

	return B_OK;
}


bool
X86VMTranslationMap64Bit::ClearAccessedAndModified(VMArea* area, addr_t address,
	bool unmapIfUnaccessed, bool& _modified)
{
	ASSERT(address % B_PAGE_SIZE == 0);

	TRACE("X86VMTranslationMap64Bit::ClearAccessedAndModified(%#" B_PRIxADDR
		")\n", address);

	RecursiveLocker locker(fLock);
	ThreadCPUPinner pinner(thread_get_current_thread());

	uint64* entry = X86PagingMethod64Bit::PageTableEntryForAddress(
		fPagingStructures->VirtualPML4(), address, fIsKernelMap,
		false, NULL, fPageMapper, fMapCount);
	if (entry == NULL)
		return false;

	uint64 oldEntry;

	if (unmapIfUnaccessed) {
		while (true) {
			oldEntry = *entry;
			if ((oldEntry & X86_64_PTE_PRESENT) == 0) {
				// page mapping not valid
				return false;
			}

			if (oldEntry & X86_64_PTE_ACCESSED) {
				// page was accessed -- just clear the flags
				oldEntry = X86PagingMethod64Bit::ClearTableEntryFlags(entry,
					X86_64_PTE_ACCESSED | X86_64_PTE_DIRTY);
				break;
			}

			// page hasn't been accessed -- unmap it
			if (X86PagingMethod64Bit::TestAndSetTableEntry(entry, 0, oldEntry)
					== oldEntry) {
				break;
			}

			// something changed -- check again
		}
	} else {
		oldEntry = X86PagingMethod64Bit::ClearTableEntryFlags(entry,
			X86_64_PTE_ACCESSED | X86_64_PTE_DIRTY);
	}

	pinner.Unlock();

	_modified = (oldEntry & X86_64_PTE_DIRTY) != 0;

	if ((oldEntry & X86_64_PTE_ACCESSED) != 0) {
		// Note, that we only need to invalidate the address, if the
		// accessed flags was set, since only then the entry could have been
		// in any TLB.
		InvalidatePage(address);
		Flush();

		return true;
	}

	if (!unmapIfUnaccessed)
		return false;

	// We have unmapped the address. Do the "high level" stuff.

	fMapCount--;

	locker.Detach();
		// UnaccessedPageUnmapped() will unlock for us

	UnaccessedPageUnmapped(area,
		(oldEntry & X86_64_PTE_ADDRESS_MASK) / B_PAGE_SIZE);

	return false;
}


X86PagingStructures*
X86VMTranslationMap64Bit::PagingStructures() const
{
	return fPagingStructures;
}
