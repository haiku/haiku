/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/32bit/X86PagingMethod32Bit.h"

#include <stdlib.h>
#include <string.h>

#include <AutoDeleter.h>

#include <arch_system_info.h>
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

#include "paging/32bit/X86PagingStructures32Bit.h"
#include "paging/32bit/X86VMTranslationMap32Bit.h"
#include "paging/x86_physical_page_mapper.h"
#include "paging/x86_physical_page_mapper_large_memory.h"


//#define TRACE_X86_PAGING_METHOD_32_BIT
#ifdef TRACE_X86_PAGING_METHOD_32_BIT
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


using X86LargePhysicalPageMapper::PhysicalPageSlot;


static const size_t kPageTableAlignment = 1024 * B_PAGE_SIZE;


static X86PagingMethod32Bit sMethod;

static page_table_entry *sPageHole = NULL;
static page_directory_entry *sPageHolePageDir = NULL;
static uint32 sKernelPhysicalPageDirectory = 0;
static page_directory_entry *sKernelVirtualPageDirectory = NULL;

static X86PhysicalPageMapper* sPhysicalPageMapper;
static TranslationMapPhysicalPageMapper* sKernelPhysicalPageMapper;


//	#pragma mark -


//! TODO: currently assumes this translation map is active
static status_t
early_query(addr_t va, phys_addr_t *_physicalAddress)
{
	if ((sPageHolePageDir[VADDR_TO_PDENT(va)] & X86_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_ERROR;
	}

	page_table_entry* pentry = sPageHole + va / B_PAGE_SIZE;
	if ((*pentry & X86_PTE_PRESENT) == 0) {
		// page mapping not valid
		return B_ERROR;
	}

	*_physicalAddress = *pentry & X86_PTE_ADDRESS_MASK;
	return B_OK;
}


static inline uint32
memory_type_to_pte_flags(uint32 memoryType)
{
	// ATM we only handle the uncacheable and write-through type explicitly. For
	// all other types we rely on the MTRRs to be set up correctly. Since we set
	// the default memory type to write-back and since the uncacheable type in
	// the PTE overrides any MTRR attribute (though, as per the specs, that is
	// not recommended for performance reasons), this reduces the work we
	// actually *have* to do with the MTRRs to setting the remaining types
	// (usually only write-combining for the frame buffer).
	switch (memoryType) {
		case B_MTR_UC:
			return X86_PTE_CACHING_DISABLED | X86_PTE_WRITE_THROUGH;

		case B_MTR_WC:
			// X86_PTE_WRITE_THROUGH would be closer, but the combination with
			// MTRR WC is "implementation defined" for Pentium Pro/II.
			return 0;

		case B_MTR_WT:
			return X86_PTE_WRITE_THROUGH;

		case B_MTR_WP:
		case B_MTR_WB:
		default:
			return 0;
	}
}


static void
put_page_table_entry_in_pgtable(page_table_entry* entry,
	phys_addr_t physicalAddress, uint32 attributes, uint32 memoryType,
	bool globalPage)
{
	page_table_entry page = (physicalAddress & X86_PTE_ADDRESS_MASK)
		| X86_PTE_PRESENT | (globalPage ? X86_PTE_GLOBAL : 0)
		| memory_type_to_pte_flags(memoryType);

	// if the page is user accessible, it's automatically
	// accessible in kernel space, too (but with the same
	// protection)
	if ((attributes & B_USER_PROTECTION) != 0) {
		page |= X86_PTE_USER;
		if ((attributes & B_WRITE_AREA) != 0)
			page |= X86_PTE_WRITABLE;
	} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		page |= X86_PTE_WRITABLE;

	// put it in the page table
	*(volatile page_table_entry*)entry = page;
}


//	#pragma mark -


void
x86_put_pgtable_in_pgdir(page_directory_entry *entry,
	phys_addr_t pgtablePhysical, uint32 attributes)
{
	*entry = (pgtablePhysical & X86_PDE_ADDRESS_MASK)
		| X86_PDE_PRESENT
		| X86_PDE_WRITABLE
		| X86_PDE_USER;
		// TODO: we ignore the attributes of the page table - for compatibility
		// with BeOS we allow having user accessible areas in the kernel address
		// space. This is currently being used by some drivers, mainly for the
		// frame buffer. Our current real time data implementation makes use of
		// this fact, too.
		// We might want to get rid of this possibility one day, especially if
		// we intend to port it to a platform that does not support this.
}


void
x86_early_prepare_page_tables(page_table_entry* pageTables, addr_t address,
	size_t size)
{
	memset(pageTables, 0, B_PAGE_SIZE * (size / (B_PAGE_SIZE * 1024)));

	// put the array of pgtables directly into the kernel pagedir
	// these will be wired and kept mapped into virtual space to be easy to get
	// to
	{
		addr_t virtualTable = (addr_t)pageTables;

		for (size_t i = 0; i < (size / (B_PAGE_SIZE * 1024));
				i++, virtualTable += B_PAGE_SIZE) {
			phys_addr_t physicalTable = 0;
			early_query(virtualTable, &physicalTable);
			page_directory_entry* entry = &sPageHolePageDir[
				(address / (B_PAGE_SIZE * 1024)) + i];
			x86_put_pgtable_in_pgdir(entry, physicalTable,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		}
	}
}


// #pragma mark - VM ops


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

	if (!kernel) {
		// user
		// allocate a physical page mapper
		status_t error = sPhysicalPageMapper
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
			sKernelVirtualPageDirectory);
	} else {
		// kernel
		// get the physical page mapper
		fPageMapper = sKernelPhysicalPageMapper;

		// we already know the kernel pgdir mapping
		fPagingStructures->Init(sKernelVirtualPageDirectory,
			sKernelPhysicalPageDirectory, NULL);
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
		x86_put_pgtable_in_pgdir(&pd[index], pgtable, attributes
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
	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(va);

	ASSERT_PRINT((pt[index] & X86_PTE_PRESENT) == 0,
		"virtual address: %#" B_PRIxADDR ", existing pte: %#" B_PRIx32, va,
		pt[index]);

	put_page_table_entry_in_pgtable(&pt[index], pa, attributes, memoryType,
		fIsKernelMap);

	pinner.Unlock();

	// Note: We don't need to invalidate the TLB for this address, as previously
	// the entry was not present and the TLB doesn't cache those entries.

	fMapCount++;

	return 0;
}


status_t
X86VMTranslationMap32Bit::Unmap(addr_t start, addr_t end)
{
	page_directory_entry *pd = fPagingStructures->pgdir_virt;

	start = ROUNDDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

	TRACE("unmap_tmap: asked to free pages 0x%lx to 0x%lx\n", start, end);

restart:
	if (start >= end)
		return B_OK;

	int index = VADDR_TO_PDENT(start);
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE * 1024);
		if (start == 0)
			return B_OK;
		goto restart;
	}

	struct thread* thread = thread_get_current_thread();
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

		page_table_entry oldEntry = clear_page_table_entry_flags(&pt[index],
			X86_PTE_PRESENT);
		fMapCount--;

		if ((oldEntry & X86_PTE_ACCESSED) != 0) {
			// Note, that we only need to invalidate the address, if the
			// accessed flags was set, since only then the entry could have been
			// in any TLB.
			if (fInvalidPagesCount < PAGE_INVALIDATE_CACHE_SIZE)
				fInvalidPages[fInvalidPagesCount] = start;

			fInvalidPagesCount++;
		}
	}

	pinner.Unlock();

	goto restart;
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
	page_table_entry oldEntry = clear_page_table_entry(&pt[index]);

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
		if (fInvalidPagesCount < PAGE_INVALIDATE_CACHE_SIZE)
			fInvalidPages[fInvalidPagesCount] = address;

		fInvalidPagesCount++;

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

	if (area->cache_type == CACHE_TYPE_DEVICE)
		return B_OK;

	// get the page
	vm_page* page = vm_lookup_page(
		(oldEntry & X86_PTE_ADDRESS_MASK) / B_PAGE_SIZE);
	ASSERT(page != NULL);

	// transfer the accessed/dirty flags to the page
	if ((oldEntry & X86_PTE_ACCESSED) != 0)
		page->accessed = true;
	if ((oldEntry & X86_PTE_DIRTY) != 0)
		page->modified = true;

	// remove the mapping object/decrement the wired_count of the page
	vm_page_mapping* mapping = NULL;
	if (area->wiring == B_NO_LOCK) {
		vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
		while ((mapping = iterator.Next()) != NULL) {
			if (mapping->area == area) {
				area->mappings.Remove(mapping);
				page->mappings.Remove(mapping);
				break;
			}
		}

		ASSERT(mapping != NULL);
	} else
		page->wired_count--;

	locker.Unlock();

	if (page->wired_count == 0 && page->mappings.IsEmpty()) {
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

	if (mapping != NULL) {
		bool isKernelSpace = area->address_space == VMAddressSpace::Kernel();
		object_cache_free(gPageMappingsObjectCache, mapping,
			CACHE_DONT_WAIT_FOR_MEMORY
				| (isKernelSpace ? CACHE_DONT_LOCK_KERNEL_SPACE : 0));
	}

	return B_OK;
}


void
X86VMTranslationMap32Bit::UnmapPages(VMArea* area, addr_t base, size_t size,
	bool updatePageQueue)
{
	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	addr_t start = base;
	addr_t end = base + size;

	TRACE("X86VMTranslationMap32Bit::UnmapPages(%p, %#" B_PRIxADDR ", %#"
		B_PRIxADDR ")\n", area, start, end);

	VMAreaMappings queue;

	RecursiveLocker locker(fLock);

	while (start < end) {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & X86_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, B_PAGE_SIZE * 1024);
			if (start == 0)
				break;
			continue;
		}

		struct thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & X86_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); (index < 1024) && (start < end);
				index++, start += B_PAGE_SIZE) {
			page_table_entry oldEntry = clear_page_table_entry(&pt[index]);
			if ((oldEntry & X86_PTE_PRESENT) == 0)
				continue;

			fMapCount--;

			if ((oldEntry & X86_PTE_ACCESSED) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flags was set, since only then the entry could have
				// been in any TLB.
				if (fInvalidPagesCount < PAGE_INVALIDATE_CACHE_SIZE)
					fInvalidPages[fInvalidPagesCount] = start;

				fInvalidPagesCount++;
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
					page->wired_count--;

				if (page->wired_count == 0 && page->mappings.IsEmpty()) {
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

		pinner.Unlock();
	}

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
		if (page->wired_count == 0 && page->mappings.IsEmpty()) {
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
			page_table_entry oldEntry = clear_page_table_entry(
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

				if (!deletingAddressSpace) {
					if (fInvalidPagesCount < PAGE_INVALIDATE_CACHE_SIZE)
						fInvalidPages[fInvalidPagesCount] = address;

					fInvalidPagesCount++;
				}
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

	struct thread* thread = thread_get_current_thread();
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
	page_table_entry* pt
		= (page_table_entry*)sPhysicalPageMapper->InterruptGetPageTableAt(
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
	page_directory_entry *pd = fPagingStructures->pgdir_virt;

	start = ROUNDDOWN(start, B_PAGE_SIZE);

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

restart:
	if (start >= end)
		return B_OK;

	int index = VADDR_TO_PDENT(start);
	if ((pd[index] & X86_PDE_PRESENT) == 0) {
		// no pagetable here, move the start up to access the next page table
		start = ROUNDUP(start + 1, B_PAGE_SIZE * 1024);
		if (start == 0)
			return B_OK;
		goto restart;
	}

	struct thread* thread = thread_get_current_thread();
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
			oldEntry = test_and_set_page_table_entry(&pt[index],
				(entry & ~(X86_PTE_PROTECTION_MASK | X86_PTE_MEMORY_TYPE_MASK))
					| newProtectionFlags | memory_type_to_pte_flags(memoryType),
				entry);
			if (oldEntry == entry)
				break;
			entry = oldEntry;
		}

		if ((oldEntry & X86_PTE_ACCESSED) != 0) {
			// Note, that we only need to invalidate the address, if the
			// accessed flag was set, since only then the entry could have been
			// in any TLB.
			if (fInvalidPagesCount < PAGE_INVALIDATE_CACHE_SIZE)
				fInvalidPages[fInvalidPagesCount] = start;

			fInvalidPagesCount++;
		}
	}

	pinner.Unlock();

	goto restart;
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

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & X86_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(va);

	// clear out the flags we've been requested to clear
	page_table_entry oldEntry
		= clear_page_table_entry_flags(&pt[index], flagsToClear);

	pinner.Unlock();

	if ((oldEntry & flagsToClear) != 0) {
		if (fInvalidPagesCount < PAGE_INVALIDATE_CACHE_SIZE)
			fInvalidPages[fInvalidPagesCount] = va;

		fInvalidPagesCount++;
	}

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
				oldEntry = clear_page_table_entry_flags(&pt[index],
					X86_PTE_ACCESSED | X86_PTE_DIRTY);
				break;
			}

			// page hasn't been accessed -- unmap it
			if (test_and_set_page_table_entry(&pt[index], 0, oldEntry)
					== oldEntry) {
				break;
			}

			// something changed -- check again
		}
	} else {
		oldEntry = clear_page_table_entry_flags(&pt[index],
			X86_PTE_ACCESSED | X86_PTE_DIRTY);
	}

	pinner.Unlock();

	_modified = (oldEntry & X86_PTE_DIRTY) != 0;

	if ((oldEntry & X86_PTE_ACCESSED) != 0) {
		// Note, that we only need to invalidate the address, if the
		// accessed flags was set, since only then the entry could have been
		// in any TLB.
		if (fInvalidPagesCount < PAGE_INVALIDATE_CACHE_SIZE)
			fInvalidPages[fInvalidPagesCount] = address;

		fInvalidPagesCount++;

		Flush();

		return true;
	}

	if (!unmapIfUnaccessed)
		return false;

	// We have unmapped the address. Do the "high level" stuff.

	fMapCount--;

	if (area->cache_type == CACHE_TYPE_DEVICE)
		return false;

	// get the page
	vm_page* page = vm_lookup_page(
		(oldEntry & X86_PTE_ADDRESS_MASK) / B_PAGE_SIZE);
	ASSERT(page != NULL);

	// remove the mapping object/decrement the wired_count of the page
	vm_page_mapping* mapping = NULL;
	if (area->wiring == B_NO_LOCK) {
		vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
		while ((mapping = iterator.Next()) != NULL) {
			if (mapping->area == area) {
				area->mappings.Remove(mapping);
				page->mappings.Remove(mapping);
				break;
			}
		}

		ASSERT(mapping != NULL);
	} else
		page->wired_count--;

	locker.Unlock();

	if (page->wired_count == 0 && page->mappings.IsEmpty())
		atomic_add(&gMappedPagesCount, -1);

	if (mapping != NULL) {
		object_cache_free(gPageMappingsObjectCache, mapping,
			CACHE_DONT_WAIT_FOR_MEMORY | CACHE_DONT_LOCK_KERNEL_SPACE);
			// Since this is called by the page daemon, we never want to lock
			// the kernel address space.
	}

	return false;
}


X86PagingStructures*
X86VMTranslationMap32Bit::PagingStructures() const
{
	return fPagingStructures;
}


// #pragma mark - X86PagingMethod32Bit::PhysicalPageSlotPool


struct X86PagingMethod32Bit::PhysicalPageSlotPool
	: X86LargePhysicalPageMapper::PhysicalPageSlotPool {
public:
	virtual						~PhysicalPageSlotPool();

			status_t			InitInitial(kernel_args* args);
			status_t			InitInitialPostArea(kernel_args* args);

			void				Init(area_id dataArea, void* data,
									area_id virtualArea, addr_t virtualBase);

	virtual	status_t			AllocatePool(
									X86LargePhysicalPageMapper
										::PhysicalPageSlotPool*& _pool);
	virtual	void				Map(phys_addr_t physicalAddress,
									addr_t virtualAddress);

public:
	static	PhysicalPageSlotPool sInitialPhysicalPagePool;

private:
	area_id					fDataArea;
	area_id					fVirtualArea;
	addr_t					fVirtualBase;
	page_table_entry*		fPageTable;
};


X86PagingMethod32Bit::PhysicalPageSlotPool
	X86PagingMethod32Bit::PhysicalPageSlotPool::sInitialPhysicalPagePool;


X86PagingMethod32Bit::PhysicalPageSlotPool::~PhysicalPageSlotPool()
{
}


status_t
X86PagingMethod32Bit::PhysicalPageSlotPool::InitInitial(kernel_args* args)
{
	// We reserve more, so that we can guarantee to align the base address
	// to page table ranges.
	addr_t virtualBase = vm_allocate_early(args,
		1024 * B_PAGE_SIZE + kPageTableAlignment - B_PAGE_SIZE, 0, 0, false);
	if (virtualBase == 0) {
		panic("LargeMemoryPhysicalPageMapper::Init(): Failed to reserve "
			"physical page pool space in virtual address space!");
		return B_ERROR;
	}
	virtualBase = (virtualBase + kPageTableAlignment - 1)
		/ kPageTableAlignment * kPageTableAlignment;

	// allocate memory for the page table and data
	size_t areaSize = B_PAGE_SIZE + sizeof(PhysicalPageSlot[1024]);
	page_table_entry* pageTable = (page_table_entry*)vm_allocate_early(args,
		areaSize, ~0L, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, false);

	// prepare the page table
	x86_early_prepare_page_tables(pageTable, virtualBase,
		1024 * B_PAGE_SIZE);

	// init the pool structure and add the initial pool
	Init(-1, pageTable, -1, (addr_t)virtualBase);

	return B_OK;
}


status_t
X86PagingMethod32Bit::PhysicalPageSlotPool::InitInitialPostArea(
	kernel_args* args)
{
	// create an area for the (already allocated) data
	size_t areaSize = B_PAGE_SIZE + sizeof(PhysicalPageSlot[1024]);
	void* temp = fPageTable;
	area_id area = create_area("physical page pool", &temp,
		B_EXACT_ADDRESS, areaSize, B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK) {
		panic("LargeMemoryPhysicalPageMapper::InitPostArea(): Failed to "
			"create area for physical page pool.");
		return area;
	}
	fDataArea = area;

	// create an area for the virtual address space
	temp = (void*)fVirtualBase;
	area = vm_create_null_area(VMAddressSpace::KernelID(),
		"physical page pool space", &temp, B_EXACT_ADDRESS,
		1024 * B_PAGE_SIZE, 0);
	if (area < B_OK) {
		panic("LargeMemoryPhysicalPageMapper::InitPostArea(): Failed to "
			"create area for physical page pool space.");
		return area;
	}
	fVirtualArea = area;

	return B_OK;
}


void
X86PagingMethod32Bit::PhysicalPageSlotPool::Init(area_id dataArea, void* data,
	area_id virtualArea, addr_t virtualBase)
{
	fDataArea = dataArea;
	fVirtualArea = virtualArea;
	fVirtualBase = virtualBase;
	fPageTable = (page_table_entry*)data;

	// init slot list
	fSlots = (PhysicalPageSlot*)(fPageTable + 1024);
	addr_t slotAddress = virtualBase;
	for (int32 i = 0; i < 1024; i++, slotAddress += B_PAGE_SIZE) {
		PhysicalPageSlot* slot = &fSlots[i];
		slot->next = slot + 1;
		slot->pool = this;
		slot->address = slotAddress;
	}

	fSlots[1023].next = NULL;
		// terminate list
}


void
X86PagingMethod32Bit::PhysicalPageSlotPool::Map(phys_addr_t physicalAddress,
	addr_t virtualAddress)
{
	page_table_entry& pte = fPageTable[
		(virtualAddress - fVirtualBase) / B_PAGE_SIZE];
	pte = (physicalAddress & X86_PTE_ADDRESS_MASK)
		| X86_PTE_WRITABLE | X86_PTE_GLOBAL | X86_PTE_PRESENT;

	invalidate_TLB(virtualAddress);
}


status_t
X86PagingMethod32Bit::PhysicalPageSlotPool::AllocatePool(
	X86LargePhysicalPageMapper::PhysicalPageSlotPool*& _pool)
{
	// create the pool structure
	PhysicalPageSlotPool* pool = new(std::nothrow) PhysicalPageSlotPool;
	if (pool == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<PhysicalPageSlotPool> poolDeleter(pool);

	// create an area that can contain the page table and the slot
	// structures
	size_t areaSize = B_PAGE_SIZE + sizeof(PhysicalPageSlot[1024]);
	void* data;
	area_id dataArea = create_area_etc(B_SYSTEM_TEAM, "physical page pool",
		&data, B_ANY_KERNEL_ADDRESS, PAGE_ALIGN(areaSize), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0, CREATE_AREA_DONT_WAIT);
	if (dataArea < 0)
		return dataArea;

	// create the null area for the virtual address space
	void* virtualBase;
	area_id virtualArea = vm_create_null_area(
		VMAddressSpace::KernelID(), "physical page pool space",
		&virtualBase, B_ANY_KERNEL_BLOCK_ADDRESS, 1024 * B_PAGE_SIZE,
		CREATE_AREA_PRIORITY_VIP);
	if (virtualArea < 0) {
		delete_area(dataArea);
		return virtualArea;
	}

	// prepare the page table
	memset(data, 0, B_PAGE_SIZE);

	// get the page table's physical address
	phys_addr_t physicalTable;
	X86VMTranslationMap32Bit* map = static_cast<X86VMTranslationMap32Bit*>(
		VMAddressSpace::Kernel()->TranslationMap());
	uint32 dummyFlags;
	cpu_status state = disable_interrupts();
	map->QueryInterrupt((addr_t)data, &physicalTable, &dummyFlags);
	restore_interrupts(state);

	// put the page table into the page directory
	int32 index = (addr_t)virtualBase / (B_PAGE_SIZE * 1024);
	page_directory_entry* entry
		= &map->PagingStructures32Bit()->pgdir_virt[index];
	x86_put_pgtable_in_pgdir(entry, physicalTable,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	X86PagingStructures32Bit::UpdateAllPageDirs(index, *entry);

	// init the pool structure
	pool->Init(dataArea, data, virtualArea, (addr_t)virtualBase);
	poolDeleter.Detach();
	_pool = pool;
	return B_OK;
}


// #pragma mark - X86PagingMethod32Bit


X86PagingMethod32Bit::X86PagingMethod32Bit()
{
}


X86PagingMethod32Bit::~X86PagingMethod32Bit()
{
}


/*static*/ X86PagingMethod*
X86PagingMethod32Bit::Create()
{
	return new(&sMethod) X86PagingMethod32Bit;
}


status_t
X86PagingMethod32Bit::Init(kernel_args* args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	TRACE("vm_translation_map_init: entry\n");

	// page hole set up in stage2
	sPageHole = (page_table_entry *)args->arch_args.page_hole;
	// calculate where the pgdir would be
	sPageHolePageDir = (page_directory_entry*)
		(((addr_t)args->arch_args.page_hole)
			+ (B_PAGE_SIZE * 1024 - B_PAGE_SIZE));
	// clear out the bottom 2 GB, unmap everything
	memset(sPageHolePageDir + FIRST_USER_PGDIR_ENT, 0,
		sizeof(page_directory_entry) * NUM_USER_PGDIR_ENTS);

	sKernelPhysicalPageDirectory = args->arch_args.phys_pgdir;
	sKernelVirtualPageDirectory = (page_directory_entry*)
		args->arch_args.vir_pgdir;

#ifdef TRACE_X86_PAGING_METHOD_32_BIT
	TRACE("page hole: %p, page dir: %p\n", sPageHole, sPageHolePageDir);
	TRACE("page dir: %p (physical: %#" B_PRIx32 ")\n",
		sKernelVirtualPageDirectory, sKernelPhysicalPageDirectory);
#endif

	X86PagingStructures32Bit::StaticInit();

	// create the initial pool for the physical page mapper
	PhysicalPageSlotPool* pool
		= new(&PhysicalPageSlotPool::sInitialPhysicalPagePool)
			PhysicalPageSlotPool;
	status_t error = pool->InitInitial(args);
	if (error != B_OK) {
		panic("X86PagingMethod32Bit::Init(): Failed to create initial pool "
			"for physical page mapper!");
		return error;
	}

	// create physical page mapper
	large_memory_physical_page_ops_init(args, pool, sPhysicalPageMapper,
		sKernelPhysicalPageMapper);
		// TODO: Select the best page mapper!

	// enable global page feature if available
	if (x86_check_feature(IA32_FEATURE_PGE, FEATURE_COMMON)) {
		// this prevents kernel pages from being flushed from TLB on
		// context-switch
		x86_write_cr4(x86_read_cr4() | IA32_CR4_GLOBAL_PAGES);
	}

	TRACE("vm_translation_map_init: done\n");

	*_physicalPageMapper = sPhysicalPageMapper;
	return B_OK;
}


status_t
X86PagingMethod32Bit::InitPostArea(kernel_args* args)
{
	// now that the vm is initialized, create an area that represents
	// the page hole
	void *temp;
	status_t error;
	area_id area;

	// unmap the page hole hack we were using before
	sKernelVirtualPageDirectory[1023] = 0;
	sPageHolePageDir = NULL;
	sPageHole = NULL;

	temp = (void *)sKernelVirtualPageDirectory;
	area = create_area("kernel_pgdir", &temp, B_EXACT_ADDRESS, B_PAGE_SIZE,
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	error = PhysicalPageSlotPool::sInitialPhysicalPagePool
		.InitInitialPostArea(args);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
X86PagingMethod32Bit::CreateTranslationMap(bool kernel, VMTranslationMap** _map)
{
	X86VMTranslationMap32Bit* map = new(std::nothrow) X86VMTranslationMap32Bit;
	if (map == NULL)
		return B_NO_MEMORY;

	status_t error = map->Init(kernel);
	if (error != B_OK) {
		delete map;
		return error;
	}

	*_map = map;
	return B_OK;
}


status_t
X86PagingMethod32Bit::MapEarly(kernel_args* args, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	phys_addr_t (*get_free_page)(kernel_args*))
{
	// XXX horrible back door to map a page quickly regardless of translation
	// map object, etc. used only during VM setup.
	// uses a 'page hole' set up in the stage 2 bootloader. The page hole is
	// created by pointing one of the pgdir entries back at itself, effectively
	// mapping the contents of all of the 4MB of pagetables into a 4 MB region.
	// It's only used here, and is later unmapped.

	// check to see if a page table exists for this range
	int index = VADDR_TO_PDENT(virtualAddress);
	if ((sPageHolePageDir[index] & X86_PDE_PRESENT) == 0) {
		phys_addr_t pgtable;
		page_directory_entry *e;
		// we need to allocate a pgtable
		pgtable = get_free_page(args);
		// pgtable is in pages, convert to physical address
		pgtable *= B_PAGE_SIZE;

		TRACE("X86PagingMethod32Bit::MapEarly(): asked for free page for "
			"pgtable. %#" B_PRIxPHYSADDR "\n", pgtable);

		// put it in the pgdir
		e = &sPageHolePageDir[index];
		x86_put_pgtable_in_pgdir(e, pgtable, attributes);

		// zero it out in it's new mapping
		memset((unsigned int*)((addr_t)sPageHole
				+ (virtualAddress / B_PAGE_SIZE / 1024) * B_PAGE_SIZE),
			0, B_PAGE_SIZE);
	}

	ASSERT_PRINT(
		(sPageHole[virtualAddress / B_PAGE_SIZE] & X86_PTE_PRESENT) == 0,
		"virtual address: %#" B_PRIxADDR ", pde: %#" B_PRIx32
		", existing pte: %#" B_PRIx32, virtualAddress, sPageHolePageDir[index],
		sPageHole[virtualAddress / B_PAGE_SIZE]);

	// now, fill in the pentry
	put_page_table_entry_in_pgtable(sPageHole + virtualAddress / B_PAGE_SIZE,
		physicalAddress, attributes, 0, IS_KERNEL_ADDRESS(virtualAddress));

	return B_OK;
}


bool
X86PagingMethod32Bit::IsKernelPageAccessible(addr_t virtualAddress,
	uint32 protection)
{
	// We only trust the kernel team's page directory. So switch to it first.
	// Always set it to make sure the TLBs don't contain obsolete data.
	uint32 physicalPageDirectory;
	read_cr3(physicalPageDirectory);
	write_cr3(sKernelPhysicalPageDirectory);

	// get the page directory entry for the address
	page_directory_entry pageDirectoryEntry;
	uint32 index = VADDR_TO_PDENT(virtualAddress);

	if (physicalPageDirectory == sKernelPhysicalPageDirectory) {
		pageDirectoryEntry = sKernelVirtualPageDirectory[index];
	} else if (sPhysicalPageMapper != NULL) {
		// map the original page directory and get the entry
		void* handle;
		addr_t virtualPageDirectory;
		status_t error = sPhysicalPageMapper->GetPageDebug(
			physicalPageDirectory, &virtualPageDirectory, &handle);
		if (error == B_OK) {
			pageDirectoryEntry
				= ((page_directory_entry*)virtualPageDirectory)[index];
			sPhysicalPageMapper->PutPageDebug(virtualPageDirectory,
				handle);
		} else
			pageDirectoryEntry = 0;
	} else
		pageDirectoryEntry = 0;

	// map the page table and get the entry
	page_table_entry pageTableEntry;
	index = VADDR_TO_PTENT(virtualAddress);

	if ((pageDirectoryEntry & X86_PDE_PRESENT) != 0
			&& sPhysicalPageMapper != NULL) {
		void* handle;
		addr_t virtualPageTable;
		status_t error = sPhysicalPageMapper->GetPageDebug(
			pageDirectoryEntry & X86_PDE_ADDRESS_MASK, &virtualPageTable,
			&handle);
		if (error == B_OK) {
			pageTableEntry = ((page_table_entry*)virtualPageTable)[index];
			sPhysicalPageMapper->PutPageDebug(virtualPageTable, handle);
		} else
			pageTableEntry = 0;
	} else
		pageTableEntry = 0;

	// switch back to the original page directory
	if (physicalPageDirectory != sKernelPhysicalPageDirectory)
		write_cr3(physicalPageDirectory);

	if ((pageTableEntry & X86_PTE_PRESENT) == 0)
		return false;

	// present means kernel-readable, so check for writable
	return (protection & B_KERNEL_WRITE_AREA) == 0
		|| (pageTableEntry & X86_PTE_WRITABLE) != 0;
}
