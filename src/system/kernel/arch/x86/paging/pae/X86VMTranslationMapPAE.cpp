/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/pae/X86VMTranslationMapPAE.h"

#include <int.h>
#include <slab/Slab.h>
#include <thread.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <util/ThreadAutoLock.h>
#include <vm/vm_page.h>
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


#if TRANSLATION_MAP_TRACING


namespace TranslationMapTracing {


class TranslationMapTraceEntryBase
	: public TRACE_ENTRY_SELECTOR(TRANSLATION_MAP_TRACING_STACK_TRACE) {
public:
	TranslationMapTraceEntryBase()
		:
		TraceEntryBase(TRANSLATION_MAP_TRACING_STACK_TRACE, 0, true)
	{
	}

	void PrintPageTableEntry(TraceOutput& out, pae_page_table_entry entry)
	{
		out.Print("%#" B_PRIx64  " %c%c%c%c%c %s %s %c%c",
			entry & X86_PAE_PTE_ADDRESS_MASK,
			(entry & X86_PAE_PTE_PRESENT) != 0 ? 'P' : '-',
			(entry & X86_PAE_PTE_WRITABLE) != 0 ? 'W' : '-',
			(entry & X86_PAE_PTE_USER) != 0 ? 'U' : '-',
			(entry & X86_PAE_PTE_NOT_EXECUTABLE) != 0 ? '-' : 'X',
			(entry & X86_PAE_PTE_GLOBAL) != 0 ? 'G' : '-',
			(entry & X86_PAE_PTE_WRITE_THROUGH) != 0 ? "WT" : "--",
			(entry & X86_PAE_PTE_CACHING_DISABLED) != 0 ? "UC" : "--",
			(entry & X86_PAE_PTE_ACCESSED) != 0 ? 'A' : '-',
			(entry & X86_PAE_PTE_DIRTY) != 0 ? 'D' : '-');
	}
};


class Map : public TranslationMapTraceEntryBase {
public:
	Map(X86VMTranslationMapPAE* map, addr_t virtualAddress,
		pae_page_table_entry entry)
		:
		TranslationMapTraceEntryBase(),
		fMap(map),
		fVirtualAddress(virtualAddress),
		fEntry(entry)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("translation map map: %p: %#" B_PRIxADDR " -> ", fMap,
			fVirtualAddress);
		PrintPageTableEntry(out, fEntry);
	}

private:
	X86VMTranslationMapPAE*	fMap;
	addr_t					fVirtualAddress;
	pae_page_table_entry	fEntry;
};


class Unmap : public TranslationMapTraceEntryBase {
public:
	Unmap(X86VMTranslationMapPAE* map, addr_t virtualAddress,
		pae_page_table_entry entry)
		:
		TranslationMapTraceEntryBase(),
		fMap(map),
		fVirtualAddress(virtualAddress),
		fEntry(entry)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("translation map unmap: %p: %#" B_PRIxADDR
			" -> ", fMap, fVirtualAddress);
		PrintPageTableEntry(out, fEntry);
	}

private:
	X86VMTranslationMapPAE*	fMap;
	addr_t					fVirtualAddress;
	pae_page_table_entry	fEntry;
};


class Protect : public TranslationMapTraceEntryBase {
public:
	Protect(X86VMTranslationMapPAE* map, addr_t virtualAddress,
		pae_page_table_entry oldEntry, pae_page_table_entry newEntry)
		:
		TranslationMapTraceEntryBase(),
		fMap(map),
		fVirtualAddress(virtualAddress),
		fOldEntry(oldEntry),
		fNewEntry(newEntry)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("translation map protect: %p: %#" B_PRIxADDR
			" -> ", fMap, fVirtualAddress);
		PrintPageTableEntry(out, fNewEntry);
		out.Print(" (%c%c%c)",
			(fOldEntry & X86_PAE_PTE_WRITABLE) != 0 ? 'W' : '-',
			(fOldEntry & X86_PAE_PTE_USER) != 0 ? 'U' : '-',
			(fOldEntry & X86_PAE_PTE_NOT_EXECUTABLE) != 0 ? '-' : 'X');
	}

private:
	X86VMTranslationMapPAE*	fMap;
	addr_t					fVirtualAddress;
	pae_page_table_entry	fOldEntry;
	pae_page_table_entry	fNewEntry;
};


class ClearFlags : public TranslationMapTraceEntryBase {
public:
	ClearFlags(X86VMTranslationMapPAE* map, addr_t virtualAddress,
		pae_page_table_entry oldEntry, pae_page_table_entry flagsCleared)
		:
		TranslationMapTraceEntryBase(),
		fMap(map),
		fVirtualAddress(virtualAddress),
		fOldEntry(oldEntry),
		fFlagsCleared(flagsCleared)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("translation map clear flags: %p: %#" B_PRIxADDR
			" -> ", fMap, fVirtualAddress);
		PrintPageTableEntry(out, fOldEntry & ~fFlagsCleared);
		out.Print(", cleared %c%c (%c%c)",
			(fOldEntry & fFlagsCleared & X86_PAE_PTE_ACCESSED) != 0 ? 'A' : '-',
			(fOldEntry & fFlagsCleared & X86_PAE_PTE_DIRTY) != 0 ? 'D' : '-',
			(fFlagsCleared & X86_PAE_PTE_ACCESSED) != 0 ? 'A' : '-',
			(fFlagsCleared & X86_PAE_PTE_DIRTY) != 0 ? 'D' : '-');
	}

private:
	X86VMTranslationMapPAE*	fMap;
	addr_t					fVirtualAddress;
	pae_page_table_entry	fOldEntry;
	pae_page_table_entry	fFlagsCleared;
};


class ClearFlagsUnmap : public TranslationMapTraceEntryBase {
public:
	ClearFlagsUnmap(X86VMTranslationMapPAE* map, addr_t virtualAddress,
		pae_page_table_entry entry)
		:
		TranslationMapTraceEntryBase(),
		fMap(map),
		fVirtualAddress(virtualAddress),
		fEntry(entry)
	{
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("translation map clear flags unmap: %p: %#" B_PRIxADDR
			" -> ", fMap, fVirtualAddress);
		PrintPageTableEntry(out, fEntry);
	}

private:
	X86VMTranslationMapPAE*	fMap;
	addr_t					fVirtualAddress;
	pae_page_table_entry	fEntry;
};


}	// namespace TranslationMapTracing

#	define T(x)	new(std::nothrow) TranslationMapTracing::x

#else
#	define T(x)
#endif	// TRANSLATION_MAP_TRACING



X86VMTranslationMapPAE::X86VMTranslationMapPAE()
	:
	fPagingStructures(NULL)
{
}


X86VMTranslationMapPAE::~X86VMTranslationMapPAE()
{
	if (fPagingStructures == NULL)
		return;

	if (fPageMapper != NULL)
		fPageMapper->Delete();

	// cycle through and free all of the user space page tables

	STATIC_ASSERT(KERNEL_BASE == 0x80000000 && KERNEL_SIZE == 0x80000000);
		// assuming 1-1 split of the address space

	for (uint32 k = 0; k < 2; k++) {
		pae_page_directory_entry* pageDir
			= fPagingStructures->VirtualPageDirs()[k];
		if (pageDir == NULL)
			continue;

		for (uint32 i = 0; i < kPAEPageDirEntryCount; i++) {
			if ((pageDir[i] & X86_PAE_PDE_PRESENT) != 0) {
				phys_addr_t address = pageDir[i] & X86_PAE_PDE_ADDRESS_MASK;
				vm_page* page = vm_lookup_page(address / B_PAGE_SIZE);
				if (page == NULL)
					panic("X86VMTranslationMapPAE::~X86VMTranslationMapPAE: "
						"didn't find page table page: page address: %#"
						B_PRIxPHYSADDR ", virtual base: %#" B_PRIxADDR "\n",
						address,
						(k * kPAEPageDirEntryCount + i) * kPAEPageTableRange);
				DEBUG_PAGE_ACCESS_START(page);
				vm_page_set_state(page, PAGE_STATE_FREE);
			}
		}
	}

	fPagingStructures->RemoveReference();
}


status_t
X86VMTranslationMapPAE::Init(bool kernel)
{
	TRACE("X86VMTranslationMapPAE::Init()\n");

	X86VMTranslationMap::Init(kernel);

	fPagingStructures = new(std::nothrow) X86PagingStructuresPAE;
	if (fPagingStructures == NULL)
		return B_NO_MEMORY;

	X86PagingMethodPAE* method = X86PagingMethodPAE::Method();

	if (kernel) {
		// kernel
		// get the physical page mapper
		fPageMapper = method->KernelPhysicalPageMapper();

		// we already know the kernel pgdir mapping
		fPagingStructures->Init(method->KernelVirtualPageDirPointerTable(),
			method->KernelPhysicalPageDirPointerTable(), NULL,
			method->KernelVirtualPageDirs(), method->KernelPhysicalPageDirs());
	} else {
		// user
		// allocate a physical page mapper
		status_t error = method->PhysicalPageMapper()
			->CreateTranslationMapPhysicalPageMapper(&fPageMapper);
		if (error != B_OK)
			return error;

		// The following code assumes that the kernel address space occupies the
		// upper half of the virtual address space. This simplifies things a
		// lot, since it allows us to just use the upper two page directories
		// of the kernel and create two new lower page directories for the
		// userland.
		STATIC_ASSERT(KERNEL_BASE == 0x80000000 && KERNEL_SIZE == 0x80000000);

		// allocate the page directories (both at once)
		pae_page_directory_entry* virtualPageDirs[4];
		phys_addr_t physicalPageDirs[4];
		virtualPageDirs[0] = (pae_page_directory_entry*)memalign(B_PAGE_SIZE,
			2 * B_PAGE_SIZE);
		if (virtualPageDirs[0] == NULL)
			return B_NO_MEMORY;
		virtualPageDirs[1] = virtualPageDirs[0] + kPAEPageTableEntryCount;

		// clear the userland page directories
		memset(virtualPageDirs[0], 0, 2 * B_PAGE_SIZE);

		// use the upper two kernel page directories
		for (int32 i = 2; i < 4; i++) {
			virtualPageDirs[i] = method->KernelVirtualPageDirs()[i];
			physicalPageDirs[i] = method->KernelPhysicalPageDirs()[i];
		}

		// look up the page directories' physical addresses
		for (int32 i = 0; i < 2; i++) {
			vm_get_page_mapping(VMAddressSpace::KernelID(),
				(addr_t)virtualPageDirs[i], &physicalPageDirs[i]);
		}

		// allocate the PDPT -- needs to have a 32 bit physical address
		phys_addr_t physicalPDPT;
		void* pdptHandle;
		pae_page_directory_pointer_table_entry* pdpt
			= (pae_page_directory_pointer_table_entry*)
				method->Allocate32BitPage(physicalPDPT, pdptHandle);
		if (pdpt == NULL) {
			free(virtualPageDirs[0]);
			return B_NO_MEMORY;
		}

		// init the PDPT entries
		for (int32 i = 0; i < 4; i++) {
			pdpt[i] = (physicalPageDirs[i] & X86_PAE_PDPTE_ADDRESS_MASK)
				| X86_PAE_PDPTE_PRESENT;
		}

		// init the paging structures
		fPagingStructures->Init(pdpt, physicalPDPT, pdptHandle, virtualPageDirs,
			physicalPageDirs);
	}

	return B_OK;
}


size_t
X86VMTranslationMapPAE::MaxPagesNeededToMap(addr_t start, addr_t end) const
{
	// If start == 0, the actual base address is not yet known to the caller and
	// we shall assume the worst case.
	if (start == 0) {
		// offset the range so it has the worst possible alignment
		start = kPAEPageTableRange - B_PAGE_SIZE;
		end += kPAEPageTableRange - B_PAGE_SIZE;
	}

	return end / kPAEPageTableRange + 1 - start / kPAEPageTableRange;
}


status_t
X86VMTranslationMapPAE::Map(addr_t virtualAddress, phys_addr_t physicalAddress,
	uint32 attributes, uint32 memoryType, vm_page_reservation* reservation)
{
	TRACE("X86VMTranslationMapPAE::Map(): %#" B_PRIxADDR " -> %#" B_PRIxPHYSADDR
		"\n", virtualAddress, physicalAddress);

	// check to see if a page table exists for this range
	pae_page_directory_entry* pageDirEntry
		= X86PagingMethodPAE::PageDirEntryForAddress(
			fPagingStructures->VirtualPageDirs(), virtualAddress);
	if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0) {
		// we need to allocate a page table
		vm_page *page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		phys_addr_t physicalPageTable
			= (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("X86VMTranslationMapPAE::Map(): asked for free page for "
			"page table: %#" B_PRIxPHYSADDR "\n", physicalPageTable);

		// put it in the page dir
		X86PagingMethodPAE::PutPageTableInPageDir(pageDirEntry,
			physicalPageTable,
			attributes
				| ((attributes & B_USER_PROTECTION) != 0
						? B_WRITE_AREA : B_KERNEL_WRITE_AREA));

		fMapCount++;
	}

	// now, fill in the page table entry
	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	pae_page_table_entry* pageTable
		= (pae_page_table_entry*)fPageMapper->GetPageTableAt(
			*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);
	pae_page_table_entry* entry = pageTable
		+ virtualAddress / B_PAGE_SIZE % kPAEPageTableEntryCount;

	ASSERT_PRINT((*entry & X86_PAE_PTE_PRESENT) == 0,
		"virtual address: %#" B_PRIxADDR ", existing pte: %#" B_PRIx64 " @ %p",
		virtualAddress, *entry, entry);

	X86PagingMethodPAE::PutPageTableEntryInTable(entry, physicalAddress,
		attributes, memoryType, fIsKernelMap);

	T(Map(this, virtualAddress, *entry));

	pinner.Unlock();

	// Note: We don't need to invalidate the TLB for this address, as previously
	// the entry was not present and the TLB doesn't cache those entries.

	fMapCount++;

	return 0;
}


status_t
X86VMTranslationMapPAE::Unmap(addr_t start, addr_t end)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("X86VMTranslationMapPAE::Unmap(): %#" B_PRIxADDR " - %#" B_PRIxADDR
		"\n", start, end);

	do {
		pae_page_directory_entry* pageDirEntry
			= X86PagingMethodPAE::PageDirEntryForAddress(
				fPagingStructures->VirtualPageDirs(), start);
		if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPAEPageTableRange);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		pae_page_table_entry* pageTable
			= (pae_page_table_entry*)fPageMapper->GetPageTableAt(
				*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);

		uint32 index = start / B_PAGE_SIZE % kPAEPageTableEntryCount;
		for (; index < kPAEPageTableEntryCount && start < end;
				index++, start += B_PAGE_SIZE) {
			if ((pageTable[index] & X86_PAE_PTE_PRESENT) == 0) {
				// page mapping not valid
				continue;
			}

			TRACE("X86VMTranslationMapPAE::Unmap(): removing page %#"
				B_PRIxADDR "\n", start);

			pae_page_table_entry oldEntry
				= X86PagingMethodPAE::ClearTableEntryFlags(
					&pageTable[index], X86_PAE_PTE_PRESENT);

			T(Unmap(this, start, oldEntry));

			fMapCount--;

			if ((oldEntry & X86_PAE_PTE_ACCESSED) != 0) {
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
X86VMTranslationMapPAE::DebugMarkRangePresent(addr_t start, addr_t end,
	bool markPresent)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	do {
		pae_page_directory_entry* pageDirEntry
			= X86PagingMethodPAE::PageDirEntryForAddress(
				fPagingStructures->VirtualPageDirs(), start);
		if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPAEPageTableRange);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		pae_page_table_entry* pageTable
			= (pae_page_table_entry*)fPageMapper->GetPageTableAt(
				*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);

		uint32 index = start / B_PAGE_SIZE % kPAEPageTableEntryCount;
		for (; index < kPAEPageTableEntryCount && start < end;
				index++, start += B_PAGE_SIZE) {

			if ((pageTable[index] & X86_PAE_PTE_PRESENT) == 0) {
				if (!markPresent)
					continue;

				X86PagingMethodPAE::SetTableEntryFlags(
					&pageTable[index], X86_PAE_PTE_PRESENT);
			} else {
				if (markPresent)
					continue;

				pae_page_table_entry oldEntry
					= X86PagingMethodPAE::ClearTableEntryFlags(
						&pageTable[index], X86_PAE_PTE_PRESENT);

				if ((oldEntry & X86_PAE_PTE_ACCESSED) != 0) {
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
X86VMTranslationMapPAE::UnmapPage(VMArea* area, addr_t address,
	bool updatePageQueue)
{
	ASSERT(address % B_PAGE_SIZE == 0);

	pae_page_directory_entry* pageDirEntry
		= X86PagingMethodPAE::PageDirEntryForAddress(
			fPagingStructures->VirtualPageDirs(), address);

	TRACE("X86VMTranslationMapPAE::UnmapPage(%#" B_PRIxADDR ")\n", address);

	RecursiveLocker locker(fLock);

	if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0)
		return B_ENTRY_NOT_FOUND;

	ThreadCPUPinner pinner(thread_get_current_thread());

	pae_page_table_entry* pageTable
		= (pae_page_table_entry*)fPageMapper->GetPageTableAt(
			*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);

	pae_page_table_entry oldEntry = X86PagingMethodPAE::ClearTableEntry(
		&pageTable[address / B_PAGE_SIZE % kPAEPageTableEntryCount]);

	T(Unmap(this, address, oldEntry));

	pinner.Unlock();

	if ((oldEntry & X86_PAE_PTE_PRESENT) == 0) {
		// page mapping not valid
		return B_ENTRY_NOT_FOUND;
	}

	fMapCount--;

	if ((oldEntry & X86_PAE_PTE_ACCESSED) != 0) {
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

	PageUnmapped(area, (oldEntry & X86_PAE_PTE_ADDRESS_MASK) / B_PAGE_SIZE,
		(oldEntry & X86_PAE_PTE_ACCESSED) != 0,
		(oldEntry & X86_PAE_PTE_DIRTY) != 0, updatePageQueue);

	return B_OK;
}


void
X86VMTranslationMapPAE::UnmapPages(VMArea* area, addr_t base, size_t size,
	bool updatePageQueue)
{
	if (size == 0)
		return;

	addr_t start = base;
	addr_t end = base + size - 1;

	TRACE("X86VMTranslationMapPAE::UnmapPages(%p, %#" B_PRIxADDR ", %#"
		B_PRIxADDR ")\n", area, start, end);

	VMAreaMappings queue;

	RecursiveLocker locker(fLock);

	do {
		pae_page_directory_entry* pageDirEntry
			= X86PagingMethodPAE::PageDirEntryForAddress(
				fPagingStructures->VirtualPageDirs(), start);
		if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPAEPageTableRange);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		pae_page_table_entry* pageTable
			= (pae_page_table_entry*)fPageMapper->GetPageTableAt(
				*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);

		uint32 index = start / B_PAGE_SIZE % kPAEPageTableEntryCount;
		for (; index < kPAEPageTableEntryCount && start < end;
				index++, start += B_PAGE_SIZE) {
			pae_page_table_entry oldEntry
				= X86PagingMethodPAE::ClearTableEntry(&pageTable[index]);
			if ((oldEntry & X86_PAE_PTE_PRESENT) == 0)
				continue;

			T(Unmap(this, start, oldEntry));

			fMapCount--;

			if ((oldEntry & X86_PAE_PTE_ACCESSED) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flags was set, since only then the entry could have
				// been in any TLB.
				InvalidatePage(start);
			}

			if (area->cache_type != CACHE_TYPE_DEVICE) {
				// get the page
				vm_page* page = vm_lookup_page(
					(oldEntry & X86_PAE_PTE_ADDRESS_MASK) / B_PAGE_SIZE);
				ASSERT(page != NULL);

				DEBUG_PAGE_ACCESS_START(page);

				// transfer the accessed/dirty flags to the page
				if ((oldEntry & X86_PAE_PTE_ACCESSED) != 0)
					page->accessed = true;
				if ((oldEntry & X86_PAE_PTE_DIRTY) != 0)
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
		vm_free_page_mapping(mapping->page->physical_page_number, mapping, freeFlags);
}


void
X86VMTranslationMapPAE::UnmapArea(VMArea* area, bool deletingAddressSpace,
	bool ignoreTopCachePageFlags)
{
	if (area->cache_type == CACHE_TYPE_DEVICE || area->wiring != B_NO_LOCK) {
		X86VMTranslationMapPAE::UnmapPages(area, area->Base(), area->Size(),
			true);
		return;
	}

	bool unmapPages = !deletingAddressSpace || !ignoreTopCachePageFlags;

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

			pae_page_directory_entry* pageDirEntry
				= X86PagingMethodPAE::PageDirEntryForAddress(
					fPagingStructures->VirtualPageDirs(), address);
			if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0) {
				panic("page %p has mapping for area %p (%#" B_PRIxADDR "), but "
					"has no page dir entry", page, area, address);
				continue;
			}

			ThreadCPUPinner pinner(thread_get_current_thread());

			pae_page_table_entry* pageTable
				= (pae_page_table_entry*)fPageMapper->GetPageTableAt(
					*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);
			pae_page_table_entry oldEntry
				= X86PagingMethodPAE::ClearTableEntry(
					&pageTable[address / B_PAGE_SIZE
						% kPAEPageTableEntryCount]);

			pinner.Unlock();

			if ((oldEntry & X86_PAE_PTE_PRESENT) == 0) {
				panic("page %p has mapping for area %p (%#" B_PRIxADDR "), but "
					"has no page table entry", page, area, address);
				continue;
			}

			T(Unmap(this, address, oldEntry));

			// transfer the accessed/dirty flags to the page and invalidate
			// the mapping, if necessary
			if ((oldEntry & X86_PAE_PTE_ACCESSED) != 0) {
				page->accessed = true;

				if (!deletingAddressSpace)
					InvalidatePage(address);
			}

			if ((oldEntry & X86_PAE_PTE_DIRTY) != 0)
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
		} else {
#if TRANSLATION_MAP_TRACING
			addr_t address = area->Base()
				+ ((page->cache_offset * B_PAGE_SIZE) - area->cache_offset);

			ThreadCPUPinner pinner(thread_get_current_thread());

			pae_page_directory_entry* pageDirEntry
				= X86PagingMethodPAE::PageDirEntryForAddress(
					fPagingStructures->VirtualPageDirs(), address);
			if ((*pageDirEntry & X86_PAE_PDE_PRESENT) != 0) {
				pae_page_table_entry* pageTable
					= (pae_page_table_entry*)fPageMapper->GetPageTableAt(
						*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);
				pae_page_table_entry oldEntry = pageTable[
					address / B_PAGE_SIZE % kPAEPageTableEntryCount];

				pinner.Unlock();

				if ((oldEntry & X86_PAE_PTE_PRESENT) != 0)
					T(Unmap(this, address, oldEntry));
			}
#endif
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
		vm_free_page_mapping(mapping->page->physical_page_number, mapping, freeFlags);
}


status_t
X86VMTranslationMapPAE::Query(addr_t virtualAddress,
	phys_addr_t* _physicalAddress, uint32* _flags)
{
	// default the flags to not present
	*_flags = 0;
	*_physicalAddress = 0;

	// get the page directory entry
	pae_page_directory_entry* pageDirEntry
		= X86PagingMethodPAE::PageDirEntryForAddress(
			fPagingStructures->VirtualPageDirs(), virtualAddress);
	if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	// get the page table entry
	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	pae_page_table_entry* pageTable
		= (pae_page_table_entry*)fPageMapper->GetPageTableAt(
			*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);
	pae_page_table_entry entry
		= pageTable[virtualAddress / B_PAGE_SIZE % kPAEPageTableEntryCount];

	pinner.Unlock();

	*_physicalAddress = entry & X86_PAE_PTE_ADDRESS_MASK;

	// translate the page state flags
	if ((entry & X86_PAE_PTE_USER) != 0) {
		*_flags |= ((entry & X86_PAE_PTE_WRITABLE) != 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA
			| ((entry & X86_PAE_PTE_NOT_EXECUTABLE) == 0 ? B_EXECUTE_AREA : 0);
	}

	*_flags |= ((entry & X86_PAE_PTE_WRITABLE) != 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & X86_PAE_PTE_NOT_EXECUTABLE) == 0
			? B_KERNEL_EXECUTE_AREA : 0)
		| ((entry & X86_PAE_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & X86_PAE_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((entry & X86_PAE_PTE_PRESENT) != 0 ? PAGE_PRESENT : 0);

	TRACE("X86VMTranslationMapPAE::Query(%#" B_PRIxADDR ") -> %#"
		B_PRIxPHYSADDR ":\n", virtualAddress, *_physicalAddress);

	return B_OK;
}


status_t
X86VMTranslationMapPAE::QueryInterrupt(addr_t virtualAddress,
	phys_addr_t* _physicalAddress, uint32* _flags)
{
	// default the flags to not present
	*_flags = 0;
	*_physicalAddress = 0;

	// get the page directory entry
	pae_page_directory_entry* pageDirEntry
		= X86PagingMethodPAE::PageDirEntryForAddress(
			fPagingStructures->VirtualPageDirs(), virtualAddress);
	if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	// get the page table entry
	pae_page_table_entry* pageTable
		= (pae_page_table_entry*)X86PagingMethodPAE::Method()
			->PhysicalPageMapper()->InterruptGetPageTableAt(
				*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);
	pae_page_table_entry entry
		= pageTable[virtualAddress / B_PAGE_SIZE % kPAEPageTableEntryCount];

	*_physicalAddress = entry & X86_PAE_PTE_ADDRESS_MASK;

	// translate the page state flags
	if ((entry & X86_PAE_PTE_USER) != 0) {
		*_flags |= ((entry & X86_PAE_PTE_WRITABLE) != 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA
			| ((entry & X86_PAE_PTE_NOT_EXECUTABLE) == 0 ? B_EXECUTE_AREA : 0);
	}

	*_flags |= ((entry & X86_PAE_PTE_WRITABLE) != 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & X86_PAE_PTE_NOT_EXECUTABLE) == 0
			? B_KERNEL_EXECUTE_AREA : 0)
		| ((entry & X86_PAE_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & X86_PAE_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((entry & X86_PAE_PTE_PRESENT) != 0 ? PAGE_PRESENT : 0);

	TRACE("X86VMTranslationMapPAE::Query(%#" B_PRIxADDR ") -> %#"
		B_PRIxPHYSADDR ":\n", virtualAddress, *_physicalAddress);

	return B_OK;
}


status_t
X86VMTranslationMapPAE::Protect(addr_t start, addr_t end, uint32 attributes,
	uint32 memoryType)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("X86VMTranslationMapPAE::Protect(): %#" B_PRIxADDR " - %#" B_PRIxADDR
		", attributes: %#" B_PRIx32 "\n", start, end, attributes);

	// compute protection/memory type flags
	uint64 newFlags
		= X86PagingMethodPAE::MemoryTypeToPageTableEntryFlags(memoryType);
	if ((attributes & B_USER_PROTECTION) != 0) {
		newFlags |= X86_PAE_PTE_USER;
		if ((attributes & B_WRITE_AREA) != 0)
			newFlags |= X86_PAE_PTE_WRITABLE;
		if ((attributes & B_EXECUTE_AREA) == 0
			&& x86_check_feature(IA32_FEATURE_AMD_EXT_NX, FEATURE_EXT_AMD)) {
			newFlags |= X86_PAE_PTE_NOT_EXECUTABLE;
		}
	} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		newFlags |= X86_PAE_PTE_WRITABLE;

	do {
		pae_page_directory_entry* pageDirEntry
			= X86PagingMethodPAE::PageDirEntryForAddress(
				fPagingStructures->VirtualPageDirs(), start);
		if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPAEPageTableRange);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		pae_page_table_entry* pageTable
			= (pae_page_table_entry*)fPageMapper->GetPageTableAt(
				*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);

		uint32 index = start / B_PAGE_SIZE % kPAEPageTableEntryCount;
		for (; index < kPAEPageTableEntryCount && start < end;
				index++, start += B_PAGE_SIZE) {
			pae_page_table_entry entry = pageTable[index];
			if ((pageTable[index] & X86_PAE_PTE_PRESENT) == 0) {
				// page mapping not valid
				continue;
			}

			TRACE("X86VMTranslationMapPAE::Protect(): protect page %#"
				B_PRIxADDR "\n", start);

			// set the new protection flags -- we want to do that atomically,
			// without changing the accessed or dirty flag
			pae_page_table_entry oldEntry;
			while (true) {
				oldEntry = X86PagingMethodPAE::TestAndSetTableEntry(
					&pageTable[index],
					(entry & ~(X86_PAE_PTE_PROTECTION_MASK
							| X86_PAE_PTE_MEMORY_TYPE_MASK))
						| newFlags,
					entry);
				if (oldEntry == entry)
					break;
				entry = oldEntry;
			}

			T(Protect(this, start, entry,
				(entry & ~(X86_PAE_PTE_PROTECTION_MASK
						| X86_PAE_PTE_MEMORY_TYPE_MASK))
					| newFlags));

			if ((oldEntry & X86_PAE_PTE_ACCESSED) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flag was set, since only then the entry could have been
				// in any TLB.
				InvalidatePage(start);
			}
		}
	} while (start != 0 && start < end);

	return B_OK;
}


status_t
X86VMTranslationMapPAE::ClearFlags(addr_t address, uint32 flags)
{
	pae_page_directory_entry* pageDirEntry
		= X86PagingMethodPAE::PageDirEntryForAddress(
			fPagingStructures->VirtualPageDirs(), address);
	if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	uint64 flagsToClear = ((flags & PAGE_MODIFIED) ? X86_PAE_PTE_DIRTY : 0)
		| ((flags & PAGE_ACCESSED) ? X86_PAE_PTE_ACCESSED : 0);

	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	pae_page_table_entry* entry
		= (pae_page_table_entry*)fPageMapper->GetPageTableAt(
				*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK)
			+ address / B_PAGE_SIZE % kPAEPageTableEntryCount;

	// clear out the flags we've been requested to clear
	pae_page_table_entry oldEntry
		= X86PagingMethodPAE::ClearTableEntryFlags(entry, flagsToClear);

	pinner.Unlock();

	T(ClearFlags(this, address, oldEntry, flagsToClear));

	if ((oldEntry & flagsToClear) != 0)
		InvalidatePage(address);

	return B_OK;
}


bool
X86VMTranslationMapPAE::ClearAccessedAndModified(VMArea* area, addr_t address,
	bool unmapIfUnaccessed, bool& _modified)
{
	ASSERT(address % B_PAGE_SIZE == 0);

	TRACE("X86VMTranslationMapPAE::ClearAccessedAndModified(%#" B_PRIxADDR
		")\n", address);

	pae_page_directory_entry* pageDirEntry
		= X86PagingMethodPAE::PageDirEntryForAddress(
			fPagingStructures->VirtualPageDirs(), address);

	RecursiveLocker locker(fLock);

	if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0)
		return false;

	ThreadCPUPinner pinner(thread_get_current_thread());

	pae_page_table_entry* entry
		= (pae_page_table_entry*)fPageMapper->GetPageTableAt(
				*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK)
			+ address / B_PAGE_SIZE % kPAEPageTableEntryCount;

	// perform the deed
	pae_page_table_entry oldEntry;

	if (unmapIfUnaccessed) {
		while (true) {
			oldEntry = *entry;
			if ((oldEntry & X86_PAE_PTE_PRESENT) == 0) {
				// page mapping not valid
				return false;
			}

			if (oldEntry & X86_PAE_PTE_ACCESSED) {
				// page was accessed -- just clear the flags
				oldEntry = X86PagingMethodPAE::ClearTableEntryFlags(entry,
					X86_PAE_PTE_ACCESSED | X86_PAE_PTE_DIRTY);
				T(ClearFlags(this, address, oldEntry,
					X86_PAE_PTE_ACCESSED | X86_PAE_PTE_DIRTY));
				break;
			}

			// page hasn't been accessed -- unmap it
			if (X86PagingMethodPAE::TestAndSetTableEntry(entry, 0, oldEntry)
					== oldEntry) {
				T(ClearFlagsUnmap(this, address, oldEntry));
				break;
			}

			// something changed -- check again
		}
	} else {
		oldEntry = X86PagingMethodPAE::ClearTableEntryFlags(entry,
			X86_PAE_PTE_ACCESSED | X86_PAE_PTE_DIRTY);
		T(ClearFlags(this, address, oldEntry,
			X86_PAE_PTE_ACCESSED | X86_PAE_PTE_DIRTY));
	}

	pinner.Unlock();

	_modified = (oldEntry & X86_PAE_PTE_DIRTY) != 0;

	if ((oldEntry & X86_PAE_PTE_ACCESSED) != 0) {
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
		(oldEntry & X86_PAE_PTE_ADDRESS_MASK) / B_PAGE_SIZE);

	return false;
}


void
X86VMTranslationMapPAE::DebugPrintMappingInfo(addr_t virtualAddress)
{
	// get the page directory
	pae_page_directory_entry* const* pdpt
		= fPagingStructures->VirtualPageDirs();
	pae_page_directory_entry* pageDirectory = pdpt[virtualAddress >> 30];
	kprintf("page directory: %p (PDPT[%zu])\n", pageDirectory,
		virtualAddress >> 30);

	// get the page directory entry
	pae_page_directory_entry* pageDirEntry
		= X86PagingMethodPAE::PageDirEntryForAddress(pdpt, virtualAddress);
	kprintf("page directory entry %zu (%p): %#" B_PRIx64 "\n",
		pageDirEntry - pageDirectory, pageDirEntry, *pageDirEntry);

	kprintf("  access: ");
	if ((*pageDirEntry & X86_PAE_PDE_PRESENT) != 0)
		kprintf(" present");
	if ((*pageDirEntry & X86_PAE_PDE_WRITABLE) != 0)
		kprintf(" writable");
	if ((*pageDirEntry & X86_PAE_PDE_USER) != 0)
		kprintf(" user");
	if ((*pageDirEntry & X86_PAE_PDE_NOT_EXECUTABLE) == 0)
		kprintf(" executable");
	if ((*pageDirEntry & X86_PAE_PDE_LARGE_PAGE) != 0)
		kprintf(" large");

	kprintf("\n  caching:");
	if ((*pageDirEntry & X86_PAE_PDE_WRITE_THROUGH) != 0)
		kprintf(" write-through");
	if ((*pageDirEntry & X86_PAE_PDE_CACHING_DISABLED) != 0)
		kprintf(" uncached");

	kprintf("\n  flags:  ");
	if ((*pageDirEntry & X86_PAE_PDE_ACCESSED) != 0)
		kprintf(" accessed");
	kprintf("\n");

	if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0)
		return;

	// get the page table entry
	pae_page_table_entry* pageTable
		= (pae_page_table_entry*)X86PagingMethodPAE::Method()
			->PhysicalPageMapper()->InterruptGetPageTableAt(
				*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);
	kprintf("page table: %#" B_PRIx64 "\n",
		*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);
	size_t pteIndex = virtualAddress / B_PAGE_SIZE % kPAEPageTableEntryCount;
	pae_page_table_entry entry = pageTable[pteIndex];
	kprintf("page table entry %zu (phys: %#" B_PRIx64 "): %#" B_PRIx64 "\n",
		pteIndex,
		(*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK)
			+ pteIndex * sizeof(pae_page_table_entry),
		entry);

	kprintf("  access: ");
	if ((entry & X86_PAE_PTE_PRESENT) != 0)
		kprintf(" present");
	if ((entry & X86_PAE_PTE_WRITABLE) != 0)
		kprintf(" writable");
	if ((entry & X86_PAE_PTE_USER) != 0)
		kprintf(" user");
	if ((entry & X86_PAE_PTE_NOT_EXECUTABLE) == 0)
		kprintf(" executable");
	if ((entry & X86_PAE_PTE_GLOBAL) == 0)
		kprintf(" global");

	kprintf("\n  caching:");
	if ((entry & X86_PAE_PTE_WRITE_THROUGH) != 0)
		kprintf(" write-through");
	if ((entry & X86_PAE_PTE_CACHING_DISABLED) != 0)
		kprintf(" uncached");
	if ((entry & X86_PAE_PTE_PAT) != 0)
		kprintf(" PAT");

	kprintf("\n  flags:  ");
	if ((entry & X86_PAE_PTE_ACCESSED) != 0)
		kprintf(" accessed");
	if ((entry & X86_PAE_PTE_DIRTY) != 0)
		kprintf(" dirty");
	kprintf("\n");

	if ((entry & X86_PAE_PTE_PRESENT) != 0) {
		kprintf("  address: %#" B_PRIx64 "\n",
			entry & X86_PAE_PTE_ADDRESS_MASK);
	}
}


bool
X86VMTranslationMapPAE::DebugGetReverseMappingInfo(phys_addr_t physicalAddress,
	ReverseMappingInfoCallback& callback)
{
	pae_page_directory_entry* const* pdpt
		= fPagingStructures->VirtualPageDirs();
	for (uint32 pageDirIndex = fIsKernelMap ? 2 : 0;
		pageDirIndex < uint32(fIsKernelMap ? 4 : 2); pageDirIndex++) {
		// iterate through the page directory
		pae_page_directory_entry* pageDirectory = pdpt[pageDirIndex];
		for (uint32 pdeIndex = 0; pdeIndex < kPAEPageDirEntryCount;
			pdeIndex++) {
			pae_page_directory_entry& pageDirEntry = pageDirectory[pdeIndex];
			if ((pageDirEntry & X86_PAE_PDE_ADDRESS_MASK) == 0)
				continue;

			// get and iterate through the page table
			pae_page_table_entry* pageTable
				= (pae_page_table_entry*)X86PagingMethodPAE::Method()
					->PhysicalPageMapper()->InterruptGetPageTableAt(
						pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);
			for (uint32 pteIndex = 0; pteIndex < kPAEPageTableEntryCount;
				pteIndex++) {
				pae_page_table_entry entry = pageTable[pteIndex];
				if ((entry & X86_PAE_PTE_PRESENT) != 0
					&& (entry & X86_PAE_PTE_ADDRESS_MASK) == physicalAddress) {
					addr_t virtualAddress = pageDirIndex * kPAEPageDirRange
						+ pdeIndex * kPAEPageTableRange
						+ pteIndex * B_PAGE_SIZE;
					if (callback.HandleVirtualAddress(virtualAddress))
						return true;
				}
			}
		}
	}

	return false;
}


X86PagingStructures*
X86VMTranslationMapPAE::PagingStructures() const
{
	return fPagingStructures;
}


#endif	// B_HAIKU_PHYSICAL_BITS == 64
