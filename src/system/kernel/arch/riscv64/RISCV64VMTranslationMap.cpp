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

#include <util/AutoLock.h>


//#define DISABLE_MODIFIED_FLAGS 1

//#define DO_TRACE
#ifdef DO_TRACE
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif

#define NOT_IMPLEMENTED_PANIC() \
	panic("not implemented: %s\n", __PRETTY_FUNCTION__)


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
			if ((1 << pteValid) & pte[i].flags)
				FreePageTable(pte[i].ppn, isKernel, level - 1);
		}
	}
	vm_page* page = vm_lookup_page(ppn);
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
		if ((1 << pteValid) & pte[i].flags)
			size += GetPageTableSize(pte[i].ppn, isKernel, level - 1);
	}
	return size;
}


//#pragma mark RISCV64VMTranslationMap


Pte*
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
				pte->flags |= (1 << pteValid);
			}
		}
	}
	Pte *pte = (Pte*)VirtFromPhys(fPageTable);
	for (int level = 2; level > 0; level--) {
		pte += VirtAdrPte(virtAdr, level);
		if (!((1 << pteValid) & pte->flags)) {
			if (!alloc)
				return NULL;
			vm_page* page = vm_page_allocate_page(reservation,
				PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
			pte->ppn = page->physical_page_number;
			if (pte->ppn == 0)
				return NULL;
			fPageTableSize++;
			pte->flags |= (1 << pteValid);
		}
		pte = (Pte*)VirtFromPhys(B_PAGE_SIZE * pte->ppn);
	}
	pte += VirtAdrPte(virtAdr, 0);
	return pte;
}


phys_addr_t
RISCV64VMTranslationMap::LookupAddr(addr_t virtAdr)
{
	Pte* pte = LookupPte(virtAdr, false, NULL);
	if (pte == NULL || !((1 << pteValid) & pte->flags))
		return 0;
	if (fIsKernel != (((1 << pteUser) & pte->flags) == 0))
		return 0;
	return pte->ppn * B_PAGE_SIZE;
}


RISCV64VMTranslationMap::RISCV64VMTranslationMap(bool kernel,
	phys_addr_t pageTable):
	fIsKernel(kernel),
	fPageTable(pageTable),
	fPageTableSize(GetPageTableSize(pageTable / B_PAGE_SIZE, kernel))
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

	Pte* pte = LookupPte(virtualAddress, true, reservation);
	if (pte == NULL)
		panic("can't allocate page table");

	pte->ppn = physicalAddress / B_PAGE_SIZE;
	pte->flags = 0;
	if ((attributes & B_USER_PROTECTION) != 0) {
		pte->flags |= (1 << pteUser);
		if ((attributes & B_READ_AREA) != 0)
			pte->flags |= (1 << pteRead);
		if ((attributes & B_WRITE_AREA) != 0)
			pte->flags |= (1 << pteWrite);
		if ((attributes & B_EXECUTE_AREA) != 0)
			pte->flags |= (1 << pteExec);
	} else {
		if ((attributes & B_KERNEL_READ_AREA) != 0)
			pte->flags |= (1 << pteRead);
		if ((attributes & B_KERNEL_WRITE_AREA) != 0)
			pte->flags |= (1 << pteWrite);
		if ((attributes & B_KERNEL_EXECUTE_AREA) != 0)
			pte->flags |= (1 << pteExec);
	}

	pte->flags |= (1 << pteValid)
#ifdef DISABLE_MODIFIED_FLAGS
		| (1 << pteAccessed) | (1 << pteDirty)
#endif
	;

	FlushTlbPage(virtualAddress);

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
		Pte* pte = LookupPte(page, false, NULL);
		if (pte != NULL) {
			fMapCount--;
			pte->flags = 0;
			pte->ppn = 0;
			FlushTlbPage(page);
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

	Pte* pte = LookupPte(address, false, NULL);
	if (pte == NULL || ((1 << pteValid) & pte->flags) == 0)
		return B_ENTRY_NOT_FOUND;

	RecursiveLocker locker(fLock);

	Pte oldPte = *pte;
	pte->flags = 0;
	pte->ppn = 0;
	fMapCount--;
	FlushTlbPage(address);
	pinner.Unlock();

	locker.Detach(); // PageUnmapped takes ownership
	PageUnmapped(area, oldPte.ppn, ((1 << pteAccessed) & oldPte.flags) != 0,
		((1 << pteDirty) & oldPte.flags) != 0, updatePageQueue);
	return B_OK;
}


void
RISCV64VMTranslationMap::UnmapPages(VMArea* area, addr_t base, size_t size,
	bool updatePageQueue)
{
	TRACE("RISCV64VMTranslationMap::UnmapPages(0x%" B_PRIxADDR "(%s), 0x%"
		B_PRIxADDR ", 0x%" B_PRIxSIZE ", %d)\n", (addr_t)area,
		area->name, base, size, updatePageQueue);

	for (addr_t end = base + size; base < end; base += B_PAGE_SIZE)
		UnmapPage(area, base, updatePageQueue);
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

			Pte* pte = LookupPte(address, false, NULL);
			if (pte == NULL
				|| ((1 << pteValid) & pte->flags) == 0) {
				panic("page %p has mapping for area %p "
					"(%#" B_PRIxADDR "), but has no "
					"page table", page, area, address);
				continue;
			}

			Pte oldPte = *pte;
			pte->flags = 0;
			pte->ppn = 0;

			// transfer the accessed/dirty flags to the page and
			// invalidate the mapping, if necessary
			if (((1 << pteAccessed) & oldPte.flags) != 0) {
				page->accessed = true;

				if (!deletingAddressSpace)
					FlushTlbPage(address);
			}

			if (((1 << pteDirty) & oldPte.flags) != 0)
				page->modified = true;

			if (pageFullyUnmapped) {
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

	Pte* pte = LookupPte(virtualAddress, false, NULL);
	if (pte == 0)
		return B_OK;

	*_physicalAddress = pte->ppn * B_PAGE_SIZE;

	if (((1 << pteValid)    & pte->flags) != 0)
		*_flags |= PAGE_PRESENT;
#ifndef DISABLE_MODIFIED_FLAGS
	if (((1 << pteDirty)    & pte->flags) != 0)
		*_flags |= PAGE_MODIFIED;
	if (((1 << pteAccessed) & pte->flags) != 0)
		*_flags |= PAGE_ACCESSED;
#endif
	if (((1 << pteUser) & pte->flags) != 0) {
		if (((1 << pteRead)  & pte->flags) != 0)
			*_flags |= B_READ_AREA;
		if (((1 << pteWrite) & pte->flags) != 0)
			*_flags |= B_WRITE_AREA;
		if (((1 << pteExec)  & pte->flags) != 0)
			*_flags |= B_EXECUTE_AREA;
	} else {
		if (((1 << pteRead)  & pte->flags) != 0)
			*_flags |= B_KERNEL_READ_AREA;
		if (((1 << pteWrite) & pte->flags) != 0)
			*_flags |= B_KERNEL_WRITE_AREA;
		if (((1 << pteExec)  & pte->flags) != 0)
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

		Pte* pte = LookupPte(page, false, NULL);
		if (pte == NULL || ((1 << pteValid) & pte->flags) == 0) {
			TRACE("attempt to protect not mapped page: 0x%"
				B_PRIxADDR "\n", page);
			continue;
		}

		Pte newPte = *pte;
		newPte.flags &= (1 << pteValid)
			| (1 << pteAccessed) | (1 << pteDirty);

		if ((attributes & B_USER_PROTECTION) != 0) {
			newPte.flags |= (1 << pteUser);
			if ((attributes & B_READ_AREA)    != 0)
				newPte.flags |= (1 << pteRead);
			if ((attributes & B_WRITE_AREA)   != 0)
				newPte.flags |= (1 << pteWrite);
			if ((attributes & B_EXECUTE_AREA) != 0)
				newPte.flags |= (1 << pteExec);
		} else {
			if ((attributes & B_KERNEL_READ_AREA)    != 0)
				newPte.flags |= (1 << pteRead);
			if ((attributes & B_KERNEL_WRITE_AREA)   != 0)
				newPte.flags |= (1 << pteWrite);
			if ((attributes & B_KERNEL_EXECUTE_AREA) != 0)
				newPte.flags |= (1 << pteExec);
		}
		*pte = newPte;

		FlushTlbPage(page);
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


static inline uint32
ConvertAccessedFlags(uint32 flags)
{
	return ((flags & PAGE_MODIFIED) ? (1 << pteDirty   ) : 0)
		| ((flags & PAGE_ACCESSED) ? (1 << pteAccessed) : 0);
}


status_t
RISCV64VMTranslationMap::SetFlags(addr_t address, uint32 flags)
{
	ThreadCPUPinner pinner(thread_get_current_thread());
	Pte* pte = LookupPte(address, false, NULL);
	if (pte == NULL || ((1 << pteValid) & pte->flags) == 0)
		return B_OK;
#ifndef DISABLE_MODIFIED_FLAGS
	pte->flags |= ConvertAccessedFlags(flags);
#endif
	FlushTlbPage(address);
	return B_OK;
}


status_t
RISCV64VMTranslationMap::ClearFlags(addr_t address, uint32 flags)
{
	ThreadCPUPinner pinner(thread_get_current_thread());

	Pte* pte = LookupPte(address, false, NULL);
	if (pte == NULL || ((1 << pteValid) & pte->flags) == 0)
		return B_OK;

#ifndef DISABLE_MODIFIED_FLAGS
	pte->flags &= ~ConvertAccessedFlags(flags);
#endif

	FlushTlbPage(address);
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

	Pte* pte = LookupPte(address, false, NULL);
	if (pte == NULL || ((1 << pteValid) & pte->flags) == 0)
		return false;

	Pte oldPte = *pte;

#ifndef DISABLE_MODIFIED_FLAGS
	if (unmapIfUnaccessed) {
		if (((1 << pteAccessed) & pte->flags) != 0) {
			pte->flags &= ~((1 << pteAccessed) | (1 << pteDirty));
		} else {
			pte->flags = 0;
			pte->ppn = 0;
		}
	} else {
		pte->flags &= ~((1 << pteAccessed) | (1 << pteDirty));
	}
#endif

	pinner.Unlock();
	_modified = ((1 << pteDirty) & oldPte.flags) != 0;
	if (((1 << pteAccessed) & oldPte.flags) != 0) {
		FlushTlbPage(address);
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
	//NOT_IMPLEMENTED_PANIC();
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
			vm_page_fault(va0, Ra(), true, false, true, true,
				&newIP);

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
			vm_page_fault(va0, Ra(), true, false, true, true,
				&newIP);
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
	set_ac();
	memset(VirtFromPhys(address), value, length);
	clear_ac();

	return B_OK;
}


status_t
RISCV64VMPhysicalPageMapper::MemcpyFromPhysical(void* to, phys_addr_t from,
	size_t length, bool user)
{
	TRACE("RISCV64VMPhysicalPageMapper::MemcpyFromPhysical(0x%" B_PRIxADDR
		", 0x%" B_PRIxADDR ", %" B_PRIuSIZE ")\n", (addr_t)to,
		from, length);

	set_ac();
	memcpy(to, VirtFromPhys(from), length);
	clear_ac();

	return B_OK;
}


status_t
RISCV64VMPhysicalPageMapper::MemcpyToPhysical(phys_addr_t to, const void* from,
	size_t length, bool user)
{
	TRACE("RISCV64VMPhysicalPageMapper::MemcpyToPhysical(0x%" B_PRIxADDR
		", 0x%" B_PRIxADDR ", %" B_PRIuSIZE ")\n", to, (addr_t)from,
		length);

	set_ac();
	memcpy(VirtFromPhys(to), from, length);
	clear_ac();

	return B_OK;
}


void
RISCV64VMPhysicalPageMapper::MemcpyPhysicalPage(phys_addr_t to,
	phys_addr_t from)
{
	TRACE("RISCV64VMPhysicalPageMapper::MemcpyPhysicalPage(0x%" B_PRIxADDR
		", 0x%" B_PRIxADDR ")\n", to, from);

	set_ac();
	memcpy(VirtFromPhys(to), VirtFromPhys(from), B_PAGE_SIZE);
	clear_ac();
}
