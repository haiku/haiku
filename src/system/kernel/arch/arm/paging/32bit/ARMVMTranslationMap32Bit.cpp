/*
 * Copyright 2010, Ithamar R. Adema, ithamar.adema@team-embedded.nl
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/32bit/ARMVMTranslationMap32Bit.h"

#include <stdlib.h>
#include <string.h>

#include <interrupts.h>
#include <thread.h>
#include <slab/Slab.h>
#include <smp.h>
#include <util/AutoLock.h>
#include <util/ThreadAutoLock.h>
#include <util/queue.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMCache.h>

#include "paging/32bit/ARMPagingMethod32Bit.h"
#include "paging/32bit/ARMPagingStructures32Bit.h"
#include "paging/arm_physical_page_mapper.h"


//#define TRACE_ARM_VM_TRANSLATION_MAP_32_BIT
#ifdef TRACE_ARM_VM_TRANSLATION_MAP_32_BIT
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


#define PAGEDIR_SIZE	ARM_MMU_L1_TABLE_SIZE
#define PAGEDIR_ALIGN	(4 * B_PAGE_SIZE)


ARMVMTranslationMap32Bit::ARMVMTranslationMap32Bit()
	:
	fPagingStructures(NULL)
{
}


ARMVMTranslationMap32Bit::~ARMVMTranslationMap32Bit()
{
	if (fPagingStructures == NULL)
		return;

	if (fPageMapper != NULL)
		fPageMapper->Delete();

	if (fPagingStructures->pgdir_virt != NULL) {
		// cycle through and free all of the user space pgtables
		vm_page_reservation reservation = {};
		for (uint32 i = VADDR_TO_PDENT(USER_BASE);
				i <= VADDR_TO_PDENT(USER_BASE + (USER_SIZE - 1)); i++) {
			if ((fPagingStructures->pgdir_virt[i] & ARM_PDE_TYPE_MASK) != 0) {
				addr_t address = fPagingStructures->pgdir_virt[i]
					& ARM_PDE_ADDRESS_MASK;
				vm_page* page = vm_lookup_page(address / B_PAGE_SIZE);
				if (!page)
					panic("destroy_tmap: didn't find pgtable page\n");
				DEBUG_PAGE_ACCESS_START(page);
				vm_page_free_etc(NULL, page, &reservation);
			}
		}
		vm_page_unreserve_pages(&reservation);
	}

	fPagingStructures->RemoveReference();
}


status_t
ARMVMTranslationMap32Bit::Init(bool kernel)
{
	TRACE("ARMVMTranslationMap32Bit::Init()\n");

	ARMVMTranslationMap::Init(kernel);

	fPagingStructures = new(std::nothrow) ARMPagingStructures32Bit;
	if (fPagingStructures == NULL)
		return B_NO_MEMORY;

	ARMPagingMethod32Bit* method = ARMPagingMethod32Bit::Method();

	if (!kernel) {
		// user
		// allocate a physical page mapper
		status_t error = method->PhysicalPageMapper()
			->CreateTranslationMapPhysicalPageMapper(&fPageMapper);
		if (error != B_OK)
			return error;

		// allocate the page directory
		page_directory_entry *virtualPageDir = NULL;

		virtual_address_restrictions virtualRestrictions = {};
		virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;

		physical_address_restrictions physicalRestrictions = {};
		physicalRestrictions.alignment = PAGEDIR_ALIGN;

		area_id pgdir_area = create_area_etc(B_SYSTEM_TEAM, "pgdir",
			PAGEDIR_SIZE, B_CONTIGUOUS,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0, 0,
			&virtualRestrictions, &physicalRestrictions, (void **)&virtualPageDir);

		if (pgdir_area < 0) {
			return B_NO_MEMORY;
		}

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
ARMVMTranslationMap32Bit::MaxPagesNeededToMap(addr_t start, addr_t end) const
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
ARMVMTranslationMap32Bit::Map(addr_t va, phys_addr_t pa, uint32 attributes,
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
	if ((pd[index] & ARM_PDE_TYPE_MASK) == 0) {
		phys_addr_t pgtable;
		vm_page *page;

		// we need to allocate a pgtable
		page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		pgtable = (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("map_tmap: asked for free page for pgtable. 0x%lx\n", pgtable);

		// put it in the pgdir
		ARMPagingMethod32Bit::PutPageTableInPageDir(&pd[index], pgtable,
			(va < KERNEL_BASE) ? ARM_MMU_L1_FLAG_PXN : 0);

		// update any other page directories, if it maps kernel space
		if (index >= FIRST_KERNEL_PGDIR_ENT
			&& index < (FIRST_KERNEL_PGDIR_ENT + NUM_KERNEL_PGDIR_ENTS)) {
			ARMPagingStructures32Bit::UpdateAllPageDirs(index, pd[index]);
		}

		fMapCount++;
	}

	// now, fill in the pentry
	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & ARM_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(va);

	ASSERT_PRINT((pt[index] & ARM_PTE_TYPE_MASK) == 0,
		"virtual address: %#" B_PRIxADDR ", existing pte: %#" B_PRIx32, va,
		pt[index]);

	ARMPagingMethod32Bit::PutPageTableEntryInTable(&pt[index], pa, attributes,
		memoryType, fIsKernelMap);

	pinner.Unlock();

	// Note: We don't need to invalidate the TLB for this address, as previously
	// the entry was not present and the TLB doesn't cache those entries.

	fMapCount++;

	return 0;
}


status_t
ARMVMTranslationMap32Bit::Unmap(addr_t start, addr_t end)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("unmap_tmap: asked to free pages 0x%lx to 0x%lx\n", start, end);

	page_directory_entry *pd = fPagingStructures->pgdir_virt;

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & ARM_PDE_TYPE_MASK) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & ARM_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); (index < 256) && (start < end);
				index++, start += B_PAGE_SIZE) {
			if ((pt[index] & ARM_PTE_TYPE_MASK) == 0) {
				// page mapping not valid
				continue;
			}

			TRACE("unmap_tmap: removing page 0x%lx\n", start);

			page_table_entry oldEntry
				= ARMPagingMethod32Bit::ClearPageTableEntryFlags(&pt[index],
					ARM_PTE_TYPE_MASK);
			fMapCount--;

			if ((oldEntry & ARM_MMU_L2_FLAG_AP0) != 0) {
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
ARMVMTranslationMap32Bit::DebugMarkRangePresent(addr_t start, addr_t end,
	bool markPresent)
{
#if 0
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

				if ((oldEntry & ARM_MMU_L2_FLAG_AP0) != 0) {
					// Note, that we only need to invalidate the address, if the
					// accessed flags was set, since only then the entry could
					// have been in any TLB.
					InvalidatePage(start);
				}
			}
		}
	} while (start != 0 && start < end);
#endif
	return B_OK;
}


status_t
ARMVMTranslationMap32Bit::UnmapPage(VMArea* area, addr_t address,
	bool updatePageQueue, bool deletingAddressSpace, uint32* _flags)
{
	ASSERT(address % B_PAGE_SIZE == 0);
	ASSERT(_flags == NULL || !updatePageQueue);

	TRACE("ARMVMTranslationMap32Bit::UnmapPage(%#" B_PRIxADDR ")\n", address);

	RecursiveLocker locker(fLock);

	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	int index = VADDR_TO_PDENT(address);
	if ((pd[index] & ARM_PDE_TYPE_MASK) == 0)
		return B_ENTRY_NOT_FOUND;

	ThreadCPUPinner pinner(thread_get_current_thread());

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & ARM_PDE_ADDRESS_MASK);

	index = VADDR_TO_PTENT(address);
	page_table_entry oldEntry = ARMPagingMethod32Bit::ClearPageTableEntry(
		&pt[index]);

	pinner.Unlock();

	if ((oldEntry & ARM_PTE_TYPE_MASK) == 0) {
		// page mapping not valid
		return B_ENTRY_NOT_FOUND;
	}

	fMapCount--;

	if ((oldEntry & ARM_MMU_L2_FLAG_AP0) != 0) {
		// Note, that we only need to invalidate the address, if the
		// accessed flags was set, since only then the entry could have been
		// in any TLB.
		if (!deletingAddressSpace)
			InvalidatePage(address);

		if (_flags == NULL)
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

	if (_flags == NULL) {
		locker.Detach();
			// PageUnmapped() will unlock for us

		PageUnmapped(area, (oldEntry & ARM_PTE_ADDRESS_MASK) / B_PAGE_SIZE,
			(oldEntry & ARM_MMU_L2_FLAG_AP0) != 0, (oldEntry & ARM_MMU_L2_FLAG_AP2) != 0,
			updatePageQueue);
	} else {
		uint32 flags = PAGE_PRESENT;
		if ((oldEntry & ARM_MMU_L2_FLAG_AP0) != 0)
			flags |= PAGE_ACCESSED;
		if ((oldEntry & ARM_MMU_L2_FLAG_AP2) != 0)
			flags |= PAGE_MODIFIED;
		*_flags = flags;
	}

	return B_OK;
}


void
ARMVMTranslationMap32Bit::UnmapPages(VMArea* area, addr_t base, size_t size,
	bool updatePageQueue, bool deletingAddressSpace)
{
	if (size == 0)
		return;

	addr_t start = base;
	addr_t end = base + size - 1;

	TRACE("ARMVMTranslationMap32Bit::UnmapPages(%p, %#" B_PRIxADDR ", %#"
		B_PRIxADDR ")\n", area, start, end);

	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	VMAreaMappings queue;

	RecursiveLocker locker(fLock);

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & ARM_PDE_TYPE_MASK) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & ARM_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); (index < 256) && (start < end);
				index++, start += B_PAGE_SIZE) {
			page_table_entry oldEntry
				= ARMPagingMethod32Bit::ClearPageTableEntry(&pt[index]);
			if ((oldEntry & ARM_PTE_TYPE_MASK) == 0)
				continue;

			fMapCount--;

			if ((oldEntry & ARM_MMU_L2_FLAG_AP0) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flags was set, since only then the entry could have
				// been in any TLB.
				if (!deletingAddressSpace)
					InvalidatePage(start);
			}

			if (area->cache_type != CACHE_TYPE_DEVICE) {
				page_num_t page = (oldEntry & ARM_PTE_ADDRESS_MASK) / B_PAGE_SIZE;
				PageUnmapped(area, page,
					(oldEntry & ARM_MMU_L2_FLAG_AP0) != 0,
					(oldEntry & ARM_MMU_L2_FLAG_AP2) != 0,
					updatePageQueue, &queue);
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


status_t
ARMVMTranslationMap32Bit::Query(addr_t va, phys_addr_t *_physical,
	uint32 *_flags)
{
	// default the flags to not present
	*_flags = 0;
	*_physical = 0;

	int index = VADDR_TO_PDENT(va);
	page_directory_entry *pd = fPagingStructures->pgdir_virt;
	if ((pd[index] & ARM_PDE_TYPE_MASK) == 0) {
		// no pagetable here
		return B_OK;
	}

	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & ARM_PDE_ADDRESS_MASK);
	page_table_entry entry = pt[VADDR_TO_PTENT(va)];

	if ((entry & ARM_PTE_TYPE_MASK) != 0)
		*_physical = (entry & ARM_PTE_ADDRESS_MASK);

	*_flags = ARMPagingMethod32Bit::PageTableEntryFlagsToAttributes(entry);
	if (*_physical != 0)
		*_flags |= PAGE_PRESENT;

	pinner.Unlock();

	TRACE("query_tmap: returning pa 0x%lx for va 0x%lx\n", *_physical, va);

	return B_OK;
}


status_t
ARMVMTranslationMap32Bit::QueryInterrupt(addr_t va, phys_addr_t *_physical,
	uint32 *_flags)
{
	*_flags = 0;
	*_physical = 0;

	int index = VADDR_TO_PDENT(va);
	page_directory_entry* pd = fPagingStructures->pgdir_virt;
	if ((pd[index] & ARM_PDE_TYPE_MASK) == 0) {
		// no pagetable here
		return B_OK;
	}

	// map page table entry
	page_table_entry* pt = (page_table_entry*)ARMPagingMethod32Bit::Method()
		->PhysicalPageMapper()->InterruptGetPageTableAt(
			pd[index] & ARM_PDE_ADDRESS_MASK);
	page_table_entry entry = pt[VADDR_TO_PTENT(va)];

	if ((entry & ARM_PTE_TYPE_MASK) != 0)
		*_physical = (entry & ARM_PTE_ADDRESS_MASK);

	*_flags = ARMPagingMethod32Bit::PageTableEntryFlagsToAttributes(entry);
	if (*_physical != 0)
		*_flags |= PAGE_PRESENT;

	return B_OK;
}


status_t
ARMVMTranslationMap32Bit::Protect(addr_t start, addr_t end, uint32 attributes,
	uint32 memoryType)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("protect_tmap: pages 0x%lx to 0x%lx, attributes %lx\n", start, end,
		attributes);

	uint32 newProtectionFlags = ARMPagingMethod32Bit::AttributesToPageTableEntryFlags(attributes);
	uint32 newMemoryTypeFlags = ARMPagingMethod32Bit::MemoryTypeToPageTableEntryFlags(memoryType);

	page_directory_entry *pd = fPagingStructures->pgdir_virt;

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & ARM_PDE_TYPE_MASK) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & ARM_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); index < 256 && start < end;
				index++, start += B_PAGE_SIZE) {
			page_table_entry entry = pt[index];
			if ((entry & ARM_PTE_TYPE_MASK) == 0) {
				// page mapping not valid
				continue;
			}

			TRACE("protect_tmap: protect page 0x%lx\n", start);

			// set the new protection flags -- we want to do that atomically,
			// without changing the accessed or dirty flag
			page_table_entry oldEntry;
			while (true) {
				oldEntry = ARMPagingMethod32Bit::TestAndSetPageTableEntry(
					&pt[index],
					(entry & ~(ARM_PTE_PROTECTION_MASK
							| ARM_PTE_MEMORY_TYPE_MASK))
						| newProtectionFlags | newMemoryTypeFlags,
					entry);
				if (oldEntry == entry)
					break;
				entry = oldEntry;
			}

			if ((oldEntry & ARM_MMU_L2_FLAG_AP0) != 0) {
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
ARMVMTranslationMap32Bit::SetFlags(addr_t virtualAddress, uint32 flags)
{
	int index = VADDR_TO_PDENT(virtualAddress);
	page_directory_entry* pd = fPagingStructures->pgdir_virt;
	if ((pd[index] & ARM_PDE_TYPE_MASK) == 0) {
		// no pagetable here
		return B_OK;
	}

	uint32 flagsToSet = (flags & PAGE_ACCESSED) ? ARM_MMU_L2_FLAG_AP0 : 0;
	uint32 flagsToClear = (flags & PAGE_MODIFIED) ? ARM_MMU_L2_FLAG_AP2 : 0;

	page_table_entry* pt = (page_table_entry*)ARMPagingMethod32Bit::Method()
		->PhysicalPageMapper()->InterruptGetPageTableAt(
			pd[index] & ARM_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(virtualAddress);

	ARMPagingMethod32Bit::SetAndClearPageTableEntryFlags(&pt[index], flagsToSet, flagsToClear);

	// normally we would call InvalidatePage() here and then Flush() later when all updates are done
	// however, as this scenario happens only in case of Modified flag handling,
	// we can directly call TLBIMVAIS from here as we need to update only a single TLB entry
	if (flagsToClear)
		arch_cpu_invalidate_TLB_page(virtualAddress);

	return B_OK;
}


status_t
ARMVMTranslationMap32Bit::ClearFlags(addr_t va, uint32 flags)
{
	int index = VADDR_TO_PDENT(va);
	page_directory_entry* pd = fPagingStructures->pgdir_virt;
	if ((pd[index] & ARM_PDE_TYPE_MASK) == 0) {
		// no pagetable here
		return B_OK;
	}

	uint32 flagsToClear = (flags & PAGE_ACCESSED) ? ARM_MMU_L2_FLAG_AP0 : 0;
	uint32 flagsToSet = (flags & PAGE_MODIFIED) ? ARM_MMU_L2_FLAG_AP2 : 0;

	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & ARM_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(va);

	// adjust the flags we've been requested to set/clear
	page_table_entry oldEntry
		= ARMPagingMethod32Bit::SetAndClearPageTableEntryFlags(&pt[index],
			flagsToSet, flagsToClear);

	pinner.Unlock();

	if (((oldEntry & flagsToClear) != 0) || ((oldEntry & flagsToSet) == 0))
		InvalidatePage(va);

	return B_OK;
}


bool
ARMVMTranslationMap32Bit::ClearAccessedAndModified(VMArea* area, addr_t address,
	bool unmapIfUnaccessed, bool& _modified)
{
	ASSERT(address % B_PAGE_SIZE == 0);

	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	TRACE("ARMVMTranslationMap32Bit::ClearAccessedAndModified(%#" B_PRIxADDR
		")\n", address);

	RecursiveLocker locker(fLock);

	int index = VADDR_TO_PDENT(address);
	if ((pd[index] & ARM_PDE_TYPE_MASK) == 0)
		return false;

	ThreadCPUPinner pinner(thread_get_current_thread());

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & ARM_PDE_ADDRESS_MASK);

	index = VADDR_TO_PTENT(address);

	// perform the deed
	page_table_entry oldEntry;

	if (unmapIfUnaccessed) {
		while (true) {
			oldEntry = pt[index];
			if ((oldEntry & ARM_PTE_TYPE_MASK) == 0) {
				// page mapping not valid
				return false;
			}
			if (oldEntry & ARM_MMU_L2_FLAG_AP0) {
				// page was accessed -- just clear the flags
				oldEntry = ARMPagingMethod32Bit::SetAndClearPageTableEntryFlags(
					&pt[index], ARM_MMU_L2_FLAG_AP2, ARM_MMU_L2_FLAG_AP0);
				break;
			}

			// page hasn't been accessed -- unmap it
			if (ARMPagingMethod32Bit::TestAndSetPageTableEntry(&pt[index], 0,
					oldEntry) == oldEntry) {
				break;
			}

			// something changed -- check again
		}
	} else {
		oldEntry = ARMPagingMethod32Bit::SetAndClearPageTableEntryFlags(&pt[index],
			ARM_MMU_L2_FLAG_AP2, ARM_MMU_L2_FLAG_AP0);
	}

	pinner.Unlock();

	_modified = (oldEntry & ARM_MMU_L2_FLAG_AP2) == 0;

	if ((oldEntry & ARM_MMU_L2_FLAG_AP0) != 0) {
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
		(oldEntry & ARM_PTE_ADDRESS_MASK) / B_PAGE_SIZE);

	return false;
}


ARMPagingStructures*
ARMVMTranslationMap32Bit::PagingStructures() const
{
	return fPagingStructures;
}
