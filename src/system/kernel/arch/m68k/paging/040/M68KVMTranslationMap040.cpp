/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/040/M68KVMTranslationMap040.h"

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

#include "paging/040/M68KPagingMethod040.h"
#include "paging/040/M68KPagingStructures040.h"
#include "paging/m68k_physical_page_mapper.h"


#define TRACE_M68K_VM_TRANSLATION_MAP_040
#ifdef TRACE_M68K_VM_TRANSLATION_MAP_040
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


M68KVMTranslationMap040::M68KVMTranslationMap040()
	:
	fPagingStructures(NULL)
{
}


M68KVMTranslationMap040::~M68KVMTranslationMap040()
{
	if (fPagingStructures == NULL)
		return;

	if (fPageMapper != NULL)
		fPageMapper->Delete();

	if (fPagingStructures->pgroot_virt != NULL) {
		vm_page_reservation reservation = {};
		page_root_entry *pgroot_virt = fPagingStructures->pgroot_virt;

		// cycle through and free all of the user space pgdirs & pgtables
		// since the size of tables don't match B_PAGE_SIZE,
		// we alloc several at once, based on modulos,
		// we make sure they are either all in the tree or none.
		for (uint32 i = VADDR_TO_PRENT(USER_BASE);
				i <= VADDR_TO_PRENT(USER_BASE + (USER_SIZE - 1)); i++) {
			addr_t pgdir_pn;
			page_directory_entry *pgdir;
			vm_page *dirpage;

			if (PRE_TYPE(pgroot_virt[i]) == DT_INVALID)
				continue;
			if (PRE_TYPE(pgroot_virt[i]) != DT_ROOT) {
				panic("rtdir[%ld]: buggy descriptor type", i);
				return;
			}
			// XXX:suboptimal (done 8 times)
			pgdir_pn = PRE_TO_PN(pgroot_virt[i]);
			dirpage = vm_lookup_page(pgdir_pn);
			pgdir = &(((page_directory_entry *)dirpage)[i%NUM_DIRTBL_PER_PAGE]);

			for (uint32 j = 0; j <= NUM_DIRENT_PER_TBL;
					j+=NUM_PAGETBL_PER_PAGE) {
				addr_t pgtbl_pn;
				page_table_entry *pgtbl;
				vm_page *page;
				if (PDE_TYPE(pgdir[j]) == DT_INVALID)
					continue;
				if (PDE_TYPE(pgdir[j]) != DT_DIR) {
					panic("pgroot[%ld][%ld]: buggy descriptor type", i, j);
					return;
				}
				pgtbl_pn = PDE_TO_PN(pgdir[j]);
				page = vm_lookup_page(pgtbl_pn);
				pgtbl = (page_table_entry *)page;

				if (!page) {
					panic("destroy_tmap: didn't find pgtable page\n");
					return;
				}
				DEBUG_PAGE_ACCESS_START(page);
				vm_page_free_etc(NULL, page, &reservation);
			}
			if (((i + 1) % NUM_DIRTBL_PER_PAGE) == 0) {
				DEBUG_PAGE_ACCESS_END(dirpage);
				vm_page_free_etc(NULL, dirpage, &reservation);
			}
		}
		vm_page_unreserve_pages(&reservation);


#if 0
//X86
		for (uint32 i = VADDR_TO_PDENT(USER_BASE);
				i <= VADDR_TO_PDENT(USER_BASE + (USER_SIZE - 1)); i++) {
			if ((fPagingStructures->pgdir_virt[i] & M68K_PDE_PRESENT) != 0) {
				addr_t address = fPagingStructures->pgdir_virt[i]
					& M68K_PDE_ADDRESS_MASK;
				vm_page* page = vm_lookup_page(address / B_PAGE_SIZE);
				if (!page)
					panic("destroy_tmap: didn't find pgtable page\n");
				DEBUG_PAGE_ACCESS_START(page);
				vm_page_free_etc(NULL, page, &reservation);
			}
		}
#endif
	}

	fPagingStructures->RemoveReference();
}


status_t
M68KVMTranslationMap040::Init(bool kernel)
{
	TRACE("M68KVMTranslationMap040::Init()\n");

	M68KVMTranslationMap::Init(kernel);

	fPagingStructures = new(std::nothrow) M68KPagingStructures040;
	if (fPagingStructures == NULL)
		return B_NO_MEMORY;

	M68KPagingMethod040* method = M68KPagingMethod040::Method();

	if (!kernel) {
		// user
		// allocate a physical page mapper
		status_t error = method->PhysicalPageMapper()
			->CreateTranslationMapPhysicalPageMapper(&fPageMapper);
		if (error != B_OK)
			return error;

		// allocate the page root
		page_root_entry* virtualPageRoot = (page_root_entry*)memalign(
			SIZ_ROOTTBL, SIZ_ROOTTBL);
		if (virtualPageRoot == NULL)
			return B_NO_MEMORY;

		// look up the page directory's physical address
		phys_addr_t physicalPageRoot;
		vm_get_page_mapping(VMAddressSpace::KernelID(),
			(addr_t)virtualPageRoot, &physicalPageRoot);

		fPagingStructures->Init(virtualPageRoot, physicalPageRoot,
			method->KernelVirtualPageRoot());
	} else {
		// kernel
		// get the physical page mapper
		fPageMapper = method->KernelPhysicalPageMapper();

		// we already know the kernel pgdir mapping
		fPagingStructures->Init(method->KernelVirtualPageRoot(),
			method->KernelPhysicalPageRoot(), NULL);
	}

	return B_OK;
}


size_t
M68KVMTranslationMap040::MaxPagesNeededToMap(addr_t start, addr_t end) const
{
	size_t need;
	size_t pgdirs;

	// If start == 0, the actual base address is not yet known to the caller and
	// we shall assume the worst case.
	if (start == 0) {
		// offset the range so it has the worst possible alignment
#warning M68K: FIXME?
		start = 1023 * B_PAGE_SIZE;
		end += 1023 * B_PAGE_SIZE;
	}

	pgdirs = VADDR_TO_PRENT(end) + 1 - VADDR_TO_PRENT(start);
	// how much for page directories
	need = (pgdirs + NUM_DIRTBL_PER_PAGE - 1) / NUM_DIRTBL_PER_PAGE;
	// and page tables themselves
	need = ((pgdirs * NUM_DIRENT_PER_TBL) + NUM_PAGETBL_PER_PAGE - 1) / NUM_PAGETBL_PER_PAGE;

	// better rounding when only 1 pgdir
	// XXX: do better for other cases
	if (pgdirs == 1) {
		need = 1;
		need += (VADDR_TO_PDENT(end) + 1 - VADDR_TO_PDENT(start) + NUM_PAGETBL_PER_PAGE - 1) / NUM_PAGETBL_PER_PAGE;
	}

	return need;
}


status_t
M68KVMTranslationMap040::Map(addr_t va, phys_addr_t pa, uint32 attributes,
	uint32 memoryType, vm_page_reservation* reservation)
{
	TRACE("M68KVMTranslationMap040::Map: entry pa 0x%lx va 0x%lx\n", pa, va);

/*
	dprintf("pgdir at 0x%x\n", pgdir);
	dprintf("index is %d\n", va / B_PAGE_SIZE / 1024);
	dprintf("final at 0x%x\n", &pgdir[va / B_PAGE_SIZE / 1024]);
	dprintf("value is 0x%x\n", *(int *)&pgdir[va / B_PAGE_SIZE / 1024]);
	dprintf("present bit is %d\n", pgdir[va / B_PAGE_SIZE / 1024].present);
	dprintf("addr is %d\n", pgdir[va / B_PAGE_SIZE / 1024].addr);
*/
	page_root_entry *pr = fPagingStructures->pgroot_virt;
	page_directory_entry *pd;
	page_table_entry *pt;
	addr_t pd_pg, pt_pg;
	uint32 rindex, dindex, pindex;


	// check to see if a page directory exists for this range
	rindex = VADDR_TO_PRENT(va);
	if (PRE_TYPE(pr[rindex]) != DT_ROOT) {
		phys_addr_t pgdir;
		vm_page *page;
		uint32 i;

		// we need to allocate a pgdir group
		page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		pgdir = (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("::Map: asked for free page for pgdir. 0x%lx\n", pgdir);

		// for each pgdir on the allocated page:
		for (i = 0; i < NUM_DIRTBL_PER_PAGE; i++) {
			uint32 aindex = rindex & ~(NUM_DIRTBL_PER_PAGE-1); /* aligned */
			page_root_entry *apr = &pr[aindex + i];

			// put in the pgroot
			M68KPagingMethod040::PutPageDirInPageRoot(apr, pgdir, attributes
				| ((attributes & B_USER_PROTECTION) != 0
						? B_WRITE_AREA : B_KERNEL_WRITE_AREA));

			// update any other page roots, if it maps kernel space
			//XXX: suboptimal, should batch them
			if ((aindex+i) >= FIRST_KERNEL_PGDIR_ENT && (aindex+i)
					< (FIRST_KERNEL_PGDIR_ENT + NUM_KERNEL_PGDIR_ENTS))
				M68KPagingStructures040::UpdateAllPageDirs((aindex+i),
					pr[aindex+i]);

			pgdir += SIZ_DIRTBL;
		}
		fMapCount++;
	}
	// now, fill in the pentry
	//XXX: is this required?
	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	pd = (page_directory_entry*)MapperGetPageTableAt(
		PRE_TO_PA(pr[rindex]));

	//pinner.Unlock();

	// we want the table at rindex, not at rindex%(tbl/page)
	//pd += (rindex % NUM_DIRTBL_PER_PAGE) * NUM_DIRENT_PER_TBL;

	// check to see if a page table exists for this range
	dindex = VADDR_TO_PDENT(va);
	if (PDE_TYPE(pd[dindex]) != DT_DIR) {
		phys_addr_t pgtable;
		vm_page *page;
		uint32 i;

		// we need to allocate a pgtable group
		page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		pgtable = (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("::Map: asked for free page for pgtable. 0x%lx\n", pgtable);

		// for each pgtable on the allocated page:
		for (i = 0; i < NUM_PAGETBL_PER_PAGE; i++) {
			uint32 aindex = dindex & ~(NUM_PAGETBL_PER_PAGE-1); /* aligned */
			page_directory_entry *apd = &pd[aindex + i];

			// put in the pgdir
			M68KPagingMethod040::PutPageTableInPageDir(apd, pgtable, attributes
				| ((attributes & B_USER_PROTECTION) != 0
						? B_WRITE_AREA : B_KERNEL_WRITE_AREA));

			// no need to update other page directories for kernel space;
			// the root-level already point to us.

			pgtable += SIZ_PAGETBL;
		}

#warning M68K: really mean map_count++ ??
		fMapCount++;
	}

	// now, fill in the pentry
	//ThreadCPUPinner pinner(thread);

	pt = (page_table_entry*)MapperGetPageTableAt(PDE_TO_PA(pd[dindex]));
	// we want the table at rindex, not at rindex%(tbl/page)
	//pt += (dindex % NUM_PAGETBL_PER_PAGE) * NUM_PAGEENT_PER_TBL;

	pindex = VADDR_TO_PTENT(va);

	ASSERT_PRINT((PTE_TYPE(pt[pindex]) != DT_INVALID) == 0,
		"virtual address: %#" B_PRIxADDR ", existing pte: %#" B_PRIx32, va,
		pt[pindex]);

	M68KPagingMethod040::PutPageTableEntryInTable(&pt[pindex], pa, attributes,
		memoryType, fIsKernelMap);

	pinner.Unlock();

	// Note: We don't need to invalidate the TLB for this address, as previously
	// the entry was not present and the TLB doesn't cache those entries.

	fMapCount++;

	return B_OK;
}


status_t
M68KVMTranslationMap040::Unmap(addr_t start, addr_t end)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("M68KVMTranslationMap040::Unmap: asked to free pages 0x%lx to 0x%lx\n", start, end);

	page_root_entry *pr = fPagingStructures->pgroot_virt;
	page_directory_entry *pd;
	page_table_entry *pt;
	int index;

	do {
		index = VADDR_TO_PRENT(start);
		if (PRE_TYPE(pr[index]) != DT_ROOT) {
			// no pagedir here, move the start up to access the next page
			// dir group
			start = ROUNDUP(start + 1, kPageDirAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		pd = (page_directory_entry*)MapperGetPageTableAt(
			PRE_TO_PA(pr[index]));
		// we want the table at rindex, not at rindex%(tbl/page)
		//pd += (index % NUM_DIRTBL_PER_PAGE) * NUM_DIRENT_PER_TBL;


		index = VADDR_TO_PDENT(start);
		if (PDE_TYPE(pd[index]) != DT_DIR) {
			// no pagedir here, move the start up to access the next page
			// table group
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		pt = (page_table_entry*)MapperGetPageTableAt(
			PDE_TO_PA(pd[index]));
		// we want the table at rindex, not at rindex%(tbl/page)
		//pt += (index % NUM_PAGETBL_PER_PAGE) * NUM_PAGEENT_PER_TBL;

		for (index = VADDR_TO_PTENT(start);
				(index < NUM_PAGEENT_PER_TBL) && (start < end);
				index++, start += B_PAGE_SIZE) {
			if (PTE_TYPE(pt[index]) != DT_PAGE
				&& PTE_TYPE(pt[index]) != DT_INDIRECT) {
				// page mapping not valid
				continue;
			}

			TRACE("::Unmap: removing page 0x%lx\n", start);

			page_table_entry oldEntry
				= M68KPagingMethod040::ClearPageTableEntry(&pt[index]);
			fMapCount--;

			if ((oldEntry & M68K_PTE_ACCESSED) != 0) {
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
M68KVMTranslationMap040::UnmapPage(VMArea* area, addr_t address,
	bool updatePageQueue, bool deletingAddressSpace, uint32* _flags)
{
	ASSERT(address % B_PAGE_SIZE == 0);
	ASSERT(_flags == NULL || !updatePageQueue);

	page_root_entry* pr = fPagingStructures->pgroot_virt;

	TRACE("M68KVMTranslationMap040::UnmapPage(%#" B_PRIxADDR ")\n", address);

	RecursiveLocker locker(fLock);

	int index;

	index = VADDR_TO_PRENT(address);
	if (PRE_TYPE(pr[index]) == DT_ROOT)
		return B_ENTRY_NOT_FOUND;

	ThreadCPUPinner pinner(thread_get_current_thread());

	page_table_entry* pd = (page_table_entry*)MapperGetPageTableAt(
		pr[index] & M68K_PRE_ADDRESS_MASK);

	index = VADDR_TO_PDENT(address);
	if (PDE_TYPE(pd[index]) == DT_DIR)
		return B_ENTRY_NOT_FOUND;

	page_table_entry* pt = (page_table_entry*)MapperGetPageTableAt(
		pd[index] & M68K_PDE_ADDRESS_MASK);

	index = VADDR_TO_PTENT(address);
	if (PTE_TYPE(pt[index]) == DT_INDIRECT) {
		phys_addr_t indirectAddress = PIE_TO_TA(pt[index]);
		pt = (page_table_entry*)MapperGetPageTableAt(
			PIE_TO_TA(pt[index]), true);
		index = 0; // single descriptor
	}

	page_table_entry oldEntry = M68KPagingMethod040::ClearPageTableEntry(
		&pt[index]);

	pinner.Unlock();

	if (PTE_TYPE(oldEntry) != DT_PAGE) {
		// page mapping not valid
		return B_ENTRY_NOT_FOUND;
	}

	fMapCount--;

	if ((oldEntry & M68K_PTE_ACCESSED) != 0) {
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

		PageUnmapped(area, (oldEntry & M68K_PTE_ADDRESS_MASK) / B_PAGE_SIZE,
			(oldEntry & M68K_PTE_ACCESSED) != 0, (oldEntry & M68K_PTE_DIRTY) != 0,
			updatePageQueue);
	} else {
		uint32 flags = PAGE_PRESENT;
		if ((oldEntry & M68K_PTE_ACCESSED) != 0)
			flags |= PAGE_ACCESSED;
		if ((oldEntry & M68K_PTE_DIRTY) != 0)
			flags |= PAGE_MODIFIED;
		*_flags = flags;
	}

	return B_OK;
}


void
M68KVMTranslationMap040::UnmapPages(VMArea* area, addr_t base, size_t size,
	bool updatePageQueue, bool deletingAddressSpace)
{
	int index;

	if (size == 0)
		return;

	addr_t start = base;
	addr_t end = base + size - 1;

	TRACE("M68KVMTranslationMap040::UnmapPages(%p, %#" B_PRIxADDR ", %#"
		B_PRIxADDR ")\n", area, start, end);

	page_root_entry* pr = fPagingStructures->pgroot_virt;

	VMAreaMappings queue;

	RecursiveLocker locker(fLock);

	do {
		index = VADDR_TO_PRENT(start);
		if (PRE_TYPE(pr[index]) != DT_ROOT) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageDirAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pd = (page_directory_entry*)MapperGetPageTableAt(
			pr[index] & M68K_PRE_ADDRESS_MASK);

		index = VADDR_TO_PDENT(start);
		if (PDE_TYPE(pd[index]) != DT_DIR) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		page_table_entry* pt = (page_table_entry*)MapperGetPageTableAt(
			pd[index] & M68K_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); (index < 1024) && (start < end);
				index++, start += B_PAGE_SIZE) {
			page_table_entry *e = &pt[index];
			// fetch indirect descriptor
			//XXX:clear the indirect descriptor too??
			if (PTE_TYPE(pt[index]) == DT_INDIRECT) {
				phys_addr_t indirectAddress = PIE_TO_TA(pt[index]);
				e = (page_table_entry*)MapperGetPageTableAt(
					PIE_TO_TA(pt[index]));
			}

			page_table_entry oldEntry
				= M68KPagingMethod040::ClearPageTableEntry(e);
			if (PTE_TYPE(oldEntry) != DT_PAGE)
				continue;

			fMapCount--;

			if ((oldEntry & M68K_PTE_ACCESSED) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flags was set, since only then the entry could have
				// been in any TLB.
				if (!deletingAddressSpace)
					InvalidatePage(start);
			}

			if (area->cache_type != CACHE_TYPE_DEVICE) {
				page_num_t page = (oldEntry & M68K_PTE_ADDRESS_MASK) / B_PAGE_SIZE;
				PageUnmapped(area, page,
					(oldEntry & M68K_PTE_ACCESSED) != 0,
					(oldEntry & M68K_PTE_DIRTY) != 0,
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
M68KVMTranslationMap040::Query(addr_t va, phys_addr_t *_physical,
	uint32 *_flags)
{
	// default the flags to not present
	*_flags = 0;
	*_physical = 0;
	TRACE("040::Query(0x%lx,)\n", va);

	int index = VADDR_TO_PRENT(va);
	page_root_entry *pr = fPagingStructures->pgroot_virt;
	if (PRE_TYPE(pr[index]) != DT_ROOT) {
		// no pagetable here
		return B_OK;
	}

	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_directory_entry* pd = (page_directory_entry*)MapperGetPageTableAt(
		pr[index] & M68K_PDE_ADDRESS_MASK);

	index = VADDR_TO_PDENT(va);
	if (PDE_TYPE(pd[index]) != DT_DIR) {
		// no pagetable here
		return B_OK;
	}
	
	page_table_entry* pt = (page_table_entry*)MapperGetPageTableAt(
		pd[index] & M68K_PDE_ADDRESS_MASK);

	index = VADDR_TO_PTENT(va);
	if (PTE_TYPE(pt[index]) == DT_INDIRECT) {
		pt = (page_table_entry*)MapperGetPageTableAt(
			pt[index] & M68K_PIE_ADDRESS_MASK);
		index = 0;
	}

	page_table_entry entry = pt[index];

	*_physical = entry & M68K_PTE_ADDRESS_MASK;

	// read in the page state flags
	if ((entry & M68K_PTE_SUPERVISOR) == 0) {
		*_flags |= ((entry & M68K_PTE_READONLY) == 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA;
	}

	*_flags |= ((entry & M68K_PTE_READONLY) == 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & M68K_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & M68K_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((PTE_TYPE(entry) == DT_PAGE) ? PAGE_PRESENT : 0);

	pinner.Unlock();

	TRACE("query_tmap: returning pa 0x%lx for va 0x%lx\n", *_physical, va);

	return B_OK;
}


status_t
M68KVMTranslationMap040::QueryInterrupt(addr_t va, phys_addr_t *_physical,
	uint32 *_flags)
{
	*_flags = 0;
	*_physical = 0;
	TRACE("040::QueryInterrupt(0x%lx,)\n", va);

	int index = VADDR_TO_PRENT(va);
	page_root_entry* pr = fPagingStructures->pgroot_virt;
	if (PRE_TYPE(pr[index]) != DT_ROOT) {
		// no pagetable here
		return B_OK;
	}

	// map page table entry
	phys_addr_t ppr = pr[index] & M68K_PRE_ADDRESS_MASK;
	page_directory_entry* pd = (page_directory_entry*)((char *)
		M68KPagingMethod040::Method()->PhysicalPageMapper()
		->InterruptGetPageTableAt(ppr & ~(B_PAGE_SIZE-1))
		+ (ppr % B_PAGE_SIZE));

	index = VADDR_TO_PDENT(va);
	if (PDE_TYPE(pd[index]) != DT_DIR) {
		// no pagetable here
		return B_OK;
	}

	phys_addr_t ppd = pd[index] & M68K_PDE_ADDRESS_MASK;
	page_table_entry* pt = (page_table_entry*)((char *)
		M68KPagingMethod040::Method()->PhysicalPageMapper()
		->InterruptGetPageTableAt(ppd & ~(B_PAGE_SIZE-1))
		+ (ppd % B_PAGE_SIZE));

	index = VADDR_TO_PTENT(va);
	if (PTE_TYPE(pt[index]) == DT_INDIRECT) {
		phys_addr_t ppt = pt[index] & M68K_PIE_ADDRESS_MASK;
		pt = (page_table_entry*)((char *)
			M68KPagingMethod040::Method()->PhysicalPageMapper()
			->InterruptGetPageTableAt(ppt & ~(B_PAGE_SIZE-1))
			+ (ppt % B_PAGE_SIZE));
		index = 0;
	}

	page_table_entry entry = pt[index];

	*_physical = entry & M68K_PTE_ADDRESS_MASK;

	// read in the page state flags
	if ((entry & M68K_PTE_SUPERVISOR) == 0) {
		*_flags |= ((entry & M68K_PTE_READONLY) == 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA;
	}

	*_flags |= ((entry & M68K_PTE_READONLY) == 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & M68K_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & M68K_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((PTE_TYPE(entry) == DT_PAGE) ? PAGE_PRESENT : 0);

	return B_OK;
}


status_t
M68KVMTranslationMap040::Protect(addr_t start, addr_t end, uint32 attributes,
	uint32 memoryType)
{
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("protect_tmap: pages 0x%lx to 0x%lx, attributes %lx\n", start, end,
		attributes);

	return ENOSYS;
#if 0
	// compute protection flags
	uint32 newProtectionFlags = 0;
	if ((attributes & B_USER_PROTECTION) != 0) {
		newProtectionFlags = M68K_PTE_USER;
		if ((attributes & B_WRITE_AREA) != 0)
			newProtectionFlags |= M68K_PTE_WRITABLE;
	} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		newProtectionFlags = M68K_PTE_WRITABLE;

	page_directory_entry *pd = fPagingStructures->pgdir_virt;

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & M68K_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		struct thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)MapperGetPageTableAt(
			pd[index] & M68K_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); index < 1024 && start < end;
				index++, start += B_PAGE_SIZE) {
			page_table_entry entry = pt[index];
			if ((entry & M68K_PTE_PRESENT) == 0) {
				// page mapping not valid
				continue;
			}

			TRACE("protect_tmap: protect page 0x%lx\n", start);

			// set the new protection flags -- we want to do that atomically,
			// without changing the accessed or dirty flag
			page_table_entry oldEntry;
			while (true) {
				oldEntry = M68KPagingMethod040::TestAndSetPageTableEntry(
					&pt[index],
					(entry & ~(M68K_PTE_PROTECTION_MASK
							| M68K_PTE_MEMORY_TYPE_MASK))
						| newProtectionFlags
						| M68KPagingMethod040::MemoryTypeToPageTableEntryFlags(
							memoryType),
					entry);
				if (oldEntry == entry)
					break;
				entry = oldEntry;
			}

			if ((oldEntry & M68K_PTE_ACCESSED) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flag was set, since only then the entry could have
				// been in any TLB.
				InvalidatePage(start);
			}
		}
	} while (start != 0 && start < end);
	return B_OK;
#endif
}


status_t
M68KVMTranslationMap040::ClearFlags(addr_t va, uint32 flags)
{
	return ENOSYS;
#if 0
	int index = VADDR_TO_PDENT(va);
	page_directory_entry* pd = fPagingStructures->pgdir_virt;
	if ((pd[index] & M68K_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	uint32 flagsToClear = ((flags & PAGE_MODIFIED) ? M68K_PTE_DIRTY : 0)
		| ((flags & PAGE_ACCESSED) ? M68K_PTE_ACCESSED : 0);

	struct thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)MapperGetPageTableAt(
		pd[index] & M68K_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(va);

	// clear out the flags we've been requested to clear
	page_table_entry oldEntry
		= M68KPagingMethod040::ClearPageTableEntryFlags(&pt[index],
			flagsToClear);

	pinner.Unlock();

	if ((oldEntry & flagsToClear) != 0)
		InvalidatePage(va);

	return B_OK;
#endif
}


bool
M68KVMTranslationMap040::ClearAccessedAndModified(VMArea* area, addr_t address,
	bool unmapIfUnaccessed, bool& _modified)
{
	ASSERT(address % B_PAGE_SIZE == 0);

	page_root_entry* pr = fPagingStructures->pgroot_virt;

	TRACE("M68KVMTranslationMap040::ClearAccessedAndModified(%#" B_PRIxADDR
		")\n", address);

#if 0
	RecursiveLocker locker(fLock);

	int index = VADDR_TO_PDENT(address);
	if ((pd[index] & M68K_PDE_PRESENT) == 0)
		return false;

	ThreadCPUPinner pinner(thread_get_current_thread());

	page_table_entry* pt = (page_table_entry*)MapperGetPageTableAt(
		pd[index] & M68K_PDE_ADDRESS_MASK);

	index = VADDR_TO_PTENT(address);

	// perform the deed
	page_table_entry oldEntry;

	if (unmapIfUnaccessed) {
		while (true) {
			oldEntry = pt[index];
			if ((oldEntry & M68K_PTE_PRESENT) == 0) {
				// page mapping not valid
				return false;
			}

			if (oldEntry & M68K_PTE_ACCESSED) {
				// page was accessed -- just clear the flags
				oldEntry = M68KPagingMethod040::ClearPageTableEntryFlags(
					&pt[index], M68K_PTE_ACCESSED | M68K_PTE_DIRTY);
				break;
			}

			// page hasn't been accessed -- unmap it
			if (M68KPagingMethod040::TestAndSetPageTableEntry(&pt[index], 0,
					oldEntry) == oldEntry) {
				break;
			}

			// something changed -- check again
		}
	} else {
		oldEntry = M68KPagingMethod040::ClearPageTableEntryFlags(&pt[index],
			M68K_PTE_ACCESSED | M68K_PTE_DIRTY);
	}

	pinner.Unlock();

	_modified = (oldEntry & M68K_PTE_DIRTY) != 0;

	if ((oldEntry & M68K_PTE_ACCESSED) != 0) {
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
		(oldEntry & M68K_PTE_ADDRESS_MASK) / B_PAGE_SIZE);

#endif
	return false;
}


M68KPagingStructures*
M68KVMTranslationMap040::PagingStructures() const
{
	return fPagingStructures;
}


inline void *
M68KVMTranslationMap040::MapperGetPageTableAt(phys_addr_t physicalAddress,
	bool indirect)
{
	// M68K fits several page tables in a single page...
	uint32 offset = physicalAddress % B_PAGE_SIZE;
	ASSERT((indirect && (offset % 4) == 0) || (offset % SIZ_ROOTTBL) == 0);
	physicalAddress &= ~(B_PAGE_SIZE-1);
	void *va = fPageMapper->GetPageTableAt(physicalAddress);
	return (void *)((addr_t)va + offset);
}


