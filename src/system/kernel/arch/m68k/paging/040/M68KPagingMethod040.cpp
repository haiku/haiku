/*
 * Copyright 2010-2012, François, revol@free.fr.
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/040/M68KPagingMethod040.h"

#include <stdlib.h>
#include <string.h>

#include <AutoDeleter.h>

#include <arch_system_info.h>
#include <boot/kernel_args.h>
#include <int.h>
#include <thread.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include "paging/040/M68KPagingStructures040.h"
#include "paging/040/M68KVMTranslationMap040.h"
#include "paging/m68k_physical_page_mapper.h"
#include "paging/m68k_physical_page_mapper_large_memory.h"


#define TRACE_M68K_PAGING_METHOD_32_BIT
#ifdef TRACE_M68K_PAGING_METHOD_32_BIT
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


/* Slots per pool for the physical page mapper.
 * Since m68k page tables are smaller than 1 page, but we allocate them
 * at page granularity anyway, just go for this.
 */
#define SLOTS_PER_POOL	1024

using M68KLargePhysicalPageMapper::PhysicalPageSlot;

//XXX: make it a class member
//static page_table_entry sQueryDesc __attribute__ (( aligned (4) ));
//XXX:REMOVEME
//static addr_t sIOSpaceBase;

//XXX: stuff it in the class
#warning M68K:REMOVE
static inline void
init_page_root_entry(page_root_entry *entry)
{
	// DT_INVALID is 0
	*entry = DFL_ROOTENT_VAL;
}


static inline void
update_page_root_entry(page_root_entry *entry, page_root_entry *with)
{
	// update page directory entry atomically
	*entry = *with;
}


static inline void
init_page_directory_entry(page_directory_entry *entry)
{
	*entry = DFL_DIRENT_VAL;
}


static inline void
update_page_directory_entry(page_directory_entry *entry, page_directory_entry *with)
{
	// update page directory entry atomically
	*entry = *with;
}


static inline void
init_page_table_entry(page_table_entry *entry)
{
	*entry = DFL_PAGEENT_VAL;
}


static inline void
update_page_table_entry(page_table_entry *entry, page_table_entry *with)
{
	// update page table entry atomically
	// XXX: is it ?? (long desc?)
	*entry = *with;
}


static inline void
init_page_indirect_entry(page_indirect_entry *entry)
{
#warning M68K: is it correct ?
	*entry = DFL_PAGEENT_VAL;
}


static inline void
update_page_indirect_entry(page_indirect_entry *entry, page_indirect_entry *with)
{
	// update page table entry atomically
	// XXX: is it ?? (long desc?)
	*entry = *with;
}



// #pragma mark - M68KPagingMethod040::PhysicalPageSlotPool


struct M68KPagingMethod040::PhysicalPageSlotPool
	: M68KLargePhysicalPageMapper::PhysicalPageSlotPool {
public:
	virtual						~PhysicalPageSlotPool();

			status_t			InitInitial(kernel_args* args);
			status_t			InitInitialPostArea(kernel_args* args);

			void				Init(area_id dataArea, void* data,
									area_id virtualArea, addr_t virtualBase);

	virtual	status_t			AllocatePool(
									M68KLargePhysicalPageMapper
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


M68KPagingMethod040::PhysicalPageSlotPool
	M68KPagingMethod040::PhysicalPageSlotPool::sInitialPhysicalPagePool;


M68KPagingMethod040::PhysicalPageSlotPool::~PhysicalPageSlotPool()
{
}


status_t
M68KPagingMethod040::PhysicalPageSlotPool::InitInitial(kernel_args* args)
{
	// allocate a virtual address range for the pages to be mapped into
	addr_t virtualBase = vm_allocate_early(args, SLOTS_PER_POOL * B_PAGE_SIZE,
		0, 0, kPageTableAlignment);
	if (virtualBase == 0) {
		panic("LargeMemoryPhysicalPageMapper::Init(): Failed to reserve "
			"physical page pool space in virtual address space!");
		return B_ERROR;
	}

	// allocate memory for the page table and data
	size_t areaSize = B_PAGE_SIZE + sizeof(PhysicalPageSlot[SLOTS_PER_POOL]);
	page_table_entry* pageTable = (page_table_entry*)vm_allocate_early(args,
		areaSize, ~0L, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0);

	// prepare the page table
	_EarlyPreparePageTables(pageTable, virtualBase,
		SLOTS_PER_POOL * B_PAGE_SIZE);

	// init the pool structure and add the initial pool
	Init(-1, pageTable, -1, (addr_t)virtualBase);

	return B_OK;
}


status_t
M68KPagingMethod040::PhysicalPageSlotPool::InitInitialPostArea(
	kernel_args* args)
{
#warning M68K:WRITEME
	// create an area for the (already allocated) data
	size_t areaSize = B_PAGE_SIZE + sizeof(PhysicalPageSlot[SLOTS_PER_POOL]);
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
		SLOTS_PER_POOL * B_PAGE_SIZE, 0);
	if (area < B_OK) {
		panic("LargeMemoryPhysicalPageMapper::InitPostArea(): Failed to "
			"create area for physical page pool space.");
		return area;
	}
	fVirtualArea = area;

	return B_OK;
}


void
M68KPagingMethod040::PhysicalPageSlotPool::Init(area_id dataArea, void* data,
	area_id virtualArea, addr_t virtualBase)
{
	fDataArea = dataArea;
	fVirtualArea = virtualArea;
	fVirtualBase = virtualBase;
	fPageTable = (page_table_entry*)data;

	// init slot list
	fSlots = (PhysicalPageSlot*)(fPageTable + SLOTS_PER_POOL);
	addr_t slotAddress = virtualBase;
	for (int32 i = 0; i < SLOTS_PER_POOL; i++, slotAddress += B_PAGE_SIZE) {
		PhysicalPageSlot* slot = &fSlots[i];
		slot->next = slot + 1;
		slot->pool = this;
		slot->address = slotAddress;
	}

	fSlots[1023].next = NULL;
		// terminate list
}


void
M68KPagingMethod040::PhysicalPageSlotPool::Map(phys_addr_t physicalAddress,
	addr_t virtualAddress)
{
	page_table_entry& pte = fPageTable[
		(virtualAddress - fVirtualBase) / B_PAGE_SIZE];
	pte = TA_TO_PTEA(physicalAddress) | DT_PAGE
		| M68K_PTE_SUPERVISOR | M68K_PTE_GLOBAL;

	arch_cpu_invalidate_TLB_range(virtualAddress, virtualAddress);
}


status_t
M68KPagingMethod040::PhysicalPageSlotPool::AllocatePool(
	M68KLargePhysicalPageMapper::PhysicalPageSlotPool*& _pool)
{
	// create the pool structure
	PhysicalPageSlotPool* pool = new(std::nothrow) PhysicalPageSlotPool;
	if (pool == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<PhysicalPageSlotPool> poolDeleter(pool);

	// create an area that can contain the page table and the slot
	// structures
	size_t areaSize = B_PAGE_SIZE + sizeof(PhysicalPageSlot[SLOTS_PER_POOL]);
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
		&virtualBase, B_ANY_KERNEL_BLOCK_ADDRESS, SLOTS_PER_POOL * B_PAGE_SIZE,
		CREATE_AREA_PRIORITY_VIP);
	if (virtualArea < 0) {
		delete_area(dataArea);
		return virtualArea;
	}

	// prepare the page table
	memset(data, 0, B_PAGE_SIZE);

	// get the page table's physical address
	phys_addr_t physicalTable;
	M68KVMTranslationMap040* map = static_cast<M68KVMTranslationMap040*>(
		VMAddressSpace::Kernel()->TranslationMap());
	uint32 dummyFlags;
	cpu_status state = disable_interrupts();
	map->QueryInterrupt((addr_t)data, &physicalTable, &dummyFlags);
	restore_interrupts(state);

#warning M68K:FIXME: insert *all* page tables!
	panic("I'm lazy");
#if 0
	// put the page table into the page directory
	int32 index = (addr_t)virtualBase / (B_PAGE_SIZE * SLOTS_PER_POOL);
	page_directory_entry* entry
		= &map->PagingStructures040()->pgdir_virt[index];
	PutPageTableInPageDir(entry, physicalTable,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	M68KPagingStructures040::UpdateAllPageDirs(index, *entry);
#endif

	// init the pool structure
	pool->Init(dataArea, data, virtualArea, (addr_t)virtualBase);
	poolDeleter.Detach();
	_pool = pool;
	return B_OK;
}


// #pragma mark - M68KPagingMethod040


M68KPagingMethod040::M68KPagingMethod040()
	:
	//fPageHole(NULL),
	//fPageHolePageDir(NULL),
	fKernelPhysicalPageRoot(0),
	fKernelVirtualPageRoot(NULL),
	fPhysicalPageMapper(NULL),
	fKernelPhysicalPageMapper(NULL)
{
}


M68KPagingMethod040::~M68KPagingMethod040()
{
}


status_t
M68KPagingMethod040::Init(kernel_args* args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	TRACE("M68KPagingMethod040::Init(): entry\n");

#if 0//XXX:We might actually need this trick to support Milan
	// page hole set up in stage2
	fPageHole = (page_table_entry*)args->arch_args.page_hole;
	// calculate where the pgdir would be
	fPageHolePageDir = (page_directory_entry*)
		(((addr_t)args->arch_args.page_hole)
			+ (B_PAGE_SIZE * 1024 - B_PAGE_SIZE));
	// clear out the bottom 2 GB, unmap everything
	memset(fPageHolePageDir + FIRST_USER_PGDIR_ENT, 0,
		sizeof(page_directory_entry) * NUM_USER_PGDIR_ENTS);
#endif

	fKernelPhysicalPageRoot = (uint32)args->arch_args.phys_pgroot;
	fKernelVirtualPageRoot = (page_root_entry *)args->arch_args.vir_pgroot;

#ifdef TRACE_M68K_PAGING_METHOD_32_BIT
	//TRACE("page hole: %p, page dir: %p\n", fPageHole, fPageHolePageDir);
	TRACE("page root: %p (physical: %#" B_PRIx32 ")\n",
		fKernelVirtualPageRoot, fKernelPhysicalPageRoot);
#endif

	//sQueryDesc.type = DT_INVALID;

	M68KPagingStructures040::StaticInit();

	// create the initial pool for the physical page mapper
	PhysicalPageSlotPool* pool
		= new(&PhysicalPageSlotPool::sInitialPhysicalPagePool)
			PhysicalPageSlotPool;
	status_t error = pool->InitInitial(args);
	if (error != B_OK) {
		panic("M68KPagingMethod040::Init(): Failed to create initial pool "
			"for physical page mapper!");
		return error;
	}

	// create physical page mapper
	large_memory_physical_page_ops_init(args, pool, fPhysicalPageMapper,
		fKernelPhysicalPageMapper);
		// TODO: Select the best page mapper!

	TRACE("M68KPagingMethod040::Init(): done\n");

	*_physicalPageMapper = fPhysicalPageMapper;
	return B_OK;
}


status_t
M68KPagingMethod040::InitPostArea(kernel_args* args)
{
	TRACE("M68KPagingMethod040::InitPostArea(): entry\n");
	// now that the vm is initialized, create an area that represents
	// the page hole
	void *temp;
	status_t error;
	area_id area;

#if 0
	// unmap the page hole hack we were using before
	fKernelVirtualPageDirectory[1023] = 0;
	fPageHolePageDir = NULL;
	fPageHole = NULL;
#endif

	temp = (void*)fKernelVirtualPageRoot;
	area = create_area("kernel_pgdir", &temp, B_EXACT_ADDRESS, B_PAGE_SIZE,
		B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (area < B_OK)
		return area;

	error = PhysicalPageSlotPool::sInitialPhysicalPagePool
		.InitInitialPostArea(args);
	if (error != B_OK)
		return error;

	// this area is used for query_tmap_interrupt()
	// TODO: Note, this only works as long as all pages belong to the same
	//	page table, which is not yet enforced (or even tested)!
	// Note we don't support SMP which makes things simpler.
#if 0	//XXX: Do we need this anymore?
	area = vm_create_null_area(VMAddressSpace::KernelID(),
		"interrupt query pages", (void **)&queryPage, B_ANY_ADDRESS,
		B_PAGE_SIZE, 0);
	if (area < B_OK)
		return area;

	// insert the indirect descriptor in the tree so we can map the page we want from it.
	//XXX...
#endif

	TRACE("M68KPagingMethod040::InitPostArea(): done\n");
	return B_OK;
}


status_t
M68KPagingMethod040::CreateTranslationMap(bool kernel, VMTranslationMap** _map)
{
	M68KVMTranslationMap040* map;

	map = new(std::nothrow) M68KVMTranslationMap040;
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
M68KPagingMethod040::MapEarly(kernel_args* args, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	phys_addr_t (*get_free_page)(kernel_args*))
{
	// XXX horrible back door to map a page quickly regardless of translation
	// map object, etc. used only during VM setup.
	// uses a 'page hole' set up in the stage 2 bootloader. The page hole is
	// created by pointing one of the pgdir entries back at itself, effectively
	// mapping the contents of all of the 4MB of pagetables into a 4 MB region.
	// It's only used here, and is later unmapped.

	addr_t va = virtualAddress;
	phys_addr_t pa = physicalAddress;
	page_root_entry *pr = (page_root_entry *)fKernelPhysicalPageRoot;
	page_directory_entry *pd;
	page_table_entry *pt;
	addr_t tbl;
	uint32 index;
	uint32 i;
	TRACE("040::MapEarly: entry pa 0x%lx va 0x%lx\n", pa, va);

	// everything much simpler here because pa = va
	// thanks to transparent translation which hasn't been disabled yet

	index = VADDR_TO_PRENT(va);
	if (PRE_TYPE(pr[index]) != DT_ROOT) {
		unsigned aindex = index & ~(NUM_DIRTBL_PER_PAGE-1); /* aligned */
		TRACE("missing page root entry %d ai %d\n", index, aindex);
		tbl = get_free_page(args) * B_PAGE_SIZE;
		if (!tbl)
			return ENOMEM;
		TRACE("040::MapEarly: asked for free page for pgdir. 0x%lx\n", tbl);
		// zero-out
		memset((void *)tbl, 0, B_PAGE_SIZE);
		// for each pgdir on the allocated page:
		for (i = 0; i < NUM_DIRTBL_PER_PAGE; i++) {
			PutPageDirInPageRoot(&pr[aindex + i], tbl, attributes);
			//TRACE("inserting tbl @ %p as %08x pr[%d] %08x\n", tbl, TA_TO_PREA(tbl), aindex + i, *(uint32 *)apr);
			// clear the table
			//TRACE("clearing table[%d]\n", i);
			pd = (page_directory_entry *)tbl;
			for (int32 j = 0; j < NUM_DIRENT_PER_TBL; j++)
				pd[j] = DFL_DIRENT_VAL;
			tbl += SIZ_DIRTBL;
		}
	}
	pd = (page_directory_entry *)PRE_TO_TA(pr[index]);

	index = VADDR_TO_PDENT(va);
	if (PDE_TYPE(pd[index]) != DT_DIR) {
		unsigned aindex = index & ~(NUM_PAGETBL_PER_PAGE-1); /* aligned */
		TRACE("missing page dir entry %d ai %d\n", index, aindex);
		tbl = get_free_page(args) * B_PAGE_SIZE;
		if (!tbl)
			return ENOMEM;
		TRACE("early_map: asked for free page for pgtable. 0x%lx\n", tbl);
		// zero-out
		memset((void *)tbl, 0, B_PAGE_SIZE);
		// for each pgdir on the allocated page:
		for (i = 0; i < NUM_PAGETBL_PER_PAGE; i++) {
			PutPageTableInPageDir(&pd[aindex + i], tbl, attributes);
			// clear the table
			//TRACE("clearing table[%d]\n", i);
			pt = (page_table_entry *)tbl;
			for (int32 j = 0; j < NUM_PAGEENT_PER_TBL; j++)
				pt[j] = DFL_PAGEENT_VAL;
			tbl += SIZ_PAGETBL;
		}
	}
	pt = (page_table_entry *)PDE_TO_TA(pd[index]);

	index = VADDR_TO_PTENT(va);
	// now, fill in the pentry
	PutPageTableEntryInTable(&pt[index],
		physicalAddress, attributes, 0, IS_KERNEL_ADDRESS(virtualAddress));

	arch_cpu_invalidate_TLB_range(va, va);

	return B_OK;



#if 0
	// check to see if a page table exists for this range
	int index = VADDR_TO_PDENT(virtualAddress);
	if ((fPageHolePageDir[index] & M68K_PDE_PRESENT) == 0) {
		phys_addr_t pgtable;
		page_directory_entry *e;
		// we need to allocate a pgtable
		pgtable = get_free_page(args);
		// pgtable is in pages, convert to physical address
		pgtable *= B_PAGE_SIZE;

		TRACE("M68KPagingMethod040::MapEarly(): asked for free page for "
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
		(fPageHole[virtualAddress / B_PAGE_SIZE] & M68K_PTE_PRESENT) == 0,
		"virtual address: %#" B_PRIxADDR ", pde: %#" B_PRIx32
		", existing pte: %#" B_PRIx32, virtualAddress, fPageHolePageDir[index],
		fPageHole[virtualAddress / B_PAGE_SIZE]);

#endif

	return B_OK;
}


bool
M68KPagingMethod040::IsKernelPageAccessible(addr_t virtualAddress,
	uint32 protection)
{
#warning M68K: WRITEME
	return false;
}


void
M68KPagingMethod040::SetPageRoot(uint32 pageRoot)
{
#warning M68K:TODO:override this for 060
	uint32 rp;
	rp = pageRoot & ~((1 << 9) - 1);

	asm volatile(          \
		"movec %0,%%srp\n" \
		"movec %0,%%urp\n" \
		: : "d"(rp));
}


/*static*/ void
M68KPagingMethod040::PutPageDirInPageRoot(page_root_entry* entry,
	phys_addr_t pgdirPhysical, uint32 attributes)
{
	*entry = TA_TO_PREA(pgdirPhysical)
		| DT_DIR;	// it's a page directory entry

	// ToDo: we ignore the attributes of the page table - for compatibility
	//	with BeOS we allow having user accessible areas in the kernel address
	//	space. This is currently being used by some drivers, mainly for the
	//	frame buffer. Our current real time data implementation makes use of
	//	this fact, too.
	//	We might want to get rid of this possibility one day, especially if
	//	we intend to port it to a platform that does not support this.
	//table.user = 1;
	//table.rw = 1;
}


/*static*/ void
M68KPagingMethod040::PutPageTableInPageDir(page_directory_entry* entry,
	phys_addr_t pgtablePhysical, uint32 attributes)
{
	*entry = TA_TO_PDEA(pgtablePhysical)
		| DT_DIR;	// it's a page directory entry

}


/*static*/ void
M68KPagingMethod040::PutPageTableEntryInTable(page_table_entry* entry,
	phys_addr_t physicalAddress, uint32 attributes, uint32 memoryType,
	bool globalPage)
{
	page_table_entry page = TA_TO_PTEA(physicalAddress)
		| DT_PAGE
#ifdef PAGE_HAS_GLOBAL_BIT
		| (globalPage ? M68K_PTE_GLOBAL : 0)
#endif
		| MemoryTypeToPageTableEntryFlags(memoryType);

	// if the page is user accessible, it's automatically
	// accessible in kernel space, too (but with the same
	// protection)
	if ((attributes & B_USER_PROTECTION) == 0) {
		page |= M68K_PTE_SUPERVISOR;
		if ((attributes & B_KERNEL_WRITE_AREA) == 0)
			page |= M68K_PTE_READONLY;
	} else if ((attributes & B_WRITE_AREA) == 0)
		page |= M68K_PTE_READONLY;


	// put it in the page table
	*(volatile page_table_entry*)entry = page;
}

/*static*/ void
M68KPagingMethod040::_EarlyPreparePageTables(page_table_entry* pageTables,
	addr_t address, size_t size)
{
	memset(pageTables, 0, B_PAGE_SIZE *
		(size / (B_PAGE_SIZE * NUM_PAGEENT_PER_TBL * NUM_PAGETBL_PER_PAGE)));

	// put the array of pgtables directly into the kernel pagedir
	// these will be wired and kept mapped into virtual space to be easy to get
	// to
	// note the bootloader allocates all page directories for us
	// as a contiguous block.
	// we also still have transparent translation enabled, va==pa.
	{
		size_t index;
		addr_t virtualTable = (addr_t)pageTables;
		page_root_entry *pr
			= M68KPagingMethod040::Method()->fKernelVirtualPageRoot;
		page_directory_entry *pd;
		page_directory_entry *e;

		for (size_t i = 0; i < (size / (B_PAGE_SIZE * NUM_PAGEENT_PER_TBL));
				i++, virtualTable += SIZ_PAGETBL) {
			// early_query handles non-page-aligned addresses
			phys_addr_t physicalTable = 0;
			_EarlyQuery(virtualTable, &physicalTable);
			index = VADDR_TO_PRENT(address) + i / NUM_DIRENT_PER_TBL;
			pd = (page_directory_entry *)PRE_TO_TA(pr[index]);
			e = &pd[(VADDR_TO_PDENT(address) + i) % NUM_DIRENT_PER_TBL];
			PutPageTableInPageDir(e, physicalTable,
				B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		}
	}
}


//! TODO: currently assumes this translation map is active
/*static*/ status_t
M68KPagingMethod040::_EarlyQuery(addr_t virtualAddress,
	phys_addr_t *_physicalAddress)
{

	M68KPagingMethod040* method = M68KPagingMethod040::Method();

	page_root_entry *pr = method->fKernelVirtualPageRoot;
	page_directory_entry *pd;
	page_indirect_entry *pi;
	page_table_entry *pt;
	addr_t pa;
	int32 index;
	status_t err = B_ERROR;	// no pagetable here
	TRACE("%s(%p,)\n", __FUNCTION__, virtualAddress);

	// this is used before the vm is fully up, it uses the
	// transparent translation of the first 256MB
	// as set up by the bootloader.

	index = VADDR_TO_PRENT(virtualAddress);
	TRACE("%s: pr[%d].type %d\n", __FUNCTION__, index, PRE_TYPE(pr[index]));
	if (pr && PRE_TYPE(pr[index]) == DT_ROOT) {
		pa = PRE_TO_TA(pr[index]);
		// pa == va when in TT
		// and no need to fiddle with cache
		pd = (page_directory_entry *)pa;

		index = VADDR_TO_PDENT(virtualAddress);
		TRACE("%s: pd[%d].type %d\n", __FUNCTION__, index,
				pd?(PDE_TYPE(pd[index])):-1);
		if (pd && PDE_TYPE(pd[index]) == DT_DIR) {
			pa = PDE_TO_TA(pd[index]);
			pt = (page_table_entry *)pa;

			index = VADDR_TO_PTENT(virtualAddress);
			TRACE("%s: pt[%d].type %d\n", __FUNCTION__, index,
					pt?(PTE_TYPE(pt[index])):-1);
			if (pt && PTE_TYPE(pt[index]) == DT_INDIRECT) {
				pi = (page_indirect_entry *)pt;
				pa = PIE_TO_TA(pi[index]);
				pt = (page_table_entry *)pa;
				index = 0; // single descriptor
			}

			if (pt && PIE_TYPE(pt[index]) == DT_PAGE) {
				*_physicalAddress = PTE_TO_PA(pt[index]);
				// we should only be passed page va, but just in case.
				*_physicalAddress += virtualAddress % B_PAGE_SIZE;
				err = B_OK;
			}
		}
	}

	return err;

#if 0

	int index = VADDR_TO_PDENT(virtualAddress);
	if ((method->PageHolePageDir()[index] & M68K_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_ERROR;
	}

	page_table_entry* entry = method->PageHole() + virtualAddress / B_PAGE_SIZE;
	if ((*entry & M68K_PTE_PRESENT) == 0) {
		// page mapping not valid
		return B_ERROR;
	}

	*_physicalAddress = *entry & M68K_PTE_ADDRESS_MASK;
	return B_OK;
#endif
}
