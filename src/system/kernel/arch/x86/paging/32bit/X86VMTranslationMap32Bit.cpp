/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/32bit/X86VMTranslationMap32Bit.h"

#include <stdlib.h>
#include <string.h>

#include <int.h>
#include <thread.h>
#include <slab/Slab.h>
#include <smp.h>
#include <util/AutoLock.h>
#include <util/queue.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMCache.h>

#include "paging/32bit/X86PagingMethod32Bit.h"
#include "paging/32bit/X86PagingStructures32Bit.h"
#include "paging/x86_physical_page_mapper.h"


//#define TRACE_X86_VM_TRANSLATION_MAP_32_BIT
#ifdef TRACE_X86_VM_TRANSLATION_MAP_32_BIT
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


X86VMTranslationMap32Bit::X86VMTranslationMap32Bit()
	:
	fPagingStructures(NULL)
{
}


X86VMTranslationMap32Bit::~X86VMTranslationMap32Bit()
{
	if (fPagingStructures == NULL)
		return;

	if (fPageMapper != NULL)
		fPageMapper->Delete();

	if (fPagingStructures->pgdir_virt != NULL) {
		// cycle through and free all of the user space pgtables
		for (uint32 i = VADDR_TO_PDENT(USER_BASE);
				i <= VADDR_TO_PDENT(USER_BASE + (USER_SIZE - 1)); i++) {
			if ((fPagingStructures->pgdir_virt[i] & X86_PDE_PRESENT) != 0) {
				addr_t address = fPagingStructures->pgdir_virt[i]
					& X86_PDE_ADDRESS_MASK;
				vm_page* page = vm_lookup_page(address / B_PAGE_SIZE);
				if (!page)
					panic("destroy_tmap: didn't find pgtable page\n");
				DEBUG_PAGE_ACCESS_START(page);
				vm_page_set_state(page, PAGE_STATE_FREE);
			}
		}
	}

	fPagingStructures->RemoveReference();
}


status_t
X86VMTranslationMap32Bit::Init(bool kernel)
{
	TRACE("X86VMTranslationMap32Bit::Init()\n");

	X86VMTranslationMap::Init(kernel);

	fPagingStructures = new(std::nothrow) X86PagingStructures32Bit;
	if (fPagingStructures == NULL)
		return B_NO_MEMORY;

	X86PagingMethod32Bit* method = X86PagingMethod32Bit::Method();

	if (!kernel) {
		// user
		// allocate a physical page mapper
		status_t error = method->PhysicalPageMapper()
			->CreateTranslationMapPhysicalPageMapper(&fPageMapper);
		if (error != B_OK)
			return error;

		// allocate the page directory
		page_directory_entry* virtualPageDir = (page_directory_entry*)memalign(
			B_PAGE_SIZE, B_PAGE_SIZE);
		if (virtualPageDir == NULL)
			return B_NO_MEMORY;

		// look up the page directory's physical address
		phys_addr_t physicalPageDir;
		vm_get_page_mapping(VMAddressSpace::KernelID(),
			(addr_t)virtualPageDir, &physicalPageDir);

		fPagingStructures->Init(virtualPageDir, physicalPageDir,
			method->KernelVirtualPageDirectory());
	} else {
		// kernel
		// get the physical page mapper
		fPageMapper = method->KernelPhysicalPageMapper();

		// we already know the kernel pgdir mapping
		fPagingStructures->Init(method->KernelVirtualPageDirectory(),
			method->KernelPhysicalPageDirectory(), NULL);
	}

	return B_OK;
}


size_t
X86VMTranslationMap32Bit::MaxPagesNeededToMap(addr_t start, addr_t end) const
{
	// If start == 0, the actual base address is not yet known to the caller and
	// we shall assume the worst case.
	if (start == 0) {
		// offset the range so it has the worst possible alignment
		start = 1023 * B_PAGE_SIZE;
		end += 1023 * B_PAGE_SIZE;
	}

	return VADDR_TO_PDENT(end) + 1 - VADDR_TO_PDENT(start);
}


status_t
X86VMTranslationMap32Bit::Map(addr_t va, phys_addr_t pa, uint32 attributes,
	uint32 memoryType, vm_page_reservation* reservation)
{
	TRACE("map_tmap: entry pa 0x%lx va 0x%lx\n", pa, va);

/*
	dprintf("pgdir at 0x%x\n", pgdir);
	dprintf("index is %d\n", va / B_PAGE_SIZE / 1024);
	dprintf("final at 0x%x\n", &pgdir[va / B_PAGE_SIZE / 1024]);
	dprintf("value is 0x%x\n", *(int *)&pgdir[va / B_PAGE_SIZE / 1024]);
	dprintf("present bit is %d\n", pgdir[va / B_PAGE_SIZE / 1024].present);
	dprintf("addr is %d\n", pgdir[va / B_PAGE_SIZE / 1024].addr);
*/
	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	// check to see if a page table exists for this range
	uint32 index = VADDR_TO_PDENT(va);
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		phys_addr_t pgtable;
		vm_page *page;

		// we need to allocate a pgtable
		page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		pgtable = (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("map_tmap: asked for free page for pgtable. 0x%lx\n", pgtable);

		// put it in the pgdir
		X86PagingMethod32Bit::PutPageTableInPageDir(&pd[index], pgtable,
			attributes
				| ((attributes & B_USER_PROTECTION) != 0
						? B_WRITE_AREA : B_KERNEL_WRITE_AREA));

		// update any other page directories, if it maps kernel space
		if (index >= FIRST_KERNEL_PGDIR_ENT
			&& index < (FIRST_KERNEL_PGDIR_ENT + NUM_KERNEL_PGDIR_ENTS)) {
			X86PagingStructures32Bit::UpdateAllPageDirs(index, pd[index]);
		}

		fMapCount++;
	}

	// now, fill in the pentry
	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(va);

	ASSERT_PRINT((pt[index] & X86_PTE_PRESENT) == 0,
		"virtual address: %#" B_PRIxADDR ", existing pte: %#" B_PRIx32, va,
		pt[index]);

	X86PagingMethod32Bit::PutPageTableEntryInTable(&pt[index], pa, attributes,
		memoryType, fIsKernelMap);

	pinner.Unlock();

	// Note: We don't need to invalidate the TLB for this address, as previously
	// the entry was not present and the TLB doesn't cache those entries.

	fMapCount++;

	return 0;
}


status_t
X86VMTranslationMap32Bit::Unmap(addr_t start, addr_t end)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("unmap_tmap: asked to free pages 0x%lx to 0x%lx\n", start, end);

	page_directory_entry *pd = fPagingStructures->pgdir_virt;

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & X86_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & X86_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); (index < 1024) && (start < end);
				index++, start += B_PAGE_SIZE) {
			if ((pt[index] & X86_PTE_PRESENT) == 0) {
				// page mapping not valid
				continue;
			}

			TRACE("unmap_tmap: removing page 0x%lx\n", start);

			page_table_entry oldEntry
				= X86PagingMethod32Bit::ClearPageTableEntryFlags(&pt[index],
					X86_PTE_PRESENT);
			fMapCount--;

			if ((oldEntry & X86_PTE_ACCESSED) != 0) {
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
X86VMTranslationMap32Bit::DebugMarkRangePresent(addr_t start, addr_t end,
	bool markPresent)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	page_directory_entry *pd = fPagingStructures->pgdir_virt;

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & X86_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & X86_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); (index < 1024) && (start < end);
				index++, start += B_PAGE_SIZE) {
			if ((pt[index] & X86_PTE_PRESENT) == 0) {
				if (!markPresent)
					continue;

				X86PagingMethod32Bit::SetPageTableEntryFlags(&pt[index],
					X86_PTE_PRESENT);
			} else {
				if (markPresent)
					continue;

				page_table_entry oldEntry
					= X86PagingMethod32Bit::ClearPageTableEntryFlags(&pt[index],
						X86_PTE_PRESENT);

				if ((oldEntry & X86_PTE_ACCESSED) != 0) {
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


/*!	Caller must have locked the cache of the page to be unmapped.
	This object shouldn't be locked.
*/
status_t
X86VMTranslationMap32Bit::UnmapPage(VMArea* area, addr_t address,
	bool updatePageQueue)
{
	ASSERT(address % B_PAGE_SIZE == 0);

	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	TRACE("X86VMTranslationMap32Bit::UnmapPage(%#" B_PRIxADDR ")\n", address);

	RecursiveLocker locker(fLock);

	int index = VADDR_TO_PDENT(address);
	if ((pd[index] & X86_PDE_PRESENT) == 0)
		return B_ENTRY_NOT_FOUND;

	ThreadCPUPinner pinner(thread_get_current_thread());

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);

	index = VADDR_TO_PTENT(address);
	page_table_entry oldEntry = X86PagingMethod32Bit::ClearPageTableEntry(
		&pt[index]);

	pinner.Unlock();

	if ((oldEntry & X86_PTE_PRESENT) == 0) {
		// page mapping not valid
		return B_ENTRY_NOT_FOUND;
	}

	fMapCount--;

	if ((oldEntry & X86_PTE_ACCESSED) != 0) {
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

	PageUnmapped(area, (oldEntry & X86_PTE_ADDRESS_MASK) / B_PAGE_SIZE,
		(oldEntry & X86_PTE_ACCESSED) != 0, (oldEntry & X86_PTE_DIRTY) != 0,
		updatePageQueue);

	return B_OK;
}


void
X86VMTranslationMap32Bit::UnmapPages(VMArea* area, addr_t base, size_t size,
	bool updatePageQueue)
{
	if (size == 0)
		return;

	addr_t start = base;
	addr_t end = base + size - 1;

	TRACE("X86VMTranslationMap32Bit::UnmapPages(%p, %#" B_PRIxADDR ", %#"
		B_PRIxADDR ")\n", area, start, end);

	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	VMAreaMappings queue;

	RecursiveLocker locker(fLock);

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & X86_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & X86_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); (index < 1024) && (start < end);
				index++, start += B_PAGE_SIZE) {
			page_table_entry oldEntry
				= X86PagingMethod32Bit::ClearPageTableEntry(&pt[index]);
			if ((oldEntry & X86_PTE_PRESENT) == 0)
				continue;

			fMapCount--;

			if ((oldEntry & X86_PTE_ACCESSED) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flags was set, since only then the entry could have
				// been in any TLB.
				InvalidatePage(start);
			}

			if (area->cache_type != CACHE_TYPE_DEVICE) {
				// get the page
				vm_page* page = vm_lookup_page(
					(oldEntry & X86_PTE_ADDRESS_MASK) / B_PAGE_SIZE);
				ASSERT(page != NULL);

				DEBUG_PAGE_ACCESS_START(page);

				// transfer the accessed/dirty flags to the page
				if ((oldEntry & X86_PTE_ACCESSED) != 0)
					page->accessed = true;
				if ((oldEntry & X86_PTE_DIRTY) != 0)
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
X86VMTranslationMap32Bit::UnmapArea(VMArea* area, bool deletingAddressSpace,
	bool ignoreTopCachePageFlags)
{
	if (area->cache_type == CACHE_TYPE_DEVICE || area->wiring != B_NO_LOCK) {
		X86VMTranslationMap32Bit::UnmapPages(area, area->Base(), area->Size(),
			true);
		return;
	}

	bool unmapPages = !deletingAddressSpace || !ignoreTopCachePageFlags;

	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	RecursiveLocker locker(fLock);

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

			int index = VADDR_TO_PDENT(address);
			if ((pd[index] & X86_PDE_PRESENT) == 0) {
				panic("page %p has mapping for area %p (%#" B_PRIxADDR "), but "
					"has no page dir entry", page, area, address);
				continue;
			}

			ThreadCPUPinner pinner(thread_get_current_thread());

			page_table_entry* pt
				= (page_table_entry*)fPageMapper->GetPageTableAt(
					pd[index] & X86_PDE_ADDRESS_MASK);
			page_table_entry oldEntry
				= X86PagingMethod32Bit::ClearPageTableEntry(
					&pt[VADDR_TO_PTENT(address)]);

			pinner.Unlock();

			if ((oldEntry & X86_PTE_PRESENT) == 0) {
				panic("page %p has mapping for area %p (%#" B_PRIxADDR "), but "
					"has no page table entry", page, area, address);
				continue;
			}

			// transfer the accessed/dirty flags to the page and invalidate
			// the mapping, if necessary
			if ((oldEntry & X86_PTE_ACCESSED) != 0) {
				page->accessed = true;

				if (!deletingAddressSpace)
					InvalidatePage(address);
			}

			if ((oldEntry & X86_PTE_DIRTY) != 0)
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
X86VMTranslationMap32Bit::Query(addr_t va, phys_addr_t *_physical,
	uint32 *_flags)
{
	// default the flags to not present
	*_flags = 0;
	*_physical = 0;

	int index = VADDR_TO_PDENT(va);
	page_directory_entry *pd = fPagingStructures->pgdir_virt;
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);
	page_table_entry entry = pt[VADDR_TO_PTENT(va)];

	*_physical = entry & X86_PDE_ADDRESS_MASK;

	// read in the page state flags
	if ((entry & X86_PTE_USER) != 0) {
		*_flags |= ((entry & X86_PTE_WRITABLE) != 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA;
	}

	*_flags |= ((entry & X86_PTE_WRITABLE) != 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & X86_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & X86_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((entry & X86_PTE_PRESENT) != 0 ? PAGE_PRESENT : 0);

	pinner.Unlock();

	TRACE("query_tmap: returning pa 0x%lx for va 0x%lx\n", *_physical, va);

	return B_OK;
}


status_t
X86VMTranslationMap32Bit::QueryInterrupt(addr_t va, phys_addr_t *_physical,
	uint32 *_flags)
{
	*_flags = 0;
	*_physical = 0;

	int index = VADDR_TO_PDENT(va);
	page_directory_entry* pd = fPagingStructures->pgdir_virt;
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	// map page table entry
	page_table_entry* pt = (page_table_entry*)X86PagingMethod32Bit::Method()
		->PhysicalPageMapper()->InterruptGetPageTableAt(
			pd[index] & X86_PDE_ADDRESS_MASK);
	page_table_entry entry = pt[VADDR_TO_PTENT(va)];

	*_physical = entry & X86_PDE_ADDRESS_MASK;

	// read in the page state flags
	if ((entry & X86_PTE_USER) != 0) {
		*_flags |= ((entry & X86_PTE_WRITABLE) != 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA;
	}

	*_flags |= ((entry & X86_PTE_WRITABLE) != 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & X86_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & X86_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((entry & X86_PTE_PRESENT) != 0 ? PAGE_PRESENT : 0);

	return B_OK;
}


status_t
X86VMTranslationMap32Bit::Protect(addr_t start, addr_t end, uint32 attributes,
	uint32 memoryType)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("protect_tmap: pages 0x%lx to 0x%lx, attributes %lx\n", start, end,
		attributes);

	// compute protection flags
	uint32 newProtectionFlags = 0;
	if ((attributes & B_USER_PROTECTION) != 0) {
		newProtectionFlags = X86_PTE_USER;
		if ((attributes & B_WRITE_AREA) != 0)
			newProtectionFlags |= X86_PTE_WRITABLE;
	} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		newProtectionFlags = X86_PTE_WRITABLE;

	page_directory_entry *pd = fPagingStructures->pgdir_virt;

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & X86_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & X86_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); index < 1024 && start < end;
				index++, start += B_PAGE_SIZE) {
			page_table_entry entry = pt[index];
			if ((entry & X86_PTE_PRESENT) == 0) {
				// page mapping not valid
				continue;
			}

			TRACE("protect_tmap: protect page 0x%lx\n", start);

			// set the new protection flags -- we want to do that atomically,
			// without changing the accessed or dirty flag
			page_table_entry oldEntry;
			while (true) {
				oldEntry = X86PagingMethod32Bit::TestAndSetPageTableEntry(
					&pt[index],
					(entry & ~(X86_PTE_PROTECTION_MASK
							| X86_PTE_MEMORY_TYPE_MASK))
						| newProtectionFlags
						| X86PagingMethod32Bit::MemoryTypeToPageTableEntryFlags(
							memoryType),
					entry);
				if (oldEntry == entry)
					break;
				entry = oldEntry;
			}

			if ((oldEntry & X86_PTE_ACCESSED) != 0) {
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
X86VMTranslationMap32Bit::ClearFlags(addr_t va, uint32 flags)
{
	int index = VADDR_TO_PDENT(va);
	page_directory_entry* pd = fPagingStructures->pgdir_virt;
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	uint32 flagsToClear = ((flags & PAGE_MODIFIED) ? X86_PTE_DIRTY : 0)
		| ((flags & PAGE_ACCESSED) ? X86_PTE_ACCESSED : 0);

	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(va);

	// clear out the flags we've been requested to clear
	page_table_entry oldEntry
		= X86PagingMethod32Bit::ClearPageTableEntryFlags(&pt[index],
			flagsToClear);

	pinner.Unlock();

	if ((oldEntry & flagsToClear) != 0)
		InvalidatePage(va);

	return B_OK;
}


bool
X86VMTranslationMap32Bit::ClearAccessedAndModified(VMArea* area, addr_t address,
	bool unmapIfUnaccessed, bool& _modified)
{
	ASSERT(address % B_PAGE_SIZE == 0);

	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	TRACE("X86VMTranslationMap32Bit::ClearAccessedAndModified(%#" B_PRIxADDR
		")\n", address);

	RecursiveLocker locker(fLock);

	int index = VADDR_TO_PDENT(address);
	if ((pd[index] & X86_PDE_PRESENT) == 0)
		return false;

	ThreadCPUPinner pinner(thread_get_current_thread());

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);

	index = VADDR_TO_PTENT(address);

	// perform the deed
	page_table_entry oldEntry;

	if (unmapIfUnaccessed) {
		while (true) {
			oldEntry = pt[index];
			if ((oldEntry & X86_PTE_PRESENT) == 0) {
				// page mapping not valid
				return false;
			}

			if (oldEntry & X86_PTE_ACCESSED) {
				// page was accessed -- just clear the flags
				oldEntry = X86PagingMethod32Bit::ClearPageTableEntryFlags(
					&pt[index], X86_PTE_ACCESSED | X86_PTE_DIRTY);
				break;
			}

			// page hasn't been accessed -- unmap it
			if (X86PagingMethod32Bit::TestAndSetPageTableEntry(&pt[index], 0,
					oldEntry) == oldEntry) {
				break;
			}

			// something changed -- check again
		}
	} else {
		oldEntry = X86PagingMethod32Bit::ClearPageTableEntryFlags(&pt[index],
			X86_PTE_ACCESSED | X86_PTE_DIRTY);
	}

	pinner.Unlock();

	_modified = (oldEntry & X86_PTE_DIRTY) != 0;

	if ((oldEntry & X86_PTE_ACCESSED) != 0) {
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
		(oldEntry & X86_PTE_ADDRESS_MASK) / B_PAGE_SIZE);

	return false;
}


X86PagingStructures*
X86VMTranslationMap32Bit::PagingStructures() const
{
	return fPagingStructures;
}
