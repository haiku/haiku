/*
 * Copyright 2010, Ithamar R. Adema, ithamar.adema@team-embedded.nl
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/32bit/ARMPagingMethod32Bit.h"

#include <stdlib.h>
#include <string.h>

#include <AutoDeleter.h>

#include <arch_system_info.h>
#include <boot/kernel_args.h>
#include <int.h>
#include <thread.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>
#include <arm_mmu.h>

#include "paging/32bit/ARMPagingStructures32Bit.h"
#include "paging/32bit/ARMVMTranslationMap32Bit.h"
#include "paging/arm_physical_page_mapper.h"
#include "paging/arm_physical_page_mapper_large_memory.h"


//#define TRACE_ARM_PAGING_METHOD_32_BIT
#ifdef TRACE_ARM_PAGING_METHOD_32_BIT
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


using ARMLargePhysicalPageMapper::PhysicalPageSlot;


// #pragma mark - ARMPagingMethod32Bit::PhysicalPageSlotPool


struct ARMPagingMethod32Bit::PhysicalPageSlotPool
	: ARMLargePhysicalPageMapper::PhysicalPageSlotPool {
public:
	virtual						~PhysicalPageSlotPool();

			status_t			InitInitial(kernel_args* args);
			status_t			InitInitialPostArea(kernel_args* args);

			void				Init(area_id dataArea, void* data,
									area_id virtualArea, addr_t virtualBase);

	virtual	status_t			AllocatePool(
									ARMLargePhysicalPageMapper
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


ARMPagingMethod32Bit::PhysicalPageSlotPool
	ARMPagingMethod32Bit::PhysicalPageSlotPool::sInitialPhysicalPagePool;


ARMPagingMethod32Bit::PhysicalPageSlotPool::~PhysicalPageSlotPool()
{
}


status_t
ARMPagingMethod32Bit::PhysicalPageSlotPool::InitInitial(kernel_args* args)
{
	// allocate a virtual address range for the pages to be mapped into
	addr_t virtualBase = vm_allocate_early(args, 1024 * B_PAGE_SIZE, 0, 0,
		kPageTableAlignment);
	if (virtualBase == 0) {
		panic("LargeMemoryPhysicalPageMapper::Init(): Failed to reserve "
			"physical page pool space in virtual address space!");
		return B_ERROR;
	}

	// allocate memory for the page table and data
	size_t areaSize = B_PAGE_SIZE + sizeof(PhysicalPageSlot[1024]);
	page_table_entry* pageTable = (page_table_entry*)vm_allocate_early(args,
		areaSize, ~0L, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0);

	// prepare the page table
	_EarlyPreparePageTables(pageTable, virtualBase, 1024 * B_PAGE_SIZE);

	// init the pool structure and add the initial pool
	Init(-1, pageTable, -1, (addr_t)virtualBase);

	return B_OK;
}


status_t
ARMPagingMethod32Bit::PhysicalPageSlotPool::InitInitialPostArea(
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
ARMPagingMethod32Bit::PhysicalPageSlotPool::Init(area_id dataArea, void* data,
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
ARMPagingMethod32Bit::PhysicalPageSlotPool::Map(phys_addr_t physicalAddress,
	addr_t virtualAddress)
{
	page_table_entry& pte = fPageTable[
		(virtualAddress - fVirtualBase) / B_PAGE_SIZE];
	pte = (physicalAddress & ARM_PTE_ADDRESS_MASK)
		| ARM_PTE_TYPE_SMALL_PAGE;

	arch_cpu_invalidate_TLB_range(virtualAddress, virtualAddress + B_PAGE_SIZE);
//	invalidate_TLB(virtualAddress);
}


status_t
ARMPagingMethod32Bit::PhysicalPageSlotPool::AllocatePool(
	ARMLargePhysicalPageMapper::PhysicalPageSlotPool*& _pool)
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
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	area_id dataArea = create_area_etc(B_SYSTEM_TEAM, "physical page pool",
		PAGE_ALIGN(areaSize), B_FULL_LOCK,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, CREATE_AREA_DONT_WAIT,
		&virtualRestrictions, &physicalRestrictions, &data);
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
	ARMVMTranslationMap32Bit* map = static_cast<ARMVMTranslationMap32Bit*>(
		VMAddressSpace::Kernel()->TranslationMap());
	uint32 dummyFlags;
	cpu_status state = disable_interrupts();
	map->QueryInterrupt((addr_t)data, &physicalTable, &dummyFlags);
	restore_interrupts(state);

	// put the page table into the page directory
	int32 index = VADDR_TO_PDENT((addr_t)virtualBase);
	page_directory_entry* entry
		= &map->PagingStructures32Bit()->pgdir_virt[index];
	PutPageTableInPageDir(entry, physicalTable,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	ARMPagingStructures32Bit::UpdateAllPageDirs(index, *entry);

	// init the pool structure
	pool->Init(dataArea, data, virtualArea, (addr_t)virtualBase);
	poolDeleter.Detach();
	_pool = pool;
	return B_OK;
}


// #pragma mark - ARMPagingMethod32Bit


ARMPagingMethod32Bit::ARMPagingMethod32Bit()
	:
	fKernelPhysicalPageDirectory(0),
	fKernelVirtualPageDirectory(NULL),
	fPhysicalPageMapper(NULL),
	fKernelPhysicalPageMapper(NULL)
{
}


ARMPagingMethod32Bit::~ARMPagingMethod32Bit()
{
}


status_t
ARMPagingMethod32Bit::Init(kernel_args* args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	TRACE("vm_translation_map_init: entry\n");

	fKernelPhysicalPageDirectory = args->arch_args.phys_pgdir;
	fKernelVirtualPageDirectory = (page_directory_entry*)
		args->arch_args.vir_pgdir;

	TRACE("page dir: %p (physical: %#" B_PRIx32 ")\n",
		fKernelVirtualPageDirectory, fKernelPhysicalPageDirectory);

	ARMPagingStructures32Bit::StaticInit();

	// create the initial pool for the physical page mapper
	PhysicalPageSlotPool* pool
		= new(&PhysicalPageSlotPool::sInitialPhysicalPagePool)
			PhysicalPageSlotPool;
	status_t error = pool->InitInitial(args);
	if (error != B_OK) {
		panic("ARMPagingMethod32Bit::Init(): Failed to create initial pool "
			"for physical page mapper!");
		return error;
	}

	// create physical page mapper
	large_memory_physical_page_ops_init(args, pool, fPhysicalPageMapper,
		fKernelPhysicalPageMapper);
		// TODO: Select the best page mapper!

	// enable global page feature if available
#if 0 //IRA: check for ARMv6!!
	if (x86_check_feature(IA32_FEATURE_PGE, FEATURE_COMMON)) {
		// this prevents kernel pages from being flushed from TLB on
		// context-switch
		x86_write_cr4(x86_read_cr4() | IA32_CR4_GLOBAL_PAGES);
	}
#endif
	TRACE("ARMPagingMethod32Bit::Init(): done\n");

	*_physicalPageMapper = fPhysicalPageMapper;
	return B_OK;
}


status_t
ARMPagingMethod32Bit::InitPostArea(kernel_args* args)
{
	void *temp;
	status_t error;
	area_id area;

	temp = (void*)fKernelVirtualPageDirectory;
	area = create_area("kernel_pgdir", &temp, B_EXACT_ADDRESS, MMU_L1_TABLE_SIZE,
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
ARMPagingMethod32Bit::CreateTranslationMap(bool kernel, VMTranslationMap** _map)
{
	ARMVMTranslationMap32Bit* map = new(std::nothrow) ARMVMTranslationMap32Bit;
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
ARMPagingMethod32Bit::MapEarly(kernel_args* args, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	phys_addr_t (*get_free_page)(kernel_args*))
{
	// check to see if a page table exists for this range
	int index = VADDR_TO_PDENT(virtualAddress);
	if ((fKernelVirtualPageDirectory[index] & ARM_PDE_TYPE_MASK) == 0) {
		phys_addr_t pgtable;
		page_directory_entry *e;
		// we need to allocate a pgtable
		pgtable = get_free_page(args);
		// pgtable is in pages, convert to physical address
		pgtable *= B_PAGE_SIZE;

		TRACE("ARMPagingMethod32Bit::MapEarly(): asked for free page for "
			"pgtable. %#" B_PRIxPHYSADDR "\n", pgtable);

		// put it in the pgdir
		e = &fKernelVirtualPageDirectory[index];
		PutPageTableInPageDir(e, pgtable, attributes);

		// zero it out in it's new mapping
		memset((void*)pgtable, 0, B_PAGE_SIZE);
	}

	page_table_entry *ptEntry = (page_table_entry*)
		(fKernelVirtualPageDirectory[index] & ARM_PDE_ADDRESS_MASK);
	ptEntry += VADDR_TO_PTENT(virtualAddress);

	ASSERT_PRINT(
		(*ptEntry & ARM_PTE_TYPE_MASK) == 0,
		"virtual address: %#" B_PRIxADDR ", pde: %#" B_PRIx32
		", existing pte: %#" B_PRIx32, virtualAddress, fKernelVirtualPageDirectory[index],
		*ptEntry);

	// now, fill in the pentry
	PutPageTableEntryInTable(ptEntry,
		physicalAddress, attributes, 0, IS_KERNEL_ADDRESS(virtualAddress));

	return B_OK;
}


bool
ARMPagingMethod32Bit::IsKernelPageAccessible(addr_t virtualAddress,
	uint32 protection)
{
#if 0
	// We only trust the kernel team's page directory. So switch to it first.
	// Always set it to make sure the TLBs don't contain obsolete data.
	uint32 physicalPageDirectory;
	read_cr3(physicalPageDirectory);
	write_cr3(fKernelPhysicalPageDirectory);

	// get the page directory entry for the address
	page_directory_entry pageDirectoryEntry;
	uint32 index = VADDR_TO_PDENT(virtualAddress);

	if (physicalPageDirectory == fKernelPhysicalPageDirectory) {
		pageDirectoryEntry = fKernelVirtualPageDirectory[index];
	} else if (fPhysicalPageMapper != NULL) {
		// map the original page directory and get the entry
		void* handle;
		addr_t virtualPageDirectory;
		status_t error = fPhysicalPageMapper->GetPageDebug(
			physicalPageDirectory, &virtualPageDirectory, &handle);
		if (error == B_OK) {
			pageDirectoryEntry
				= ((page_directory_entry*)virtualPageDirectory)[index];
			fPhysicalPageMapper->PutPageDebug(virtualPageDirectory, handle);
		} else
			pageDirectoryEntry = 0;
	} else
		pageDirectoryEntry = 0;

	// map the page table and get the entry
	page_table_entry pageTableEntry;
	index = VADDR_TO_PTENT(virtualAddress);

	if ((pageDirectoryEntry & ARM_PDE_PRESENT) != 0
			&& fPhysicalPageMapper != NULL) {
		void* handle;
		addr_t virtualPageTable;
		status_t error = fPhysicalPageMapper->GetPageDebug(
			pageDirectoryEntry & ARM_PDE_ADDRESS_MASK, &virtualPageTable,
			&handle);
		if (error == B_OK) {
			pageTableEntry = ((page_table_entry*)virtualPageTable)[index];
			fPhysicalPageMapper->PutPageDebug(virtualPageTable, handle);
		} else
			pageTableEntry = 0;
	} else
		pageTableEntry = 0;

	// switch back to the original page directory
	if (physicalPageDirectory != fKernelPhysicalPageDirectory)
		write_cr3(physicalPageDirectory);

	if ((pageTableEntry & ARM_PTE_PRESENT) == 0)
		return false;

	// present means kernel-readable, so check for writable
	return (protection & B_KERNEL_WRITE_AREA) == 0
		|| (pageTableEntry & ARM_PTE_WRITABLE) != 0;
#endif
	//IRA: fix the above!
	return true;
}


/*static*/ void
ARMPagingMethod32Bit::PutPageTableInPageDir(page_directory_entry* entry,
	phys_addr_t pgtablePhysical, uint32 attributes)
{
	*entry = (pgtablePhysical & ARM_PDE_ADDRESS_MASK)
		| ARM_PDE_TYPE_COARSE_L2_PAGE_TABLE;
		// TODO: we ignore the attributes of the page table - for compatibility
		// with BeOS we allow having user accessible areas in the kernel address
		// space. This is currently being used by some drivers, mainly for the
		// frame buffer. Our current real time data implementation makes use of
		// this fact, too.
		// We might want to get rid of this possibility one day, especially if
		// we intend to port it to a platform that does not support this.
}


/*static*/ void
ARMPagingMethod32Bit::PutPageTableEntryInTable(page_table_entry* entry,
	phys_addr_t physicalAddress, uint32 attributes, uint32 memoryType,
	bool globalPage)
{
	page_table_entry page = (physicalAddress & ARM_PTE_ADDRESS_MASK)
		| ARM_PTE_TYPE_SMALL_PAGE;
#if 0 //IRA
		| ARM_PTE_PRESENT | (globalPage ? ARM_PTE_GLOBAL : 0)
		| MemoryTypeToPageTableEntryFlags(memoryType);

	// if the page is user accessible, it's automatically
	// accessible in kernel space, too (but with the same
	// protection)
	if ((attributes & B_USER_PROTECTION) != 0) {
		page |= ARM_PTE_USER;
		if ((attributes & B_WRITE_AREA) != 0)
			page |= ARM_PTE_WRITABLE;
	} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		page |= ARM_PTE_WRITABLE;
#endif
	// put it in the page table
	*(volatile page_table_entry*)entry = page;
}


/*static*/ void
ARMPagingMethod32Bit::_EarlyPreparePageTables(page_table_entry* pageTables,
	addr_t address, size_t size)
{
	ARMPagingMethod32Bit* method = ARMPagingMethod32Bit::Method();
	memset(pageTables, 0, 256 * (size / (B_PAGE_SIZE * 256)));

	// put the array of pgtables directly into the kernel pagedir
	// these will be wired and kept mapped into virtual space to be easy to get
	// to
	{
		addr_t virtualTable = (addr_t)pageTables;

		for (size_t i = 0; i < (size / (B_PAGE_SIZE * 256));
				i++, virtualTable += 256*sizeof(page_directory_entry)) {
			phys_addr_t physicalTable = 0;
			_EarlyQuery(virtualTable, &physicalTable);
			page_directory_entry* entry = method->KernelVirtualPageDirectory()
				+ VADDR_TO_PDENT(address) + i;
			PutPageTableInPageDir(entry, physicalTable,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		}
	}
}


//! TODO: currently assumes this translation map is active
/*static*/ status_t
ARMPagingMethod32Bit::_EarlyQuery(addr_t virtualAddress,
	phys_addr_t *_physicalAddress)
{
	ARMPagingMethod32Bit* method = ARMPagingMethod32Bit::Method();
	int index = VADDR_TO_PDENT(virtualAddress);
	if ((method->KernelVirtualPageDirectory()[index] & ARM_PDE_TYPE_MASK) == 0) {
		// no pagetable here
		return B_ERROR;
	}

	page_table_entry* entry = (page_table_entry*)
		(method->KernelVirtualPageDirectory()[index] & ARM_PDE_ADDRESS_MASK);
	entry += VADDR_TO_PTENT(virtualAddress);

	if ((*entry & ARM_PTE_TYPE_MASK) == 0) {
		// page mapping not valid
		return B_ERROR;
	}

	*_physicalAddress = (*entry & ARM_PTE_ADDRESS_MASK)
		| VADDR_TO_PGOFF(virtualAddress);

	return B_OK;
}
