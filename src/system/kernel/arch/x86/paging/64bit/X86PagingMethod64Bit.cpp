/*
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/64bit/X86PagingMethod64Bit.h"

#include <stdlib.h>
#include <string.h>

#include <boot/kernel_args.h>
#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/VMAddressSpace.h>

#include "paging/64bit/X86PagingStructures64Bit.h"
#include "paging/64bit/X86VMTranslationMap64Bit.h"
#include "paging/x86_physical_page_mapper.h"
#include "paging/x86_physical_page_mapper_mapped.h"


//#define TRACE_X86_PAGING_METHOD_64BIT
#ifdef TRACE_X86_PAGING_METHOD_64BIT
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


// #pragma mark - X86PagingMethod64Bit


X86PagingMethod64Bit::X86PagingMethod64Bit()
	:
	fKernelPhysicalPML4(0),
	fKernelVirtualPML4(NULL),
	fPhysicalPageMapper(NULL),
	fKernelPhysicalPageMapper(NULL)
{
}


X86PagingMethod64Bit::~X86PagingMethod64Bit()
{
}


status_t
X86PagingMethod64Bit::Init(kernel_args* args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	fKernelPhysicalPML4 = args->arch_args.phys_pgdir;
	fKernelVirtualPML4 = (uint64*)(addr_t)args->arch_args.vir_pgdir;

	// Ensure that the user half of the address space is clear. This removes
	// the temporary identity mapping made by the boot loader.
	memset(fKernelVirtualPML4, 0, sizeof(uint64) * 256);
	arch_cpu_global_TLB_invalidate();

	// Create the physical page mapper.
	mapped_physical_page_ops_init(args, fPhysicalPageMapper,
		fKernelPhysicalPageMapper);

	*_physicalPageMapper = fPhysicalPageMapper;
	return B_ERROR;
}


status_t
X86PagingMethod64Bit::InitPostArea(kernel_args* args)
{
	// Create an area to represent the kernel PML4.
	area_id area = create_area("kernel pml4", (void**)&fKernelVirtualPML4,
		B_EXACT_ADDRESS, B_PAGE_SIZE, B_ALREADY_WIRED,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	return B_OK;
}


status_t
X86PagingMethod64Bit::CreateTranslationMap(bool kernel, VMTranslationMap** _map)
{
	X86VMTranslationMap64Bit* map = new(std::nothrow) X86VMTranslationMap64Bit;
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
X86PagingMethod64Bit::MapEarly(kernel_args* args, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	page_num_t (*get_free_page)(kernel_args*))
{
	TRACE("X86PagingMethod64Bit::MapEarly(%#" B_PRIxADDR ", %#" B_PRIxPHYSADDR
		", %#" B_PRIx8 ")\n", virtualAddress, physicalAddress, attributes);

	// Get the PDPT. We should be mapping on an existing PDPT at this stage.
	uint64* pml4e = &fKernelVirtualPML4[VADDR_TO_PML4E(virtualAddress)];
	ASSERT((*pml4e & X86_64_PML4E_PRESENT) != 0);
	uint64* virtualPDPT = (uint64*)fKernelPhysicalPageMapper->GetPageTableAt(
		*pml4e & X86_64_PML4E_ADDRESS_MASK);

	// Get the page directory.
	uint64* pdpte = &virtualPDPT[VADDR_TO_PDPTE(virtualAddress)];
	uint64* virtualPageDir;
	if ((*pdpte & X86_64_PDPTE_PRESENT) == 0) {
		phys_addr_t physicalPageDir = get_free_page(args) * B_PAGE_SIZE;

		TRACE("X86PagingMethod64Bit::MapEarly(): creating page directory for va"
			" %#" B_PRIxADDR " at %#" B_PRIxPHYSADDR "\n", virtualAddress,
			physicalPageDir);

		SetTableEntry(pdpte, (physicalPageDir & X86_64_PDPTE_ADDRESS_MASK)
			| X86_64_PDPTE_PRESENT
			| X86_64_PDPTE_WRITABLE);

		// Map it and zero it.
		virtualPageDir = (uint64*)fKernelPhysicalPageMapper->GetPageTableAt(
			physicalPageDir);
		memset(virtualPageDir, 0, B_PAGE_SIZE);
	} else {
		virtualPageDir = (uint64*)fKernelPhysicalPageMapper->GetPageTableAt(
			*pdpte & X86_64_PDPTE_ADDRESS_MASK);
	}

	// Get the page table.
	uint64* pde = &virtualPageDir[VADDR_TO_PDE(virtualAddress)];
	uint64* virtualPageTable;
	if ((*pde & X86_64_PDE_PRESENT) == 0) {
		phys_addr_t physicalPageTable = get_free_page(args) * B_PAGE_SIZE;

		TRACE("X86PagingMethod64Bit::MapEarly(): creating page table for va"
			" %#" B_PRIxADDR " at %#" B_PRIxPHYSADDR "\n", virtualAddress,
			physicalPageTable);

		SetTableEntry(pde, (physicalPageTable & X86_64_PDE_ADDRESS_MASK)
			| X86_64_PDE_PRESENT
			| X86_64_PDE_WRITABLE);

		// Map it and zero it.
		virtualPageTable = (uint64*)fKernelPhysicalPageMapper->GetPageTableAt(
			physicalPageTable);
		memset(virtualPageTable, 0, B_PAGE_SIZE);
	} else {
		virtualPageTable = (uint64*)fKernelPhysicalPageMapper->GetPageTableAt(
			*pde & X86_64_PDE_ADDRESS_MASK);
	}

	// The page table entry must not already be mapped.
	uint64* pte = &virtualPageTable[VADDR_TO_PTE(virtualAddress)];
	ASSERT_PRINT(
		(*pte & X86_64_PTE_PRESENT) == 0,
		"virtual address: %#" B_PRIxADDR ", existing pte: %#" B_PRIx64,
		virtualAddress, *pte);

	// Fill in the table entry.
	PutPageTableEntryInTable(pte, physicalAddress, attributes, 0,
		IS_KERNEL_ADDRESS(virtualAddress));

	return B_OK;
}


bool
X86PagingMethod64Bit::IsKernelPageAccessible(addr_t virtualAddress,
	uint32 protection)
{
	return true;
}


/*!	Traverses down the paging structure hierarchy to find the page table for a
	virtual address, allocating new tables if required.
*/
/*static*/ uint64*
X86PagingMethod64Bit::PageTableForAddress(uint64* virtualPML4,
	addr_t virtualAddress, bool isKernel, bool allocateTables,
	vm_page_reservation* reservation,
	TranslationMapPhysicalPageMapper* pageMapper, int32& mapCount)
{
	TRACE("X86PagingMethod64Bit::PageTableEntryForAddress(%#" B_PRIxADDR ", "
		"%d)\n", virtualAddress, allocateTables);

	// Get the PDPT.
	uint64* pml4e = &virtualPML4[VADDR_TO_PML4E(virtualAddress)];
	if ((*pml4e & X86_64_PML4E_PRESENT) == 0) {
		if (!allocateTables)
			return NULL;

		// Allocate a new PDPT.
		vm_page* page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		phys_addr_t physicalPDPT
			= (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("X86PagingMethod64Bit::PageTableEntryForAddress(): creating PDPT "
			"for va %#" B_PRIxADDR " at %#" B_PRIxPHYSADDR "\n", virtualAddress,
			physicalPDPT);

		SetTableEntry(pml4e, (physicalPDPT & X86_64_PML4E_ADDRESS_MASK)
			| X86_64_PML4E_PRESENT
			| X86_64_PML4E_WRITABLE
			| (isKernel ? 0 : X86_64_PML4E_USER));

		mapCount++;
	}

	uint64* virtualPDPT = (uint64*)pageMapper->GetPageTableAt(
		*pml4e & X86_64_PML4E_ADDRESS_MASK);

	// Get the page directory.
	uint64* pdpte = &virtualPDPT[VADDR_TO_PDPTE(virtualAddress)];
	if ((*pdpte & X86_64_PDPTE_PRESENT) == 0) {
		if (!allocateTables)
			return NULL;

		// Allocate a new page directory.
		vm_page* page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		phys_addr_t physicalPageDir
			= (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("X86PagingMethod64Bit::PageTableEntryForAddress(): creating page "
			"directory for va %#" B_PRIxADDR " at %#" B_PRIxPHYSADDR "\n",
			virtualAddress, physicalPageDir);

		SetTableEntry(pdpte, (physicalPageDir & X86_64_PDPTE_ADDRESS_MASK)
			| X86_64_PDPTE_PRESENT
			| X86_64_PDPTE_WRITABLE
			| (isKernel ? 0 : X86_64_PDPTE_USER));

		mapCount++;
	}

	uint64* virtualPageDir = (uint64*)pageMapper->GetPageTableAt(
		*pdpte & X86_64_PDPTE_ADDRESS_MASK);

	// Get the page table.
	uint64* pde = &virtualPageDir[VADDR_TO_PDE(virtualAddress)];
	if ((*pde & X86_64_PDE_PRESENT) == 0) {
		if (!allocateTables)
			return NULL;

		// Allocate a new page table.
		vm_page* page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		phys_addr_t physicalPageTable
			= (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("X86PagingMethod64Bit::PageTableEntryForAddress(): creating page "
			"table for va %#" B_PRIxADDR " at %#" B_PRIxPHYSADDR "\n",
			virtualAddress, physicalPageTable);

		SetTableEntry(pde, (physicalPageTable & X86_64_PDE_ADDRESS_MASK)
			| X86_64_PDE_PRESENT
			| X86_64_PDE_WRITABLE
			| (isKernel ? 0 : X86_64_PDE_USER));

		mapCount++;
	}

	return (uint64*)pageMapper->GetPageTableAt(*pde & X86_64_PDE_ADDRESS_MASK);
}


/*static*/ uint64*
X86PagingMethod64Bit::PageTableEntryForAddress(uint64* virtualPML4,
	addr_t virtualAddress, bool isKernel, bool allocateTables,
	vm_page_reservation* reservation,
	TranslationMapPhysicalPageMapper* pageMapper, int32& mapCount)
{
	uint64* virtualPageTable = PageTableForAddress(virtualPML4, virtualAddress,
		isKernel, allocateTables, reservation, pageMapper, mapCount);
	if (virtualPageTable == NULL)
		return NULL;

	return &virtualPageTable[VADDR_TO_PTE(virtualAddress)];
}


/*static*/ void
X86PagingMethod64Bit::PutPageTableEntryInTable(uint64* entry,
	phys_addr_t physicalAddress, uint32 attributes, uint32 memoryType,
	bool globalPage)
{
	uint64 page = (physicalAddress & X86_64_PTE_ADDRESS_MASK)
		| X86_64_PTE_PRESENT | (globalPage ? X86_64_PTE_GLOBAL : 0)
		| MemoryTypeToPageTableEntryFlags(memoryType);

	// if the page is user accessible, it's automatically
	// accessible in kernel space, too (but with the same
	// protection)
	if ((attributes & B_USER_PROTECTION) != 0) {
		page |= X86_64_PTE_USER;
		if ((attributes & B_WRITE_AREA) != 0)
			page |= X86_64_PTE_WRITABLE;
	} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		page |= X86_64_PTE_WRITABLE;

	// put it in the page table
	SetTableEntry(entry, page);
}

