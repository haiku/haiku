/*
 * Copyright 2008-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "paging/460/PPCPagingMethod460.h"

#include <stdlib.h>
#include <string.h>

#include <AutoDeleter.h>

#include <arch/cpu.h>
#include <arch_mmu.h>
#include <arch_system_info.h>
#include <boot/kernel_args.h>
#include <int.h>
#include <thread.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include "paging/460/PPCPagingStructures460.h"
#include "paging/460/PPCVMTranslationMap460.h"
#include "generic_vm_physical_page_mapper.h"
#include "generic_vm_physical_page_ops.h"
#include "GenericVMPhysicalPageMapper.h"


//#define TRACE_PPC_PAGING_METHOD_460
#ifdef TRACE_PPC_PAGING_METHOD_460
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif

// 64 MB of iospace
#define IOSPACE_SIZE (64*1024*1024)
// We only have small (4 KB) pages. The only reason for choosing greater chunk
// size is to keep the waste of memory limited, since the generic page mapper
// allocates structures per physical/virtual chunk.
// TODO: Implement a page mapper more suitable for small pages!
#define IOSPACE_CHUNK_SIZE (16 * B_PAGE_SIZE)

static addr_t sIOSpaceBase;


static status_t
map_iospace_chunk(addr_t va, phys_addr_t pa, uint32 flags)
{
	pa &= ~(B_PAGE_SIZE - 1); // make sure it's page aligned
	va &= ~(B_PAGE_SIZE - 1); // make sure it's page aligned
	if (va < sIOSpaceBase || va >= (sIOSpaceBase + IOSPACE_SIZE))
		panic("map_iospace_chunk: passed invalid va 0x%lx\n", va);

	// map the pages
	return ppc_map_address_range(va, pa, IOSPACE_CHUNK_SIZE);
}


// #pragma mark - PPCPagingMethod460


PPCPagingMethod460::PPCPagingMethod460()
/*
	:
	fPageHole(NULL),
	fPageHolePageDir(NULL),
	fKernelPhysicalPageDirectory(0),
	fKernelVirtualPageDirectory(NULL),
	fPhysicalPageMapper(NULL),
	fKernelPhysicalPageMapper(NULL)
*/
{
}


PPCPagingMethod460::~PPCPagingMethod460()
{
}


status_t
PPCPagingMethod460::Init(kernel_args* args,
	VMPhysicalPageMapper** _physicalPageMapper)
{
	TRACE("PPCPagingMethod460::Init(): entry\n");

	fPageTable = (page_table_entry_group *)args->arch_args.page_table.start;
	fPageTableSize = args->arch_args.page_table.size;
	fPageTableHashMask = fPageTableSize / sizeof(page_table_entry_group) - 1;

	// init physical page mapper
	status_t error = generic_vm_physical_page_mapper_init(args,
		map_iospace_chunk, &sIOSpaceBase, IOSPACE_SIZE, IOSPACE_CHUNK_SIZE);
	if (error != B_OK)
		return error;

	new(&fPhysicalPageMapper) GenericVMPhysicalPageMapper;

	*_physicalPageMapper = &fPhysicalPageMapper;
	return B_OK;

#if 0//X86
	fKernelPhysicalPageDirectory = args->arch_args.phys_pgdir;
	fKernelVirtualPageDirectory = (page_directory_entry*)(addr_t)
		args->arch_args.vir_pgdir;

#ifdef TRACE_PPC_PAGING_METHOD_460
	TRACE("page hole: %p, page dir: %p\n", fPageHole, fPageHolePageDir);
	TRACE("page dir: %p (physical: %#" B_PRIx32 ")\n",
		fKernelVirtualPageDirectory, fKernelPhysicalPageDirectory);
#endif

	PPCPagingStructures460::StaticInit();

	// create the initial pool for the physical page mapper
	PhysicalPageSlotPool* pool
		= new(&PhysicalPageSlotPool::sInitialPhysicalPagePool)
			PhysicalPageSlotPool;
	status_t error = pool->InitInitial(args);
	if (error != B_OK) {
		panic("PPCPagingMethod460::Init(): Failed to create initial pool "
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

	TRACE("PPCPagingMethod460::Init(): done\n");

	*_physicalPageMapper = fPhysicalPageMapper;
	return B_OK;
#endif
}


status_t
PPCPagingMethod460::InitPostArea(kernel_args* args)
{

	// If the page table doesn't lie within the kernel address space, we
	// remap it.
	if (!IS_KERNEL_ADDRESS(fPageTable)) {
		addr_t newAddress = (addr_t)fPageTable;
		status_t error = ppc_remap_address_range(&newAddress, fPageTableSize,
			false);
		if (error != B_OK) {
			panic("arch_vm_translation_map_init_post_area(): Failed to remap "
				"the page table!");
			return error;
		}

		// set the new page table address
		addr_t oldVirtualBase = (addr_t)(fPageTable);
		fPageTable = (page_table_entry_group*)newAddress;

		// unmap the old pages
		ppc_unmap_address_range(oldVirtualBase, fPageTableSize);

// TODO: We should probably map the page table via BAT. It is relatively large,
// and due to being a hash table the access patterns might look sporadic, which
// certainly isn't to the liking of the TLB.
	}

	// create an area to cover the page table
	fPageTableArea = create_area("page_table", (void **)&fPageTable, B_EXACT_ADDRESS,
		fPageTableSize, B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	// init physical page mapper
	status_t error = generic_vm_physical_page_mapper_init_post_area(args);
	if (error != B_OK)
		return error;

	return B_OK;

#if 0//X86
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
#endif//X86
}


status_t
PPCPagingMethod460::CreateTranslationMap(bool kernel, VMTranslationMap** _map)
{
	PPCVMTranslationMap460* map = new(std::nothrow) PPCVMTranslationMap460;
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
PPCPagingMethod460::MapEarly(kernel_args* args, addr_t virtualAddress,
	phys_addr_t physicalAddress, uint8 attributes,
	page_num_t (*get_free_page)(kernel_args*))
{
	uint32 virtualSegmentID = get_sr((void *)virtualAddress) & 0xffffff;

	uint32 hash = page_table_entry::PrimaryHash(virtualSegmentID, (uint32)virtualAddress);
	page_table_entry_group *group = &fPageTable[hash & fPageTableHashMask];

	for (int32 i = 0; i < 8; i++) {
		// 8 entries in a group
		if (group->entry[i].valid)
			continue;

		FillPageTableEntry(&group->entry[i], virtualSegmentID,
			virtualAddress, physicalAddress, PTE_READ_WRITE, 0, false);
		return B_OK;
	}

	hash = page_table_entry::SecondaryHash(hash);
	group = &fPageTable[hash & fPageTableHashMask];

	for (int32 i = 0; i < 8; i++) {
		if (group->entry[i].valid)
			continue;

		FillPageTableEntry(&group->entry[i], virtualSegmentID,
			virtualAddress, physicalAddress, PTE_READ_WRITE, 0, true);
		return B_OK;
	}

	return B_ERROR;
}


bool
PPCPagingMethod460::IsKernelPageAccessible(addr_t virtualAddress,
	uint32 protection)
{
	// TODO:factor out to baseclass
	VMAddressSpace *addressSpace = VMAddressSpace::Kernel();

//XXX:
//	PPCVMTranslationMap* map = static_cast<PPCVMTranslationMap*>(
//		addressSpace->TranslationMap());
//	VMTranslationMap* map = addressSpace->TranslationMap();
	PPCVMTranslationMap460* map = static_cast<PPCVMTranslationMap460*>(
		addressSpace->TranslationMap());

	phys_addr_t physicalAddress;
	uint32 flags;
	if (map->Query(virtualAddress, &physicalAddress, &flags) != B_OK)
		return false;

	if ((flags & PAGE_PRESENT) == 0)
		return false;

	// present means kernel-readable, so check for writable
	return (protection & B_KERNEL_WRITE_AREA) == 0
		|| (flags & B_KERNEL_WRITE_AREA) != 0;
}


void
PPCPagingMethod460::FillPageTableEntry(page_table_entry *entry,
	uint32 virtualSegmentID, addr_t virtualAddress, phys_addr_t physicalAddress,
	uint8 protection, uint32 memoryType, bool secondaryHash)
{
	// lower 32 bit - set at once
	entry->physical_page_number = physicalAddress / B_PAGE_SIZE;
	entry->_reserved0 = 0;
	entry->referenced = false;
	entry->changed = false;
	entry->write_through = (memoryType == B_UNCACHED_MEMORY) || (memoryType == B_WRITE_THROUGH_MEMORY);
	entry->caching_inhibited = (memoryType == B_UNCACHED_MEMORY);
	entry->memory_coherent = false;
	entry->guarded = false;
	entry->_reserved1 = 0;
	entry->page_protection = protection & 0x3;
	eieio();
		// we need to make sure that the lower 32 bit were
		// already written when the entry becomes valid

	// upper 32 bit
	entry->virtual_segment_id = virtualSegmentID;
	entry->secondary_hash = secondaryHash;
	entry->abbr_page_index = (virtualAddress >> 22) & 0x3f;
	entry->valid = true;

	ppc_sync();
}


#if 0//X86
/*static*/ void
PPCPagingMethod460::PutPageTableInPageDir(page_directory_entry* entry,
	phys_addr_t pgtablePhysical, uint32 attributes)
{
	*entry = (pgtablePhysical & PPC_PDE_ADDRESS_MASK)
		| PPC_PDE_PRESENT
		| PPC_PDE_WRITABLE
		| PPC_PDE_USER;
		// TODO: we ignore the attributes of the page table - for compatibility
		// with BeOS we allow having user accessible areas in the kernel address
		// space. This is currently being used by some drivers, mainly for the
		// frame buffer. Our current real time data implementation makes use of
		// this fact, too.
		// We might want to get rid of this possibility one day, especially if
		// we intend to port it to a platform that does not support this.
}


/*static*/ void
PPCPagingMethod460::PutPageTableEntryInTable(page_table_entry* entry,
	phys_addr_t physicalAddress, uint32 attributes, uint32 memoryType,
	bool globalPage)
{
	page_table_entry page = (physicalAddress & PPC_PTE_ADDRESS_MASK)
		| PPC_PTE_PRESENT | (globalPage ? PPC_PTE_GLOBAL : 0)
		| MemoryTypeToPageTableEntryFlags(memoryType);

	// if the page is user accessible, it's automatically
	// accessible in kernel space, too (but with the same
	// protection)
	if ((attributes & B_USER_PROTECTION) != 0) {
		page |= PPC_PTE_USER;
		if ((attributes & B_WRITE_AREA) != 0)
			page |= PPC_PTE_WRITABLE;
	} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		page |= PPC_PTE_WRITABLE;

	// put it in the page table
	*(volatile page_table_entry*)entry = page;
}


/*static*/ void
PPCPagingMethod460::_EarlyPreparePageTables(page_table_entry* pageTables,
	addr_t address, size_t size)
{
	memset(pageTables, 0, B_PAGE_SIZE * (size / (B_PAGE_SIZE * 1024)));

	// put the array of pgtables directly into the kernel pagedir
	// these will be wired and kept mapped into virtual space to be easy to get
	// to
	{
		addr_t virtualTable = (addr_t)pageTables;

		page_directory_entry* pageHolePageDir
			= PPCPagingMethod460::Method()->PageHolePageDir();

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
PPCPagingMethod460::_EarlyQuery(addr_t virtualAddress,
	phys_addr_t *_physicalAddress)
{
	PPCPagingMethod460* method = PPCPagingMethod460::Method();
	int index = VADDR_TO_PDENT(virtualAddress);
	if ((method->PageHolePageDir()[index] & PPC_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_ERROR;
	}

	page_table_entry* entry = method->PageHole() + virtualAddress / B_PAGE_SIZE;
	if ((*entry & PPC_PTE_PRESENT) == 0) {
		// page mapping not valid
		return B_ERROR;
	}

	*_physicalAddress = *entry & PPC_PTE_ADDRESS_MASK;
	return B_OK;
}
#endif
