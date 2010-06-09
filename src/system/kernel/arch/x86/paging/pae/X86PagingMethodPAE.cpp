/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "paging/pae/X86PagingMethodPAE.h"

#include <stdlib.h>
#include <string.h>

#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include "paging/32bit/paging.h"
#include "paging/32bit/X86PagingMethod32Bit.h"
#include "paging/pae/X86PagingStructuresPAE.h"
#include "paging/pae/X86VMTranslationMapPAE.h"
#include "paging/x86_physical_page_mapper.h"
#include "paging/x86_physical_page_mapper_large_memory.h"


#define TRACE_X86_PAGING_METHOD_PAE
#ifdef TRACE_X86_PAGING_METHOD_PAE
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


#if B_HAIKU_PHYSICAL_BITS == 64


using X86LargePhysicalPageMapper::PhysicalPageSlot;


// #pragma mark - ToPAESwitcher


struct X86PagingMethodPAE::ToPAESwitcher {
	ToPAESwitcher(kernel_args* args)
		:
		fKernelArgs(args)
	{
		// page hole set up in the boot loader
		fPageHole = (page_table_entry*)fKernelArgs->arch_args.page_hole;

		// calculate where the page dir would be
		fPageHolePageDir = (page_directory_entry*)
			(((addr_t)fKernelArgs->arch_args.page_hole)
				+ (B_PAGE_SIZE * 1024 - B_PAGE_SIZE));

		fPhysicalPageDir = fKernelArgs->arch_args.phys_pgdir;

		TRACE("page hole: %p\n", fPageHole);
		TRACE("page dir:  %p (physical: %#" B_PRIxPHYSADDR ")\n",
			fPageHolePageDir, fPhysicalPageDir);
	}

	void Switch(void*& _pageStructures, size_t& _pageStructuresSize,
		pae_page_directory_entry** pageDirs, phys_addr_t* physicalPageDirs,
		addr_t& _freeVirtualSlot, pae_page_table_entry*& _freeVirtualSlotPTE)
	{
		// count the page tables we have to translate
		uint32 pageTableCount = 0;
		for (uint32 i = FIRST_KERNEL_PGDIR_ENT;
			i < FIRST_KERNEL_PGDIR_ENT + NUM_KERNEL_PGDIR_ENTS; i++) {
			page_directory_entry entry = fPageHolePageDir[i];
			if ((entry & X86_PDE_PRESENT) != 0)
				pageTableCount++;
		}

		TRACE("page tables to translate: %" B_PRIu32 "\n", pageTableCount);

		// The pages we need to allocate to do our job:
		// + 1 page dir pointer table
		// + 4 page dirs
		// + 2 * page tables (each has 512 instead of 1024 entries)
		// + 1 page for the free virtual slot (no physical page needed)
		uint32 pagesNeeded = 1 + 4 + pageTableCount * 2 + 1;

		// We need additional PAE page tables for the new pages we're going to
		// allocate: Two tables for every 1024 pages to map, i.e. 2 additional
		// pages for every 1022 pages we want to allocate. We also need 32 bit
		// page tables, but we don't need additional virtual space for them,
		// since we can access then via the page hole.
		pagesNeeded += ((pagesNeeded + 1021) / 1022) * 2;

		TRACE("pages needed: %" B_PRIu32 "\n", pagesNeeded);

		// allocate the pages we need
		_AllocateNeededPages(pagesNeeded);

		// prepare the page directory pointer table
		phys_addr_t physicalPDPT;
		pae_page_directory_pointer_table_entry* pdpt
			= (pae_page_directory_pointer_table_entry*)_NextPage(true,
				physicalPDPT);

		for (int32 i = 0; i < 4; i++) {
			fPageDirs[i] = (pae_page_directory_entry*)_NextPage(true,
				fPhysicalPageDirs[i]);

			pdpt[i] = X86_PAE_PDPTE_PRESENT
				| (fPhysicalPageDirs[i] & X86_PAE_PDPTE_ADDRESS_MASK);
		}

		// Since we have to enable PAE in two steps -- setting cr3 to the PDPT
		// and setting the cr4 PAE bit -- we copy the kernel page dir entries to
		// the PDPT page, so after setting cr3, we continue to have working
		// kernel mappings. This requires that the PDPTE registers and the
		// page dir entries don't interect, obviously.
		ASSERT(4 * sizeof(pae_page_directory_pointer_table_entry)
			<= FIRST_KERNEL_PGDIR_ENT * sizeof(page_directory_entry));

		// translate the page tables
		for (uint32 i = FIRST_KERNEL_PGDIR_ENT;
			i < FIRST_KERNEL_PGDIR_ENT + NUM_KERNEL_PGDIR_ENTS; i++) {
			if ((fPageHolePageDir[i] & X86_PDE_PRESENT) != 0) {
				// two PAE page tables per 32 bit page table
				_TranslatePageTable((addr_t)i * 1024 * B_PAGE_SIZE);
				_TranslatePageTable(((addr_t)i * 1024 + 512) * B_PAGE_SIZE);

				// copy the page directory entry to the PDPT page
				((page_directory_entry*)pdpt)[i] = fPageHolePageDir[i];
			}
		}

		TRACE("free virtual slot: %#" B_PRIxADDR ", PTE: %p\n",
			fFreeVirtualSlot, fFreeVirtualSlotPTE);

		// switch to PAE
		write_cr3((uint32)physicalPDPT);
		x86_write_cr4(x86_read_cr4() | IA32_CR4_PAE | IA32_CR4_GLOBAL_PAGES);

		// set return values
		_pageStructures = fAllocatedPages;
		_pageStructuresSize = (size_t)fUsedPagesCount * B_PAGE_SIZE;
		memcpy(pageDirs, fPageDirs, sizeof(fPageDirs));
		memcpy(physicalPageDirs, fPhysicalPageDirs, sizeof(fPhysicalPageDirs));

		_freeVirtualSlot = fFreeVirtualSlot;
		_freeVirtualSlotPTE = fFreeVirtualSlotPTE;
	}

private:
	void _TranslatePageTable(addr_t virtualBase)
	{
		page_table_entry* entry = &fPageHole[virtualBase / B_PAGE_SIZE];

		// allocate a PAE page table
		phys_addr_t physicalTable;
		pae_page_table_entry* paeTable = (pae_page_table_entry*)_NextPage(false,
			physicalTable);

		// enter it into the page dir
		pae_page_directory_entry* pageDirEntry = PageDirEntryForAddress(
			fPageDirs, virtualBase);
		PutPageTableInPageDir(pageDirEntry, physicalTable,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

		pae_page_table_entry* paeEntry = paeTable;
		for (uint32 i = 0; i < kPAEPageTableEntryCount;
				i++, entry++, paeEntry++) {
			if ((*entry & X86_PTE_PRESENT) != 0) {
				// Note, we use the fact that the PAE flags are defined to the
				// same values.
				*paeEntry = *entry & (X86_PTE_PRESENT
					| X86_PTE_WRITABLE
					| X86_PTE_USER
					| X86_PTE_WRITE_THROUGH
					| X86_PTE_CACHING_DISABLED
					| X86_PTE_GLOBAL
					| X86_PTE_ADDRESS_MASK);
			} else
				*paeEntry = 0;
		}

		if (fFreeVirtualSlot / kPAEPageTableRange
				== virtualBase / kPAEPageTableRange) {
			fFreeVirtualSlotPTE = paeTable
				+ fFreeVirtualSlot / B_PAGE_SIZE % kPAEPageTableEntryCount;
		}
	}

	void _AllocateNeededPages(uint32 pagesNeeded)
	{
		size_t virtualSize = ROUNDUP(pagesNeeded, 1024) * B_PAGE_SIZE;
		addr_t virtualBase = vm_allocate_early(fKernelArgs, virtualSize, 0, 0,
			kPageTableAlignment);
		if (virtualBase == 0) {
			panic("Failed to reserve virtual address space for the switch to "
				"PAE!");
		}

		TRACE("virtual space: %#" B_PRIxADDR ", size: %#" B_PRIxSIZE "\n",
			virtualBase, virtualSize);

		// allocate pages for the 32 bit page tables and prepare the tables
		uint32 oldPageTableCount = virtualSize / B_PAGE_SIZE / 1024;
		for (uint32 i = 0; i < oldPageTableCount; i++) {
			// allocate a page
			phys_addr_t physicalTable =_AllocatePage32Bit();
TRACE("allocated page table: %#" B_PRIxPHYSADDR "\n", physicalTable);

			// put the page into the page dir
			page_directory_entry* entry = &fPageHolePageDir[
				virtualBase / B_PAGE_SIZE / 1024 + i];
TRACE("  -> mapping in pde %p (%#lx)\n", entry, *entry);
			X86PagingMethod32Bit::PutPageTableInPageDir(entry, physicalTable,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

			// clear the table
			memset((void*)((addr_t)fPageHole
					+ (virtualBase / B_PAGE_SIZE / 1024 + i) * B_PAGE_SIZE),
				0, B_PAGE_SIZE);
		}

		// We don't need a physical page for the free virtual slot.
		pagesNeeded--;

		// allocate and map the pages we need
		for (uint32 i = 0; i < pagesNeeded; i++) {
			// allocate a page
			phys_addr_t physicalAddress =_AllocatePage32Bit();
//TRACE("allocated page: %#" B_PRIxPHYSADDR "\n", physicalAddress);

			// put the page into the page table
			page_table_entry* entry = fPageHole + virtualBase / B_PAGE_SIZE + i;
//TRACE("  -> mapping in pte %p (%#lx)\n", entry, *entry);
			X86PagingMethod32Bit::PutPageTableEntryInTable(entry,
				physicalAddress, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0,
				true);

			// Write the page's physical address into the page itself, so we
			// don't need to look it up later.
			*(phys_addr_t*)(virtualBase + i * B_PAGE_SIZE) = physicalAddress;
		}

		fAllocatedPages = (uint8*)virtualBase;
		fAllocatedPagesCount = pagesNeeded;
		fUsedPagesCount = 0;
		fFreeVirtualSlot
			= (addr_t)(fAllocatedPages + pagesNeeded * B_PAGE_SIZE);
	}

	phys_addr_t _AllocatePage()
	{
		phys_addr_t physicalAddress
			= (phys_addr_t)vm_allocate_early_physical_page(fKernelArgs)
				* B_PAGE_SIZE;
		if (physicalAddress == 0)
			panic("Failed to allocate page for the switch to PAE!");
		return physicalAddress;
	}

	phys_addr_t _AllocatePage32Bit()
	{
		phys_addr_t physicalAddress = _AllocatePage();
		if (physicalAddress > 0xffffffff) {
			panic("Failed to allocate 32 bit addressable page for the switch "
				"to PAE!");
			return 0;
		}
		return physicalAddress;
	}

	void* _NextPage(bool clearPage, phys_addr_t& _physicalAddress)
	{
		if (fUsedPagesCount >= fAllocatedPagesCount) {
			panic("X86PagingMethodPAE::ToPAESwitcher::_NextPage(): no more "
				"allocated pages!");
			return NULL;
		}

		void* page = fAllocatedPages + (fUsedPagesCount++) * B_PAGE_SIZE;
		_physicalAddress = *((phys_addr_t*)page);

		if (clearPage)
			memset(page, 0, B_PAGE_SIZE);

		return page;
	}

private:
	kernel_args*				fKernelArgs;
	page_table_entry*			fPageHole;
	page_directory_entry*		fPageHolePageDir;
	phys_addr_t					fPhysicalPageDir;
	uint8*						fAllocatedPages;
	uint32						fAllocatedPagesCount;
	uint32						fUsedPagesCount;
	addr_t						fFreeVirtualSlot;
	pae_page_table_entry*		fFreeVirtualSlotPTE;
	pae_page_directory_entry*	fPageDirs[4];
	phys_addr_t					fPhysicalPageDirs[4];
};


// #pragma mark - PhysicalPageSlotPool


struct X86PagingMethodPAE::PhysicalPageSlotPool
	: X86LargePhysicalPageMapper::PhysicalPageSlotPool {
public:
	virtual						~PhysicalPageSlotPool();

			status_t			InitInitial(X86PagingMethodPAE* method,
									kernel_args* args);
			status_t			InitInitialPostArea(kernel_args* args);

			void				Init(area_id dataArea,
									pae_page_table_entry* pageTable,
									area_id virtualArea, addr_t virtualBase);

	virtual	status_t			AllocatePool(
									X86LargePhysicalPageMapper
										::PhysicalPageSlotPool*& _pool);
	virtual	void				Map(phys_addr_t physicalAddress,
									addr_t virtualAddress);

public:
	static	PhysicalPageSlotPool sInitialPhysicalPagePool;

private:
			area_id				fDataArea;
			area_id				fVirtualArea;
			addr_t				fVirtualBase;
			pae_page_table_entry* fPageTable;
};


X86PagingMethodPAE::PhysicalPageSlotPool
	X86PagingMethodPAE::PhysicalPageSlotPool::sInitialPhysicalPagePool;


X86PagingMethodPAE::PhysicalPageSlotPool::~PhysicalPageSlotPool()
{
}


status_t
X86PagingMethodPAE::PhysicalPageSlotPool::InitInitial(
	X86PagingMethodPAE* method, kernel_args* args)
{
	// allocate a virtual address range for the pages to be mapped into
	addr_t virtualBase = vm_allocate_early(args, kPAEPageTableRange, 0, 0,
		kPAEPageTableRange);
	if (virtualBase == 0) {
		panic("LargeMemoryPhysicalPageMapper::Init(): Failed to reserve "
			"physical page pool space in virtual address space!");
		return B_ERROR;
	}

	// allocate memory for the page table and data
	size_t areaSize = B_PAGE_SIZE
		+ sizeof(PhysicalPageSlot[kPAEPageTableEntryCount]);
	pae_page_table_entry* pageTable = (pae_page_table_entry*)vm_allocate_early(
		args, areaSize, ~0L, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0);

	// clear the page table and put it in the page dir
	memset(pageTable, 0, B_PAGE_SIZE);

	phys_addr_t physicalTable = 0;
	method->_EarlyQuery((addr_t)pageTable, &physicalTable);

	pae_page_directory_entry* entry = PageDirEntryForAddress(
		method->KernelVirtualPageDirs(), virtualBase);
	PutPageTableInPageDir(entry, physicalTable,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	// init the pool structure and add the initial pool
	Init(-1, pageTable, -1, (addr_t)virtualBase);

	return B_OK;
}


status_t
X86PagingMethodPAE::PhysicalPageSlotPool::InitInitialPostArea(
	kernel_args* args)
{
	// create an area for the (already allocated) data
	size_t areaSize = B_PAGE_SIZE
		+ sizeof(PhysicalPageSlot[kPAEPageTableEntryCount]);
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
		kPAEPageTableRange, 0);
	if (area < B_OK) {
		panic("LargeMemoryPhysicalPageMapper::InitPostArea(): Failed to "
			"create area for physical page pool space.");
		return area;
	}
	fVirtualArea = area;

	return B_OK;
}


void
X86PagingMethodPAE::PhysicalPageSlotPool::Init(area_id dataArea,
	pae_page_table_entry* pageTable, area_id virtualArea, addr_t virtualBase)
{
	fDataArea = dataArea;
	fVirtualArea = virtualArea;
	fVirtualBase = virtualBase;
	fPageTable = pageTable;

	// init slot list
	fSlots = (PhysicalPageSlot*)(fPageTable + kPAEPageTableEntryCount);
	addr_t slotAddress = virtualBase;
	for (uint32 i = 0; i < kPAEPageTableEntryCount;
			i++, slotAddress += B_PAGE_SIZE) {
		PhysicalPageSlot* slot = &fSlots[i];
		slot->next = slot + 1;
		slot->pool = this;
		slot->address = slotAddress;
	}

	fSlots[kPAEPageTableEntryCount - 1].next = NULL;
		// terminate list
}


void
X86PagingMethodPAE::PhysicalPageSlotPool::Map(phys_addr_t physicalAddress,
	addr_t virtualAddress)
{
// TODO: Implement!
	panic("X86PagingMethodPAE::PhysicalPageSlotPool::Map(): not implemented");
}


status_t
X86PagingMethodPAE::PhysicalPageSlotPool::AllocatePool(
	X86LargePhysicalPageMapper::PhysicalPageSlotPool*& _pool)
{
// TODO: Implement!
	panic("X86PagingMethodPAE::PhysicalPageSlotPool::AllocatePool(): not implemented");
	return B_UNSUPPORTED;
}


// #pragma mark - X86PagingMethodPAE


X86PagingMethodPAE::X86PagingMethodPAE()
	:
	fPhysicalPageMapper(NULL),
	fKernelPhysicalPageMapper(NULL)
{
}


X86PagingMethodPAE::~X86PagingMethodPAE()
{
}


status_t
X86PagingMethodPAE::Init(kernel_args* args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	// switch to PAE
	ToPAESwitcher(args).Switch(fEarlyPageStructures, fEarlyPageStructuresSize,
		fKernelVirtualPageDirs, fKernelPhysicalPageDirs, fFreeVirtualSlot,
		fFreeVirtualSlotPTE);

	// create the initial pool for the physical page mapper
	PhysicalPageSlotPool* pool
		= new(&PhysicalPageSlotPool::sInitialPhysicalPagePool)
			PhysicalPageSlotPool;
	status_t error = pool->InitInitial(this, args);
	if (error != B_OK) {
		panic("X86PagingMethodPAE::Init(): Failed to create initial pool "
			"for physical page mapper!");
		return error;
	}

	// create physical page mapper
	large_memory_physical_page_ops_init(args, pool, fPhysicalPageMapper,
		fKernelPhysicalPageMapper);

	*_physicalPageMapper = fPhysicalPageMapper;
	return B_OK;
}


status_t
X86PagingMethodPAE::InitPostArea(kernel_args* args)
{
// TODO: Implement!
	panic("X86PagingMethodPAE::InitPostArea(): not implemented");
	return B_UNSUPPORTED;
}


status_t
X86PagingMethodPAE::CreateTranslationMap(bool kernel, VMTranslationMap** _map)
{
// TODO: Implement!
	panic("X86PagingMethodPAE::CreateTranslationMap(): not implemented");
	return B_UNSUPPORTED;
}


status_t
X86PagingMethodPAE::MapEarly(kernel_args* args, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	phys_addr_t (*get_free_page)(kernel_args*))
{
	// check to see if a page table exists for this range
	pae_page_directory_entry* pageDirEntry = PageDirEntryForAddress(
		fKernelVirtualPageDirs, virtualAddress);
	pae_page_table_entry* pageTable;
	if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0) {
		// we need to allocate a page table
		phys_addr_t physicalPageTable = get_free_page(args) * B_PAGE_SIZE;

		TRACE("X86PagingMethodPAE::MapEarly(): asked for free page for "
			"page table: %#" B_PRIxPHYSADDR "\n", physicalPageTable);

		// put it in the pgdir
		PutPageTableInPageDir(pageDirEntry, physicalPageTable, attributes);

		// zero it out
		pageTable = _EarlyGetPageTable(physicalPageTable);
		memset(pageTable, 0, B_PAGE_SIZE);
	} else {
		// table already exists -- map it
		pageTable = _EarlyGetPageTable(
			*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK);
	}

	pae_page_table_entry* entry = pageTable
		+ virtualAddress / B_PAGE_SIZE % kPAEPageTableEntryCount;

	ASSERT_PRINT(
		(*entry & X86_PAE_PTE_PRESENT) == 0,
		"virtual address: %#" B_PRIxADDR ", pde: %#" B_PRIx64
		", existing pte: %#" B_PRIx64, virtualAddress, *pageDirEntry, *entry);

	// now, fill in the pentry
	PutPageTableEntryInTable(entry, physicalAddress, attributes, 0,
		IS_KERNEL_ADDRESS(virtualAddress));

	return B_OK;
}


bool
X86PagingMethodPAE::IsKernelPageAccessible(addr_t virtualAddress,
	uint32 protection)
{
// TODO: Implement!
	return false;
}


/*static*/ void
X86PagingMethodPAE::PutPageTableInPageDir(pae_page_directory_entry* entry,
	phys_addr_t physicalTable, uint32 attributes)
{
	*entry = (physicalTable & X86_PAE_PDE_ADDRESS_MASK)
		| X86_PAE_PDE_PRESENT
		| X86_PAE_PDE_WRITABLE
		| X86_PAE_PDE_USER;
		// TODO: We ignore the attributes of the page table -- for compatibility
		// with BeOS we allow having user accessible areas in the kernel address
		// space. This is currently being used by some drivers, mainly for the
		// frame buffer. Our current real time data implementation makes use of
		// this fact, too.
		// We might want to get rid of this possibility one day, especially if
		// we intend to port it to a platform that does not support this.
}


/*static*/ void
X86PagingMethodPAE::PutPageTableEntryInTable(pae_page_table_entry* entry,
	phys_addr_t physicalAddress, uint32 attributes, uint32 memoryType,
	bool globalPage)
{
	page_table_entry page = (physicalAddress & X86_PAE_PTE_ADDRESS_MASK)
		| X86_PAE_PTE_PRESENT | (globalPage ? X86_PAE_PTE_GLOBAL : 0)
		| MemoryTypeToPageTableEntryFlags(memoryType);

	// if the page is user accessible, it's automatically
	// accessible in kernel space, too (but with the same
	// protection)
	if ((attributes & B_USER_PROTECTION) != 0) {
		page |= X86_PAE_PTE_USER;
		if ((attributes & B_WRITE_AREA) != 0)
			page |= X86_PAE_PTE_WRITABLE;
	} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		page |= X86_PAE_PTE_WRITABLE;

	// put it in the page table
	*(volatile pae_page_table_entry*)entry = page;
}


bool
X86PagingMethodPAE::_EarlyQuery(addr_t virtualAddress,
	phys_addr_t* _physicalAddress)
{
	pae_page_directory_entry* pageDirEntry = PageDirEntryForAddress(
		fKernelVirtualPageDirs, virtualAddress);
	if ((*pageDirEntry & X86_PAE_PDE_PRESENT) == 0) {
		// no pagetable here
		return false;
	}

	pae_page_table_entry* entry = _EarlyGetPageTable(
			*pageDirEntry & X86_PAE_PDE_ADDRESS_MASK)
		+ virtualAddress / B_PAGE_SIZE % kPAEPageTableEntryCount;
	if ((*entry & X86_PAE_PTE_PRESENT) == 0) {
		// page mapping not valid
		return false;
	}

	*_physicalAddress = *entry & X86_PAE_PTE_ADDRESS_MASK;
	return true;
}


pae_page_table_entry*
X86PagingMethodPAE::_EarlyGetPageTable(phys_addr_t address)
{
	*fFreeVirtualSlotPTE = (address & X86_PAE_PTE_ADDRESS_MASK)
		| X86_PAE_PTE_PRESENT | X86_PAE_PTE_WRITABLE | X86_PAE_PTE_GLOBAL;

	invalidate_TLB(fFreeVirtualSlot);

	return (pae_page_table_entry*)fFreeVirtualSlot;
}


#endif	// B_HAIKU_PHYSICAL_BITS == 64
