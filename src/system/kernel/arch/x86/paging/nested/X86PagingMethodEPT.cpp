/*
 * Copyright 2024, Daniel Martin, dalmemail@gmail.com
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/nested/X86PagingMethodEPT.h"

#include <stdlib.h>
#include <string.h>

#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/vm_page.h>

#include "paging/x86_physical_page_mapper.h"
#include "paging/x86_physical_page_mapper_mapped.h"


//#define TRACE_EPT_PAGING_METHOD
#ifdef TRACE_EPT_PAGING_METHOD
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


// #pragma mark - X86PagingMethodEPT


X86PagingMethodEPT::X86PagingMethodEPT()
	:
	fPhysicalPageMapper(NULL)
{
}


X86PagingMethodEPT::~X86PagingMethodEPT()
{
}


/*!	Traverses down the paging structure hierarchy to find the page directory
	for a virtual address, allocating new tables if required.
*/
/*static*/ uint64*
X86PagingMethodEPT::PageDirectoryForAddress(uint64* virtualPMLTop,
	addr_t virtualAddress, bool isKernel, bool allocateTables,
	vm_page_reservation* reservation,
	TranslationMapPhysicalPageMapper* pageMapper, int32& mapCount)
{
	uint64* virtualPML4 = virtualPMLTop;

	// Get the PDPT.
	uint64* pml4e = &virtualPML4[VADDR_TO_PML4E(virtualAddress)];
	if ((*pml4e & EPT_PML4E_PRESENT) == 0) {
		if (!allocateTables)
			return NULL;

		// Allocate a new PDPT.
		vm_page* page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		phys_addr_t physicalPDPT
			= (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("X86PagingMethodEPT::PageTableForAddress(): creating PDPT "
			"for va %#" B_PRIxADDR " at %#" B_PRIxPHYSADDR "\n", virtualAddress,
			physicalPDPT);

		SetTableEntry(pml4e, (physicalPDPT & EPT_PML4E_ADDRESS_MASK)
			| EPT_PML4E_PRESENT
			| EPT_PML4E_WRITABLE
			| EPT_PML4E_EXECUTABLE);

		mapCount++;
	}

	uint64* virtualPDPT = (uint64*)pageMapper->GetPageTableAt(
		*pml4e & EPT_PML4E_ADDRESS_MASK);

	// Get the page directory.
	uint64* pdpte = &virtualPDPT[VADDR_TO_PDPTE(virtualAddress)];
	if ((*pdpte & EPT_PDPTE_PRESENT) == 0) {
		if (!allocateTables)
			return NULL;

		// Allocate a new page directory.
		vm_page* page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		phys_addr_t physicalPageDir
			= (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("X86PagingMethodEPT::PageTableForAddress(): creating page "
			"directory for va %#" B_PRIxADDR " at %#" B_PRIxPHYSADDR "\n",
			virtualAddress, physicalPageDir);

		SetTableEntry(pdpte, (physicalPageDir & EPT_PDPTE_ADDRESS_MASK)
			| EPT_PDPTE_PRESENT
			| EPT_PDPTE_WRITABLE
			| EPT_PDPTE_EXECUTABLE);

		mapCount++;
	}

	return (uint64*)pageMapper->GetPageTableAt(
		*pdpte & EPT_PDPTE_ADDRESS_MASK);
}


/*static*/ uint64*
X86PagingMethodEPT::PageDirectoryEntryForAddress(uint64* virtualPMLTop,
	addr_t virtualAddress, bool isKernel, bool allocateTables,
	vm_page_reservation* reservation,
	TranslationMapPhysicalPageMapper* pageMapper, int32& mapCount)
{
	uint64* virtualPageDirectory = PageDirectoryForAddress(virtualPMLTop,
		virtualAddress, isKernel, allocateTables, reservation, pageMapper,
		mapCount);
	if (virtualPageDirectory == NULL)
		return NULL;

	return &virtualPageDirectory[VADDR_TO_PDE(virtualAddress)];
}


/*!	Traverses down the paging structure hierarchy to find the page table for a
	virtual address, allocating new tables if required.
*/
/*static*/ uint64*
X86PagingMethodEPT::PageTableForAddress(uint64* virtualPMLTop,
	addr_t virtualAddress, bool isKernel, bool allocateTables,
	vm_page_reservation* reservation,
	TranslationMapPhysicalPageMapper* pageMapper, int32& mapCount)
{
	TRACE("X86PagingMethodEPT::PageTableForAddress(%#" B_PRIxADDR ", "
		"%d)\n", virtualAddress, allocateTables);

	uint64* pde = PageDirectoryEntryForAddress(virtualPMLTop, virtualAddress,
		isKernel, allocateTables, reservation, pageMapper, mapCount);
	if (pde == NULL)
		return NULL;

	if ((*pde & EPT_PDE_PRESENT) == 0) {
		if (!allocateTables)
			return NULL;

		// Allocate a new page table.
		vm_page* page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		phys_addr_t physicalPageTable
			= (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("X86PagingMethodEPT::PageTableForAddress(): creating page "
			"table for va %#" B_PRIxADDR " at %#" B_PRIxPHYSADDR "\n",
			virtualAddress, physicalPageTable);

		SetTableEntry(pde, (physicalPageTable & EPT_PDE_ADDRESS_MASK)
			| EPT_PDE_PRESENT
			| EPT_PDE_WRITABLE
			| EPT_PDE_EXECUTABLE);

		mapCount++;
	}

	return (uint64*)pageMapper->GetPageTableAt(*pde & EPT_PDE_ADDRESS_MASK);
}


/*static*/ uint64*
X86PagingMethodEPT::PageTableEntryForAddress(uint64* virtualPMLTop,
	addr_t virtualAddress, bool isKernel, bool allocateTables,
	vm_page_reservation* reservation,
	TranslationMapPhysicalPageMapper* pageMapper, int32& mapCount)
{
	uint64* virtualPageTable = PageTableForAddress(virtualPMLTop, virtualAddress,
		isKernel, allocateTables, reservation, pageMapper, mapCount);
	if (virtualPageTable == NULL)
		return NULL;

	return &virtualPageTable[VADDR_TO_PTE(virtualAddress)];
}


/*static*/ void
X86PagingMethodEPT::PutPageTableEntryInTable(uint64* entry,
	phys_addr_t physicalAddress, uint32 attributes, uint32 memoryType,
	bool globalPage)
{
	uint64 page = (physicalAddress & EPT_PTE_ADDRESS_MASK)
		| EPT_PTE_PRESENT | EPT_PTE_IGNORE_PAT
		| MemoryTypeToPageTableEntryFlags(memoryType);

	if ((attributes & B_WRITE_AREA) != 0)
		page |= EPT_PTE_WRITABLE;
	if ((attributes & B_EXECUTE_AREA) != 0)
		page |= EPT_PTE_EXECUTABLE;

	// put it in the page table
	SetTableEntry(entry, page);
}
