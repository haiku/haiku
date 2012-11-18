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
#include <boot/kernel_args.h>
#include <int.h>
#include <thread.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

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
	PutPageTableInPageDir(entry, physicalTable,
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
	:
	fPageHole(NULL),
	fPageHolePageDir(NULL),
	fKernelPhysicalPageDirectory(0),
	fKernelVirtualPageDirectory(NULL),
	fPhysicalPageMapper(NULL),
	fKernelPhysicalPageMapper(NULL)
{
}


X86PagingMethod32Bit::~X86PagingMethod32Bit()
{
}


status_t
X86PagingMethod32Bit::Init(kernel_args* args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	TRACE("X86PagingMethod32Bit::Init(): entry\n");

	// page hole set up in stage2
	fPageHole = (page_table_entry*)(addr_t)args->arch_args.page_hole;
	// calculate where the pgdir would be
	fPageHolePageDir = (page_directory_entry*)
		(((addr_t)args->arch_args.page_hole)
			+ (B_PAGE_SIZE * 1024 - B_PAGE_SIZE));
	// clear out the bottom 2 GB, unmap everything
	memset(fPageHolePageDir + FIRST_USER_PGDIR_ENT, 0,
		sizeof(page_directory_entry) * NUM_USER_PGDIR_ENTS);

	fKernelPhysicalPageDirectory = args->arch_args.phys_pgdir;
	fKernelVirtualPageDirectory = (page_directory_entry*)(addr_t)
		args->arch_args.vir_pgdir;

#ifdef TRACE_X86_PAGING_METHOD_32_BIT
	TRACE("page hole: %p, page dir: %p\n", fPageHole, fPageHolePageDir);
	TRACE("page dir: %p (physical: %#" B_PRIx32 ")\n",
		fKernelVirtualPageDirectory, fKernelPhysicalPageDirectory);
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
	large_memory_physical_page_ops_init(args, pool, fPhysicalPageMapper,
		fKernelPhysicalPageMapper);
		// TODO: Select the best page mapper!

	// enable global page feature if available
	if (x86_check_feature(IA32_FEATURE_PGE, FEATURE_COMMON)) {
		// this prevents kernel pages from being flushed from TLB on
		// context-switch
		x86_write_cr4(x86_read_cr4() | IA32_CR4_GLOBAL_PAGES);
	}

	TRACE("X86PagingMethod32Bit::Init(): done\n");

	*_physicalPageMapper = fPhysicalPageMapper;
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
	fKernelVirtualPageDirectory[1023] = 0;
	fPageHolePageDir = NULL;
	fPageHole = NULL;

	temp = (void*)fKernelVirtualPageDirectory;
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
	page_num_t (*get_free_page)(kernel_args*))
{
	// XXX horrible back door to map a page quickly regardless of translation
	// map object, etc. used only during VM setup.
	// uses a 'page hole' set up in the stage 2 bootloader. The page hole is
	// created by pointing one of the pgdir entries back at itself, effectively
	// mapping the contents of all of the 4MB of pagetables into a 4 MB region.
	// It's only used here, and is later unmapped.

	// check to see if a page table exists for this range
	int index = VADDR_TO_PDENT(virtualAddress);
	if ((fPageHolePageDir[index] & X86_PDE_PRESENT) == 0) {
		phys_addr_t pgtable;
		page_directory_entry *e;
		// we need to allocate a pgtable
		pgtable = get_free_page(args);
		// pgtable is in pages, convert to physical address
		pgtable *= B_PAGE_SIZE;

		TRACE("X86PagingMethod32Bit::MapEarly(): asked for free page for "
			"pgtable. %#" B_PRIxPHYSADDR "\n", pgtable);

		// put it in the pgdir
		e = &fPageHolePageDir[index];
		PutPageTableInPageDir(e, pgtable, attributes);

		// zero it out in it's new mapping
		memset((unsigned int*)((addr_t)fPageHole
				+ (virtualAddress / B_PAGE_SIZE / 1024) * B_PAGE_SIZE),
			0, B_PAGE_SIZE);
	}

	ASSERT_PRINT(
		(fPageHole[virtualAddress / B_PAGE_SIZE] & X86_PTE_PRESENT) == 0,
		"virtual address: %#" B_PRIxADDR ", pde: %#" B_PRIx32
		", existing pte: %#" B_PRIx32, virtualAddress, fPageHolePageDir[index],
		fPageHole[virtualAddress / B_PAGE_SIZE]);

	// now, fill in the pentry
	PutPageTableEntryInTable(fPageHole + virtualAddress / B_PAGE_SIZE,
		physicalAddress, attributes, 0, IS_KERNEL_ADDRESS(virtualAddress));

	return B_OK;
}


bool
X86PagingMethod32Bit::IsKernelPageAccessible(addr_t virtualAddress,
	uint32 protection)
{
	// We only trust the kernel team's page directory. So switch to it first.
	// Always set it to make sure the TLBs don't contain obsolete data.
	uint32 physicalPageDirectory = x86_read_cr3();
	x86_write_cr3(fKernelPhysicalPageDirectory);

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

	if ((pageDirectoryEntry & X86_PDE_PRESENT) != 0
			&& fPhysicalPageMapper != NULL) {
		void* handle;
		addr_t virtualPageTable;
		status_t error = fPhysicalPageMapper->GetPageDebug(
			pageDirectoryEntry & X86_PDE_ADDRESS_MASK, &virtualPageTable,
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
		x86_write_cr3(physicalPageDirectory);

	if ((pageTableEntry & X86_PTE_PRESENT) == 0)
		return false;

	// present means kernel-readable, so check for writable
	return (protection & B_KERNEL_WRITE_AREA) == 0
		|| (pageTableEntry & X86_PTE_WRITABLE) != 0;
}


/*static*/ void
X86PagingMethod32Bit::PutPageTableInPageDir(page_directory_entry* entry,
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


/*static*/ void
X86PagingMethod32Bit::PutPageTableEntryInTable(page_table_entry* entry,
	phys_addr_t physicalAddress, uint32 attributes, uint32 memoryType,
	bool globalPage)
{
	page_table_entry page = (physicalAddress & X86_PTE_ADDRESS_MASK)
		| X86_PTE_PRESENT | (globalPage ? X86_PTE_GLOBAL : 0)
		| MemoryTypeToPageTableEntryFlags(memoryType);

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


/*static*/ void
X86PagingMethod32Bit::_EarlyPreparePageTables(page_table_entry* pageTables,
	addr_t address, size_t size)
{
	memset(pageTables, 0, B_PAGE_SIZE * (size / (B_PAGE_SIZE * 1024)));

	// put the array of pgtables directly into the kernel pagedir
	// these will be wired and kept mapped into virtual space to be easy to get
	// to
	{
		addr_t virtualTable = (addr_t)pageTables;

		page_directory_entry* pageHolePageDir
			= X86PagingMethod32Bit::Method()->PageHolePageDir();

		for (size_t i = 0; i < (size / (B_PAGE_SIZE * 1024));
				i++, virtualTable += B_PAGE_SIZE) {
			phys_addr_t physicalTable = 0;
			_EarlyQuery(virtualTable, &physicalTable);
			page_directory_entry* entry = &pageHolePageDir[
				(address / (B_PAGE_SIZE * 1024)) + i];
			PutPageTableInPageDir(entry, physicalTable,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		}
	}
}


//! TODO: currently assumes this translation map is active
/*static*/ status_t
X86PagingMethod32Bit::_EarlyQuery(addr_t virtualAddress,
	phys_addr_t *_physicalAddress)
{
	X86PagingMethod32Bit* method = X86PagingMethod32Bit::Method();
	int index = VADDR_TO_PDENT(virtualAddress);
	if ((method->PageHolePageDir()[index] & X86_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_ERROR;
	}

	page_table_entry* entry = method->PageHole() + virtualAddress / B_PAGE_SIZE;
	if ((*entry & X86_PTE_PRESENT) == 0) {
		// page mapping not valid
		return B_ERROR;
	}

	*_physicalAddress = *entry & X86_PTE_ADDRESS_MASK;
	return B_OK;
}
