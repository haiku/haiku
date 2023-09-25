/*
 * Copyright 2020-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   X512 <danger_mail@list.ru>
 */


#include "RISCV64VMTranslationMap.h"

#include <kernel.h>
#include <vm/vm_priv.h>
#include <vm/vm_page.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMCache.h>
#include <slab/Slab.h>
#include <platform/sbi/sbi_syscalls.h>

#include <util/AutoLock.h>
#include <util/ThreadAutoLock.h>


//#define DO_TRACE
#ifdef DO_TRACE
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif

#define NOT_IMPLEMENTED_PANIC() \
	panic("not implemented: %s\n", __PRETTY_FUNCTION__)

extern uint32 gPlatform;


static void
WriteVmPage(vm_page* page)
{
	dprintf("0x%08" B_PRIxADDR " ",
		(addr_t)(page->physical_page_number * B_PAGE_SIZE));
	switch (page->State()) {
		case PAGE_STATE_ACTIVE:
			dprintf("A");
			break;
		case PAGE_STATE_INACTIVE:
			dprintf("I");
			break;
		case PAGE_STATE_MODIFIED:
			dprintf("M");
			break;
		case PAGE_STATE_CACHED:
			dprintf("C");
			break;
		case PAGE_STATE_FREE:
			dprintf("F");
			break;
		case PAGE_STATE_CLEAR:
			dprintf("L");
			break;
		case PAGE_STATE_WIRED:
			dprintf("W");
			break;
		case PAGE_STATE_UNUSED:
			dprintf("-");
			break;
	}
	dprintf(" ");
	if (page->busy)
		dprintf("B");
	else
		dprintf("-");

	if (page->busy_writing)
		dprintf("W");
	else
		dprintf("-");

	if (page->accessed)
		dprintf("A");
	else
		dprintf("-");

	if (page->modified)
		dprintf("M");
	else
		dprintf("-");

	if (page->unused)
		dprintf("U");
	else
		dprintf("-");

	dprintf(" usage:%3u", page->usage_count);
	dprintf(" wired:%5u", page->WiredCount());

	bool first = true;
	vm_page_mappings::Iterator iterator = page->mappings.GetIterator();
	vm_page_mapping* mapping;
	while ((mapping = iterator.Next()) != NULL) {
		if (first) {
			dprintf(": ");
			first = false;
		} else
			dprintf(", ");

		dprintf("%" B_PRId32 " (%s)", mapping->area->id, mapping->area->name);
		mapping = mapping->page_link.next;
	}
}


static void
FreePageTable(page_num_t ppn, bool isKernel, uint32 level = 2)
{
	if (level > 0) {
		Pte* pte = (Pte*)VirtFromPhys(ppn * B_PAGE_SIZE);
		uint64 beg = 0;
		uint64 end = pteCount - 1;
		if (level == 2 && !isKernel) {
			beg = VirtAdrPte(USER_BASE, 2);
			end = VirtAdrPte(USER_TOP, 2);
		}
		for (uint64 i = beg; i <= end; i++) {
			if (pte[i].isValid)
				FreePageTable(pte[i].ppn, isKernel, level - 1);
		}
	}
	vm_page* page = vm_lookup_page(ppn);
	DEBUG_PAGE_ACCESS_START(page);
	vm_page_set_state(page, PAGE_STATE_FREE);
}


static uint64
GetPageTableSize(page_num_t ppn, bool isKernel, uint32 level = 2)
{
	if (ppn == 0)
		return 0;

	if (level == 0)
		return 1;

	uint64 size = 1;
	Pte* pte = (Pte*)VirtFromPhys(ppn * B_PAGE_SIZE);
	uint64 beg = 0;
	uint64 end = pteCount - 1;
	if (level == 2 && !isKernel) {
		beg = VirtAdrPte(USER_BASE, 2);
		end = VirtAdrPte(USER_TOP, 2);
	}
	for (uint64 i = beg; i <= end; i++) {
		if (pte[i].isValid)
			size += GetPageTableSize(pte[i].ppn, isKernel, level - 1);
	}
	return size;
}


//#pragma mark RISCV64VMTranslationMap


std::atomic<Pte>*
RISCV64VMTranslationMap::LookupPte(addr_t virtAdr, bool alloc,
	vm_page_reservation* reservation)
{
	if (fPageTable == 0) {
		if (!alloc)
			return NULL;
		vm_page* page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
		fPageTable = page->physical_page_number * B_PAGE_SIZE;
		if (fPageTable == 0)
			return NULL;
		DEBUG_PAGE_ACCESS_END(page);
		fPageTableSize++;
		if (!fIsKernel) {
			// Map kernel address space into user address space. Preallocated
			// kernel level-2 PTEs are reused.
			RISCV64VMTranslationMap* kernelMap = (RISCV64VMTranslationMap*)
				VMAddressSpace::Kernel()->TranslationMap();
			Pte *kernelPageTable = (Pte*)VirtFromPhys(kernelMap->PageTable());
			Pte *userPageTable = (Pte*)VirtFromPhys(fPageTable);
			for (uint64 i = VirtAdrPte(KERNEL_BASE, 2);
				i <= VirtAdrPte(KERNEL_TOP, 2); i++) {
				Pte *pte = &userPageTable[i];
				pte->ppn = kernelPageTable[i].ppn;
				pte->isValid = true;
			}
		}
	}
	auto pte = (std::atomic<Pte>*)VirtFromPhys(fPageTable);
	for (int level = 2; level > 0; level--) {
		pte += VirtAdrPte(virtAdr, level);
		if (!pte->load().isValid) {
			if (!alloc)
				return NULL;
			vm_page* page = vm_page_allocate_page(reservation,
				PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
			page_num_t ppn = page->physical_page_number;
			if (ppn == 0)
				return NULL;
			DEBUG_PAGE_ACCESS_END(page);
			fPageTableSize++;
			Pte newPte {
				.isValid = true,
				.isGlobal = fIsKernel,
				.ppn = ppn
			};
			pte->store(newPte);
		}
		pte = (std::atomic<Pte>*)VirtFromPhys(B_PAGE_SIZE * pte->load().ppn);
	}
	pte += VirtAdrPte(virtAdr, 0);
	return pte;
}


phys_addr_t
RISCV64VMTranslationMap::LookupAddr(addr_t virtAdr)
{
	std::atomic<Pte>* pte = LookupPte(virtAdr, false, NULL);
	if (pte == NULL)
		return 0;
	Pte pteVal = pte->load();
	if (!pteVal.isValid)
		return 0;
	if (fIsKernel != !pteVal.isUser)
		return 0;
	return pteVal.ppn * B_PAGE_SIZE;
}


RISCV64VMTranslationMap::RISCV64VMTranslationMap(bool kernel,
	phys_addr_t pageTable):
	fIsKernel(kernel),
	fPageTable(pageTable),
	fPageTableSize(GetPageTableSize(pageTable / B_PAGE_SIZE, kernel)),
	fInvalidPagesCount(0),
	fInvalidCode(false)
{
	TRACE("+RISCV64VMTranslationMap(%p, %d, 0x%" B_PRIxADDR ")\n", this,
		kernel, pageTable);
	TRACE("  pageTableSize: %" B_PRIu64 "\n", fPageTableSize);
}


RISCV64VMTranslationMap::~RISCV64VMTranslationMap()
{
	TRACE("-RISCV64VMTranslationMap(%p)\n", this);
	TRACE("  pageTableSize: %" B_PRIu64 "\n", fPageTableSize);
	TRACE("  GetPageTableSize(): %" B_PRIu64 "\n",
		GetPageTableSize(fPageTable / B_PAGE_SIZE, fIsKernel));

	ASSERT_ALWAYS(!fIsKernel);
	// Can't delete currently used page table
	ASSERT_ALWAYS(::Satp() != Satp());

	FreePageTable(fPageTable / B_PAGE_SIZE, fIsKernel);
}


bool
RISCV64VMTranslationMap::Lock()
{
	TRACE("RISCV64VMTranslationMap::Lock()\n");
	recursive_lock_lock(&fLock);
	return true;
}


void
RISCV64VMTranslationMap::Unlock()
{
	TRACE("RISCV64VMTranslationMap::Unlock()\n");
	if (recursive_lock_get_recursion(&fLock) == 1) {
		// we're about to release it for the last time
		Flush();
	}
	recursive_lock_unlock(&fLock);
}


addr_t
RISCV64VMTranslationMap::MappedSize() const
{
	NOT_IMPLEMENTED_PANIC();
	return 0;
}


size_t
RISCV64VMTranslationMap::MaxPagesNeededToMap(addr_t start, addr_t end) const
{
	enum {
		level0Range = (uint64_t)B_PAGE_SIZE * pteCount,
		level1Range = (uint64_t)level0Range * pteCount,
		level2Range = (uint64_t)level1Range * pteCount,
	};

	if (start == 0) {
		start = (level2Range) - B_PAGE_SIZE;
		end += start;
	}

	size_t requiredLevel2 = end / level2Range + 1 - start / level2Range;
	size_t requiredLevel1 = end / level1Range + 1 - start / level1Range;
	size_t requiredLevel0 = end / level0Range + 1 - start / level0Range;

	return requiredLevel2 + requiredLevel1 + requiredLevel0;
}


status_t
RISCV64VMTranslationMap::Map(addr_t virtualAddress, phys_addr_t physicalAddress,
	uint32 attributes, uint32 memoryType,
	vm_page_reservation* reservation)
{
	TRACE("RISCV64VMTranslationMap::Map(0x%" B_PRIxADDR ", 0x%" B_PRIxADDR
		")\n", virtualAddress, physicalAddress);

	ThreadCPUPinner pinner(thread_get_current_thread());

	std::atomic<Pte>* pte = LookupPte(virtualAddress, true, reservation);
	if (pte == NULL)
		panic("can't allocate page table");

	Pte newPte {
		.isValid = true,
		.isGlobal = fIsKernel,
		.ppn = physicalAddress / B_PAGE_SIZE
	};

	if ((attributes & B_USER_PROTECTION) != 0) {
		newPte.isUser = true;
		if ((attributes & B_READ_AREA) != 0)
			newPte.isRead = true;
		if ((attributes & B_WRITE_AREA) != 0)
			newPte.isWrite = true;
		if ((attributes & B_EXECUTE_AREA) != 0) {
			newPte.isExec = true;
			fInvalidCode = true;
		}
	} else {
		if ((attributes & B_KERNEL_READ_AREA) != 0)
			newPte.isRead = true;
		if ((attributes & B_KERNEL_WRITE_AREA) != 0)
			newPte.isWrite = true;
		if ((attributes & B_KERNEL_EXECUTE_AREA) != 0) {
			newPte.isExec = true;
			fInvalidCode = true;
		}
	}

	pte->store(newPte);

	// Note: We don't need to invalidate the TLB for this address, as previously
	// the entry was not present and the TLB doesn't cache those entries.

	fMapCount++;

	return B_OK;
}


status_t
RISCV64VMTranslationMap::Unmap(addr_t start, addr_t end)
{
	TRACE("RISCV64VMTranslationMap::Unmap(0x%" B_PRIxADDR ", 0x%" B_PRIxADDR
		")\n", start, end);

	ThreadCPUPinner pinner(thread_get_current_thread());

	for (addr_t page = start; page < end; page += B_PAGE_SIZE) {
		std::atomic<Pte>* pte = LookupPte(page, false, NULL);
		if (pte != NULL) {
			fMapCount--;
			Pte oldPte = pte->exchange({});
			if (oldPte.isAccessed)
				InvalidatePage(page);
		}
	}
	return B_OK;
}


status_t
RISCV64VMTranslationMap::DebugMarkRangePresent(addr_t start, addr_t end,
	bool markPresent)
{
	NOT_IMPLEMENTED_PANIC();
	return B_NOT_SUPPORTED;
}


/*
Things need to be done when unmapping VMArea pages
	update vm_page::accessed, modified
	MMIO pages:
		just unmap
	wired pages:
		decrement wired count
	non-wired pages:
		remove from VMArea and vm_page `mappings` list
	wired and non-wird pages
		vm_page_set_state
*/

status_t
RISCV64VMTranslationMap::UnmapPage(VMArea* area, addr_t address,
	bool updatePageQueue)
{
	TRACE("RISCV64VMTranslationMap::UnmapPage(0x%" B_PRIxADDR "(%s), 0x%"
		B_PRIxADDR ", %d)\n", (addr_t)area, area->name, address,
		updatePageQueue);

	ThreadCPUPinner pinner(thread_get_current_thread());

	std::atomic<Pte>* pte = LookupPte(address, false, NULL);
	if (pte == NULL || !pte->load().isValid)
		return B_ENTRY_NOT_FOUND;

	RecursiveLocker locker(fLock);

	Pte oldPte = pte->exchange({});
	fMapCount--;
	pinner.Unlock();

	if (oldPte.isAccessed)
		InvalidatePage(address);

	Flush();

	locker.Detach(); // PageUnmapped takes ownership
	PageUnmapped(area, oldPte.ppn, oldPte.isAccessed, oldPte.isDirty, updatePageQueue);
	return B_OK;
}


void
RISCV64VMTranslationMap::UnmapPages(VMArea* area, addr_t base, size_t size,
	bool updatePageQueue)
{
	TRACE("RISCV64VMTranslationMap::UnmapPages(0x%" B_PRIxADDR "(%s), 0x%"
		B_PRIxADDR ", 0x%" B_PRIxSIZE ", %d)\n", (addr_t)area,
		area->name, base, size, updatePageQueue);

	if (size == 0)
		return;

	addr_t end = base + size - 1;

	VMAreaMappings queue;
	RecursiveLocker locker(fLock);
	ThreadCPUPinner pinner(thread_get_current_thread());

	for (addr_t start = base; start < end; start += B_PAGE_SIZE) {
		std::atomic<Pte>* pte = LookupPte(start, false, NULL);
		if (pte == NULL)
			continue;

		Pte oldPte = pte->exchange({});
		if (!oldPte.isValid)
			continue;

		fMapCount--;

		if (oldPte.isAccessed)
			InvalidatePage(start);

		if (area->cache_type != CACHE_TYPE_DEVICE) {
			// get the page
			vm_page* page = vm_lookup_page(oldPte.ppn);
			ASSERT(page != NULL);
			if (false) {
				WriteVmPage(page); dprintf("\n");
			}

			DEBUG_PAGE_ACCESS_START(page);

			// transfer the accessed/dirty flags to the page
			page->accessed = oldPte.isAccessed;
			page->modified = oldPte.isDirty;

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

		// flush explicitly, since we directly use the lock
		Flush();
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
RISCV64VMTranslationMap::UnmapArea(VMArea* area, bool deletingAddressSpace,
	bool ignoreTopCachePageFlags)
{
	TRACE("RISCV64VMTranslationMap::UnmapArea(0x%" B_PRIxADDR "(%s), 0x%"
		B_PRIxADDR ", 0x%" B_PRIxSIZE ", %d, %d)\n", (addr_t)area,
		area->name, area->Base(), area->Size(), deletingAddressSpace,
		ignoreTopCachePageFlags);

	if (area->cache_type == CACHE_TYPE_DEVICE || area->wiring != B_NO_LOCK) {
		UnmapPages(area, area->Base(), area->Size(), true);
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
				+ ((page->cache_offset * B_PAGE_SIZE)
				- area->cache_offset);

			std::atomic<Pte>* pte = LookupPte(address, false, NULL);
			if (pte == NULL || !pte->load().isValid) {
				panic("page %p has mapping for area %p "
					"(%#" B_PRIxADDR "), but has no "
					"page table", page, area, address);
				continue;
			}

			Pte oldPte = pte->exchange({});

			// transfer the accessed/dirty flags to the page and
			// invalidate the mapping, if necessary
			if (oldPte.isAccessed) {
				page->accessed = true;

				if (!deletingAddressSpace)
					InvalidatePage(address);
			}

			if (oldPte.isDirty)
				page->modified = true;

			if (pageFullyUnmapped) {
				DEBUG_PAGE_ACCESS_START(page);

				if (cache->temporary) {
					vm_page_set_state(page,
						PAGE_STATE_INACTIVE);
				} else if (page->modified) {
					vm_page_set_state(page,
						PAGE_STATE_MODIFIED);
				} else {
					vm_page_set_state(page,
						PAGE_STATE_CACHED);
				}

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
RISCV64VMTranslationMap::Query(addr_t virtualAddress,
	phys_addr_t* _physicalAddress, uint32* _flags)
{
	*_flags = 0;
	*_physicalAddress = 0;

	ThreadCPUPinner pinner(thread_get_current_thread());

	if (fPageTable == 0)
		return B_OK;

	std::atomic<Pte>* pte = LookupPte(virtualAddress, false, NULL);
	if (pte == NULL)
		return B_OK;

	Pte pteVal = pte->load();
	*_physicalAddress = pteVal.ppn * B_PAGE_SIZE;

	if (pteVal.isValid)
		*_flags |= PAGE_PRESENT;
	if (pteVal.isDirty)
		*_flags |= PAGE_MODIFIED;
	if (pteVal.isAccessed)
		*_flags |= PAGE_ACCESSED;
	if (pteVal.isUser) {
		if (pteVal.isRead)
			*_flags |= B_READ_AREA;
		if (pteVal.isWrite)
			*_flags |= B_WRITE_AREA;
		if (pteVal.isExec)
			*_flags |= B_EXECUTE_AREA;
	} else {
		if (pteVal.isRead)
			*_flags |= B_KERNEL_READ_AREA;
		if (pteVal.isWrite)
			*_flags |= B_KERNEL_WRITE_AREA;
		if (pteVal.isExec)
			*_flags |= B_KERNEL_EXECUTE_AREA;
	}

	return B_OK;
}


status_t
RISCV64VMTranslationMap::QueryInterrupt(addr_t virtualAddress,
	phys_addr_t* _physicalAddress, uint32* _flags)
{
	return Query(virtualAddress, _physicalAddress, _flags);
}


status_t RISCV64VMTranslationMap::Protect(addr_t base, addr_t top,
	uint32 attributes, uint32 memoryType)
{
	TRACE("RISCV64VMTranslationMap::Protect(0x%" B_PRIxADDR ", 0x%"
		B_PRIxADDR ")\n", base, top);

	ThreadCPUPinner pinner(thread_get_current_thread());

	for (addr_t page = base; page < top; page += B_PAGE_SIZE) {

		std::atomic<Pte>* pte = LookupPte(page, false, NULL);
		if (pte == NULL || !pte->load().isValid) {
			TRACE("attempt to protect not mapped page: 0x%"
				B_PRIxADDR "\n", page);
			continue;
		}

		Pte oldPte {};
		Pte newPte {};
		while (true) {
			oldPte = pte->load();

			newPte = oldPte;
			if ((attributes & B_USER_PROTECTION) != 0) {
				newPte.isUser = true;
				newPte.isRead  = (attributes & B_READ_AREA)    != 0;
				newPte.isWrite = (attributes & B_WRITE_AREA)   != 0;
				newPte.isExec  = (attributes & B_EXECUTE_AREA) != 0;
			} else {
				newPte.isUser = false;
				newPte.isRead  = (attributes & B_KERNEL_READ_AREA)    != 0;
				newPte.isWrite = (attributes & B_KERNEL_WRITE_AREA)   != 0;
				newPte.isExec  = (attributes & B_KERNEL_EXECUTE_AREA) != 0;
			}

			if (pte->compare_exchange_strong(oldPte, newPte))
				break;
		}

		fInvalidCode = newPte.isExec;

		if (oldPte.isAccessed)
			InvalidatePage(page);
	}

	return B_OK;
}


status_t
RISCV64VMTranslationMap::ProtectPage(VMArea* area, addr_t address,
	uint32 attributes)
{
	NOT_IMPLEMENTED_PANIC();
	return B_OK;
}


status_t
RISCV64VMTranslationMap::ProtectArea(VMArea* area, uint32 attributes)
{
	NOT_IMPLEMENTED_PANIC();
	return B_NOT_SUPPORTED;
}


static inline uint64
ConvertAccessedFlags(uint32 flags)
{
	Pte pteFlags {
		.isAccessed = (flags & PAGE_ACCESSED) != 0,
		.isDirty = (flags & PAGE_MODIFIED) != 0
	};
	return pteFlags.val;
}


void
RISCV64VMTranslationMap::SetFlags(addr_t address, uint32 flags)
{
	// Only called from interrupt handler with interrupts disabled for CPUs that don't support
	// setting accessed/modified flags by hardware.

	std::atomic<Pte>* pte = LookupPte(address, false, NULL);
	if (pte == NULL || !pte->load().isValid)
		return;

	*(std::atomic<uint64>*)pte |= ConvertAccessedFlags(flags);

	if (IS_KERNEL_ADDRESS(address))
		FlushTlbPage(address);
	else
		FlushTlbPageAsid(address, 0);

	return;
}


status_t
RISCV64VMTranslationMap::ClearFlags(addr_t address, uint32 flags)
{
	ThreadCPUPinner pinner(thread_get_current_thread());

	std::atomic<Pte>* pte = LookupPte(address, false, NULL);
	if (pte == NULL || !pte->load().isValid)
		return B_OK;

	*(std::atomic<uint64>*)pte &= ~ConvertAccessedFlags(flags);
	InvalidatePage(address);
	return B_OK;
}


bool
RISCV64VMTranslationMap::ClearAccessedAndModified(VMArea* area, addr_t address,
	bool unmapIfUnaccessed, bool& _modified)
{
	TRACE("RISCV64VMPhysicalPageMapper::ClearAccessedAndModified(0x%"
		B_PRIxADDR "(%s), 0x%" B_PRIxADDR ", %d)\n", (addr_t)area,
		area->name, address, unmapIfUnaccessed);

	RecursiveLocker locker(fLock);
	ThreadCPUPinner pinner(thread_get_current_thread());

	std::atomic<Pte>* pte = LookupPte(address, false, NULL);
	if (pte == NULL || !pte->load().isValid)
		return false;

	Pte oldPte {};
	if (unmapIfUnaccessed) {
		for (;;) {
			oldPte = pte->load();
			if (!oldPte.isValid)
				return false;

			if (oldPte.isAccessed) {
				oldPte.val = ((std::atomic<uint64>*)pte)->fetch_and(
					~Pte {.isAccessed = true, .isDirty = true}.val);
				break;
			}
			if (pte->compare_exchange_strong(oldPte, {}))
				break;
		}
	} else {
		oldPte.val = ((std::atomic<uint64>*)pte)->fetch_and(
			~Pte {.isAccessed = true, .isDirty = true}.val);
	}

	pinner.Unlock();
	_modified = oldPte.isDirty;
	if (oldPte.isAccessed) {
		InvalidatePage(address);
		Flush();
		return true;
	}

	if (!unmapIfUnaccessed)
		return false;

	fMapCount--;

	locker.Detach(); // UnaccessedPageUnmapped takes ownership
	UnaccessedPageUnmapped(area, oldPte.ppn);
	return false;
}


void
RISCV64VMTranslationMap::Flush()
{
	// copy of X86VMTranslationMap::Flush
	// TODO: move to common VMTranslationMap class

	if (fInvalidPagesCount <= 0)
		return;

	ThreadCPUPinner pinner(thread_get_current_thread());

	if (fInvalidPagesCount > PAGE_INVALIDATE_CACHE_SIZE) {
		// invalidate all pages
		TRACE("flush_tmap: %d pages to invalidate, invalidate all\n",
			fInvalidPagesCount);

		if (fIsKernel) {
			arch_cpu_global_TLB_invalidate();

			smp_send_broadcast_ici(SMP_MSG_GLOBAL_INVALIDATE_PAGES, 0, 0, 0,
				NULL, SMP_MSG_FLAG_SYNC);
		} else {
			cpu_status state = disable_interrupts();
			arch_cpu_user_TLB_invalidate();
			restore_interrupts(state);

			int cpu = smp_get_current_cpu();
			CPUSet cpuMask = fActiveOnCpus;
			cpuMask.ClearBit(cpu);

			if (!cpuMask.IsEmpty()) {
				smp_send_multicast_ici(cpuMask, SMP_MSG_USER_INVALIDATE_PAGES,
					0, 0, 0, NULL, SMP_MSG_FLAG_SYNC);
			}
		}
	} else {
		TRACE("flush_tmap: %d pages to invalidate, invalidate list\n",
			fInvalidPagesCount);

		arch_cpu_invalidate_TLB_list(fInvalidPages, fInvalidPagesCount);

		if (fIsKernel) {
			smp_send_broadcast_ici(SMP_MSG_INVALIDATE_PAGE_LIST,
				(addr_t)fInvalidPages, fInvalidPagesCount, 0, NULL,
				SMP_MSG_FLAG_SYNC);
		} else {
			int cpu = smp_get_current_cpu();
			CPUSet cpuMask = fActiveOnCpus;
			cpuMask.ClearBit(cpu);

			if (!cpuMask.IsEmpty()) {
				smp_send_multicast_ici(cpuMask, SMP_MSG_INVALIDATE_PAGE_LIST,
					(addr_t)fInvalidPages, fInvalidPagesCount, 0, NULL,
					SMP_MSG_FLAG_SYNC);
			}
		}
	}
	fInvalidPagesCount = 0;

	if (fInvalidCode) {
		FenceI();

		int cpu = smp_get_current_cpu();
		CPUSet cpuMask = fActiveOnCpus;
		cpuMask.ClearBit(cpu);

		if (!cpuMask.IsEmpty()) {
			switch (gPlatform) {
				case kPlatformSbi: {
					uint64 hartMask = 0;
					int32 cpuCount = smp_get_num_cpus();
					for (int32 i = 0; i < cpuCount; i++) {
						if (cpuMask.GetBit(i))
							hartMask |= (uint64)1 << gCPU[i].arch.hartId;
					}
					// TODO: handle hart ID >= 64
					memory_full_barrier();
					sbi_remote_fence_i(hartMask, 0);
					break;
				}
			}
		}
		fInvalidCode = false;
	}
}


void
RISCV64VMTranslationMap::DebugPrintMappingInfo(addr_t virtualAddress)
{
	NOT_IMPLEMENTED_PANIC();
}


bool
RISCV64VMTranslationMap::DebugGetReverseMappingInfo(phys_addr_t physicalAddress,
	ReverseMappingInfoCallback& callback)
{
	NOT_IMPLEMENTED_PANIC();
	return false;
}


status_t
RISCV64VMTranslationMap::MemcpyToMap(addr_t to, const char *from, size_t size)
{
	TRACE("RISCV64VMPhysicalPageMapper::MemcpyToMap(0x%" B_PRIxADDR ", 0x%"
		B_PRIxADDR ", %" B_PRIuSIZE ")\n", to, (addr_t)from, size);

	while (size > 0) {
		uint64 va0 = ROUNDDOWN(to, B_PAGE_SIZE);
		uint64 pa0 = LookupAddr(va0);
		TRACE("LookupAddr(0x%" B_PRIxADDR "): 0x%" B_PRIxADDR "\n",
			va0, pa0);

		if (pa0 == 0) {
			TRACE("[!] not mapped: 0x%" B_PRIxADDR "\n", va0);
			return B_BAD_ADDRESS;
		}

		uint64 n = B_PAGE_SIZE - (to - va0);
		if (n > size)
			n = size;

		memcpy(VirtFromPhys(pa0 + (to - va0)), from, n);

		size -= n;
		from += n;
		to = va0 + B_PAGE_SIZE;
	}
	return B_OK;
}


status_t
RISCV64VMTranslationMap::MemcpyFromMap(char *to, addr_t from, size_t size)
{
	TRACE("RISCV64VMPhysicalPageMapper::MemcpyFromMap(0x%" B_PRIxADDR
		", 0x%" B_PRIxADDR ", %" B_PRIuSIZE ")\n",
		(addr_t)to, from, size);

	while (size > 0) {
		uint64 va0 = ROUNDDOWN(from, B_PAGE_SIZE);
		uint64 pa0 = LookupAddr(va0);
		if (pa0 == 0) {
			TRACE("[!] not mapped: 0x%" B_PRIxADDR
				", calling page fault handler\n", va0);

			addr_t newIP;
			vm_page_fault(va0, Ra(), true, false, true, &newIP);

			pa0 = LookupAddr(va0);
			TRACE("LookupAddr(0x%" B_PRIxADDR "): 0x%"
				B_PRIxADDR "\n", va0, pa0);

			if (pa0 == 0)
				return B_BAD_ADDRESS;
		}
		uint64 n = B_PAGE_SIZE - (from - va0);
		if(n > size)
			n = size;

		memcpy(to, VirtFromPhys(pa0 + (from - va0)), n);

		size -= n;
		to += n;
		from = va0 + B_PAGE_SIZE;
	}

	return B_OK;
}


status_t
RISCV64VMTranslationMap::MemsetToMap(addr_t to, char c, size_t count)
{
	TRACE("RISCV64VMPhysicalPageMapper::MemsetToMap(0x%" B_PRIxADDR
		", %d, %" B_PRIuSIZE ")\n", to, c, count);

	while (count > 0) {
		uint64 va0 = ROUNDDOWN(to, B_PAGE_SIZE);
		uint64 pa0 = LookupAddr(va0);
		TRACE("LookupAddr(0x%" B_PRIxADDR "): 0x%" B_PRIxADDR "\n",
			va0, pa0);

		if (pa0 == 0) {
			TRACE("[!] not mapped: 0x%" B_PRIxADDR
				", calling page fault handler\n", va0);
			addr_t newIP;
			vm_page_fault(va0, Ra(), true, false, true, &newIP);
			pa0 = LookupAddr(va0);
			TRACE("LookupAddr(0x%" B_PRIxADDR "): 0x%"
				B_PRIxADDR "\n", va0, pa0);

			if (pa0 == 0)
				return B_BAD_ADDRESS;
		}

		uint64 n = B_PAGE_SIZE - (to - va0);
		if (n > count)
			n = count;

		memset(VirtFromPhys(pa0 + (to - va0)), c, n);

		count -= n;
		to = va0 + B_PAGE_SIZE;
	}
	return B_OK;
}


ssize_t
RISCV64VMTranslationMap::StrlcpyFromMap(char *to, addr_t from, size_t size)
{
	// NOT_IMPLEMENTED_PANIC();
	return strlcpy(to, (const char*)from, size);
	// return 0;
}


ssize_t
RISCV64VMTranslationMap::StrlcpyToMap(addr_t to, const char *from, size_t size)
{
	ssize_t len = strlen(from) + 1;
	if ((size_t)len > size)
		len = size;

	if (MemcpyToMap(to, from, len) < B_OK)
		return 0;

	return len;
}


//#pragma mark RISCV64VMPhysicalPageMapper


RISCV64VMPhysicalPageMapper::RISCV64VMPhysicalPageMapper()
{
	TRACE("+RISCV64VMPhysicalPageMapper\n");
}


RISCV64VMPhysicalPageMapper::~RISCV64VMPhysicalPageMapper()
{
	TRACE("-RISCV64VMPhysicalPageMapper\n");
}


status_t
RISCV64VMPhysicalPageMapper::GetPage(phys_addr_t physicalAddress,
	addr_t* _virtualAddress, void** _handle)
{
	*_virtualAddress = (addr_t)VirtFromPhys(physicalAddress);
	*_handle = (void*)1;
	return B_OK;
}


status_t
RISCV64VMPhysicalPageMapper::PutPage(addr_t virtualAddress, void* handle)
{
	return B_OK;
}


status_t
RISCV64VMPhysicalPageMapper::GetPageCurrentCPU( phys_addr_t physicalAddress,
	addr_t* _virtualAddress, void** _handle)
{
	return GetPage(physicalAddress, _virtualAddress, _handle);
}


status_t
RISCV64VMPhysicalPageMapper::PutPageCurrentCPU(addr_t virtualAddress,
	void* _handle)
{
	return PutPage(virtualAddress, _handle);
}


status_t
RISCV64VMPhysicalPageMapper::GetPageDebug(phys_addr_t physicalAddress,
	addr_t* _virtualAddress, void** _handle)
{
	NOT_IMPLEMENTED_PANIC();
	return B_NOT_SUPPORTED;
}


status_t
RISCV64VMPhysicalPageMapper::PutPageDebug(addr_t virtualAddress, void* handle)
{
	NOT_IMPLEMENTED_PANIC();
	return B_NOT_SUPPORTED;
}


status_t
RISCV64VMPhysicalPageMapper::MemsetPhysical(phys_addr_t address, int value,
	phys_size_t length)
{
	TRACE("RISCV64VMPhysicalPageMapper::MemsetPhysical(0x%" B_PRIxADDR
		", 0x%x, 0x%" B_PRIxADDR ")\n", address, value, length);
	return user_memset(VirtFromPhys(address), value, length);
}


status_t
RISCV64VMPhysicalPageMapper::MemcpyFromPhysical(void* to, phys_addr_t from,
	size_t length, bool user)
{
	TRACE("RISCV64VMPhysicalPageMapper::MemcpyFromPhysical(0x%" B_PRIxADDR
		", 0x%" B_PRIxADDR ", %" B_PRIuSIZE ")\n", (addr_t)to,
		from, length);
	return user_memcpy(to, VirtFromPhys(from), length);
}


status_t
RISCV64VMPhysicalPageMapper::MemcpyToPhysical(phys_addr_t to, const void* from,
	size_t length, bool user)
{
	TRACE("RISCV64VMPhysicalPageMapper::MemcpyToPhysical(0x%" B_PRIxADDR
		", 0x%" B_PRIxADDR ", %" B_PRIuSIZE ")\n", to, (addr_t)from,
		length);
	return user_memcpy(VirtFromPhys(to), from, length);
}


void
RISCV64VMPhysicalPageMapper::MemcpyPhysicalPage(phys_addr_t to,
	phys_addr_t from)
{
	TRACE("RISCV64VMPhysicalPageMapper::MemcpyPhysicalPage(0x%" B_PRIxADDR
		", 0x%" B_PRIxADDR ")\n", to, from);
	user_memcpy(VirtFromPhys(to), VirtFromPhys(from), B_PAGE_SIZE);
}
