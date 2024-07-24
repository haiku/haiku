/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*	(bonefish) Some explanatory words on how address translation is implemented
	for the 32 bit PPC architecture.

	I use the address type nomenclature as used in the PPC architecture
	specs, i.e.
	- effective address: An address as used by program instructions, i.e.
	  that's what elsewhere (e.g. in the VM implementation) is called
	  virtual address.
	- virtual address: An intermediate address computed from the effective
	  address via the segment registers.
	- physical address: An address referring to physical storage.

	The hardware translates an effective address to a physical address using
	either of two mechanisms: 1) Block Address Translation (BAT) or
	2) segment + page translation. The first mechanism does this directly
	using two sets (for data/instructions) of special purpose registers.
	The latter mechanism is of more relevance here, though:

	effective address (32 bit):	     [ 0 ESID  3 | 4  PIX 19 | 20 Byte 31 ]
								           |           |            |
							     (segment registers)   |            |
									       |           |            |
	virtual address (52 bit):   [ 0      VSID 23 | 24 PIX 39 | 40 Byte 51 ]
	                            [ 0             VPN       39 | 40 Byte 51 ]
								                 |                  |
										   (page table)             |
											     |                  |
	physical address (32 bit):       [ 0        PPN       19 | 20 Byte 31 ]


	ESID: Effective Segment ID
	VSID: Virtual Segment ID
	PIX:  Page Index
	VPN:  Virtual Page Number
	PPN:  Physical Page Number


	Unlike on x86 we can't just switch the context to another team by just
	setting a register to another page directory, since we only have one
	page table containing both kernel and user address mappings. Instead we
	map the effective address space of kernel and *all* teams
	non-intersectingly into the virtual address space (which fortunately is
	20 bits wider), and use the segment registers to select the section of
	the virtual address space for the current team. Half of the 16 segment
	registers (8 - 15) map the kernel addresses, so they remain unchanged.

	The range of the virtual address space a team's effective address space
	is mapped to is defined by its PPCVMTranslationMap::fVSIDBase,
	which is the first of the 8 successive VSID values used for the team.

	Which fVSIDBase values are already taken is defined by the set bits in
	the bitmap sVSIDBaseBitmap.


	TODO:
	* If we want to continue to use the OF services, we would need to add
	  its address mappings to the kernel space. Unfortunately some stuff
	  (especially RAM) is mapped in an address range without the kernel
	  address space. We probably need to map those into each team's address
	  space as kernel read/write areas.
	* The current locking scheme is insufficient. The page table is a resource
	  shared by all teams. We need to synchronize access to it. Probably via a
	  spinlock.
 */

#include "paging/classic/PPCVMTranslationMapClassic.h"

#include <stdlib.h>
#include <string.h>

#include <arch/cpu.h>
#include <arch_mmu.h>
#include <int.h>
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

#include "paging/classic/PPCPagingMethodClassic.h"
#include "paging/classic/PPCPagingStructuresClassic.h"
#include "generic_vm_physical_page_mapper.h"
#include "generic_vm_physical_page_ops.h"
#include "GenericVMPhysicalPageMapper.h"


//#define TRACE_PPC_VM_TRANSLATION_MAP_CLASSIC
#ifdef TRACE_PPC_VM_TRANSLATION_MAP_CLASSIC
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


// The VSID is a 24 bit number. The lower three bits are defined by the
// (effective) segment number, which leaves us with a 21 bit space of
// VSID bases (= 2 * 1024 * 1024).
#define MAX_VSID_BASES (B_PAGE_SIZE * 8)
static uint32 sVSIDBaseBitmap[MAX_VSID_BASES / (sizeof(uint32) * 8)];
static spinlock sVSIDBaseBitmapLock;

#define VSID_BASE_SHIFT 3
#define VADDR_TO_VSID(vsidBase, vaddr) (vsidBase + ((vaddr) >> 28))


// #pragma mark -


PPCVMTranslationMapClassic::PPCVMTranslationMapClassic()
	:
	fPagingStructures(NULL)
{
}


PPCVMTranslationMapClassic::~PPCVMTranslationMapClassic()
{
	if (fPagingStructures == NULL)
		return;

#if 0//X86
	if (fPageMapper != NULL)
		fPageMapper->Delete();
#endif

	if (fMapCount > 0) {
		panic("vm_translation_map.destroy_tmap: map %p has positive map count %ld\n",
			this, fMapCount);
	}

	// mark the vsid base not in use
	int baseBit = fVSIDBase >> VSID_BASE_SHIFT;
	atomic_and((int32 *)&sVSIDBaseBitmap[baseBit / 32],
			~(1 << (baseBit % 32)));

#if 0//X86
	if (fPagingStructures->pgdir_virt != NULL) {
		// cycle through and free all of the user space pgtables
		for (uint32 i = VADDR_TO_PDENT(USER_BASE);
				i <= VADDR_TO_PDENT(USER_BASE + (USER_SIZE - 1)); i++) {
			if ((fPagingStructures->pgdir_virt[i] & PPC_PDE_PRESENT) != 0) {
				addr_t address = fPagingStructures->pgdir_virt[i]
					& PPC_PDE_ADDRESS_MASK;
				vm_page* page = vm_lookup_page(address / B_PAGE_SIZE);
				if (!page)
					panic("destroy_tmap: didn't find pgtable page\n");
				DEBUG_PAGE_ACCESS_START(page);
				vm_page_set_state(page, PAGE_STATE_FREE);
			}
		}
	}
#endif

	fPagingStructures->RemoveReference();
}


status_t
PPCVMTranslationMapClassic::Init(bool kernel)
{
	TRACE("PPCVMTranslationMapClassic::Init()\n");

	PPCVMTranslationMap::Init(kernel);

	cpu_status state = disable_interrupts();
	acquire_spinlock(&sVSIDBaseBitmapLock);

	// allocate a VSID base for this one
	if (kernel) {
		// The boot loader has set up the segment registers for identical
		// mapping. Two VSID bases are reserved for the kernel: 0 and 8. The
		// latter one for mapping the kernel address space (0x80000000...), the
		// former one for the lower addresses required by the Open Firmware
		// services.
		fVSIDBase = 0;
		sVSIDBaseBitmap[0] |= 0x3;
	} else {
		int i = 0;

		while (i < MAX_VSID_BASES) {
			if (sVSIDBaseBitmap[i / 32] == 0xffffffff) {
				i += 32;
				continue;
			}
			if ((sVSIDBaseBitmap[i / 32] & (1 << (i % 32))) == 0) {
				// we found it
				sVSIDBaseBitmap[i / 32] |= 1 << (i % 32);
				break;
			}
			i++;
		}
		if (i >= MAX_VSID_BASES)
			panic("vm_translation_map_create: out of VSID bases\n");
		fVSIDBase = i << VSID_BASE_SHIFT;
	}

	release_spinlock(&sVSIDBaseBitmapLock);
	restore_interrupts(state);

	fPagingStructures = new(std::nothrow) PPCPagingStructuresClassic;
	if (fPagingStructures == NULL)
		return B_NO_MEMORY;

	PPCPagingMethodClassic* method = PPCPagingMethodClassic::Method();

	if (!kernel) {
		// user
#if 0//X86
		// allocate a physical page mapper
		status_t error = method->PhysicalPageMapper()
			->CreateTranslationMapPhysicalPageMapper(&fPageMapper);
		if (error != B_OK)
			return error;
#endif
#if 0//X86
		// allocate the page directory
		page_directory_entry* virtualPageDir = (page_directory_entry*)memalign(
			B_PAGE_SIZE, B_PAGE_SIZE);
		if (virtualPageDir == NULL)
			return B_NO_MEMORY;

		// look up the page directory's physical address
		phys_addr_t physicalPageDir;
		vm_get_page_mapping(VMAddressSpace::KernelID(),
			(addr_t)virtualPageDir, &physicalPageDir);
#endif

		fPagingStructures->Init(/*NULL, 0,
			method->KernelVirtualPageDirectory()*/method->PageTable());
	} else {
		// kernel
#if 0//X86
		// get the physical page mapper
		fPageMapper = method->KernelPhysicalPageMapper();
#endif

		// we already know the kernel pgdir mapping
		fPagingStructures->Init(/*method->KernelVirtualPageDirectory(),
			method->KernelPhysicalPageDirectory(), NULL*/method->PageTable());
	}

	return B_OK;
}


void
PPCVMTranslationMapClassic::ChangeASID()
{
// this code depends on the kernel being at 0x80000000, fix if we change that
#if KERNEL_BASE != 0x80000000
#error fix me
#endif
	int vsidBase = VSIDBase();

	isync();	// synchronize context
	asm("mtsr	0,%0" : : "g"(vsidBase));
	asm("mtsr	1,%0" : : "g"(vsidBase + 1));
	asm("mtsr	2,%0" : : "g"(vsidBase + 2));
	asm("mtsr	3,%0" : : "g"(vsidBase + 3));
	asm("mtsr	4,%0" : : "g"(vsidBase + 4));
	asm("mtsr	5,%0" : : "g"(vsidBase + 5));
	asm("mtsr	6,%0" : : "g"(vsidBase + 6));
	asm("mtsr	7,%0" : : "g"(vsidBase + 7));
	isync();	// synchronize context
}


page_table_entry *
PPCVMTranslationMapClassic::LookupPageTableEntry(addr_t virtualAddress)
{
	// lookup the vsid based off the va
	uint32 virtualSegmentID = VADDR_TO_VSID(fVSIDBase, virtualAddress);

//	dprintf("vm_translation_map.lookup_page_table_entry: vsid %ld, va 0x%lx\n", virtualSegmentID, virtualAddress);

	PPCPagingMethodClassic* m = PPCPagingMethodClassic::Method();

	// Search for the page table entry using the primary hash value

	uint32 hash = page_table_entry::PrimaryHash(virtualSegmentID, virtualAddress);
	page_table_entry_group *group = &(m->PageTable())[hash & m->PageTableHashMask()];

	for (int i = 0; i < 8; i++) {
		page_table_entry *entry = &group->entry[i];

		if (entry->virtual_segment_id == virtualSegmentID
			&& entry->secondary_hash == false
			&& entry->abbr_page_index == ((virtualAddress >> 22) & 0x3f))
			return entry;
	}

	// didn't find it, try the secondary hash value

	hash = page_table_entry::SecondaryHash(hash);
	group = &(m->PageTable())[hash & m->PageTableHashMask()];

	for (int i = 0; i < 8; i++) {
		page_table_entry *entry = &group->entry[i];

		if (entry->virtual_segment_id == virtualSegmentID
			&& entry->secondary_hash == true
			&& entry->abbr_page_index == ((virtualAddress >> 22) & 0x3f))
			return entry;
	}

	return NULL;
}


bool
PPCVMTranslationMapClassic::RemovePageTableEntry(addr_t virtualAddress)
{
	page_table_entry *entry = LookupPageTableEntry(virtualAddress);
	if (entry == NULL)
		return false;

	entry->valid = 0;
	ppc_sync();
	tlbie(virtualAddress);
	eieio();
	tlbsync();
	ppc_sync();

	return true;
}


size_t
PPCVMTranslationMapClassic::MaxPagesNeededToMap(addr_t start, addr_t end) const
{
	return 0;
}


status_t
PPCVMTranslationMapClassic::Map(addr_t virtualAddress,
	phys_addr_t physicalAddress, uint32 attributes,
	uint32 memoryType, vm_page_reservation* reservation)
{
	TRACE("map_tmap: entry pa 0x%lx va 0x%lx\n", pa, va);

	// lookup the vsid based off the va
	uint32 virtualSegmentID = VADDR_TO_VSID(fVSIDBase, virtualAddress);
	uint32 protection = 0;

	// ToDo: check this
	// all kernel mappings are R/W to supervisor code
	if (attributes & (B_READ_AREA | B_WRITE_AREA))
		protection = (attributes & B_WRITE_AREA) ? PTE_READ_WRITE : PTE_READ_ONLY;

	//dprintf("vm_translation_map.map_tmap: vsid %d, pa 0x%lx, va 0x%lx\n", vsid, pa, va);

	PPCPagingMethodClassic* m = PPCPagingMethodClassic::Method();

	// Search for a free page table slot using the primary hash value
	uint32 hash = page_table_entry::PrimaryHash(virtualSegmentID, virtualAddress);
	page_table_entry_group *group = &(m->PageTable())[hash & m->PageTableHashMask()];

	for (int i = 0; i < 8; i++) {
		page_table_entry *entry = &group->entry[i];

		if (entry->valid)
			continue;

		m->FillPageTableEntry(entry, virtualSegmentID, virtualAddress,
			physicalAddress, protection, memoryType, false);
		fMapCount++;
		return B_OK;
	}

	// Didn't found one, try the secondary hash value

	hash = page_table_entry::SecondaryHash(hash);
	group = &(m->PageTable())[hash & m->PageTableHashMask()];

	for (int i = 0; i < 8; i++) {
		page_table_entry *entry = &group->entry[i];

		if (entry->valid)
			continue;

		m->FillPageTableEntry(entry, virtualSegmentID, virtualAddress,
			physicalAddress, protection, memoryType, false);
		fMapCount++;
		return B_OK;
	}

	panic("vm_translation_map.map_tmap: hash table full\n");
	return B_ERROR;

#if 0//X86
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
	if ((pd[index] & PPC_PDE_PRESENT) == 0) {
		phys_addr_t pgtable;
		vm_page *page;

		// we need to allocate a pgtable
		page = vm_page_allocate_page(reservation,
			PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);

		DEBUG_PAGE_ACCESS_END(page);

		pgtable = (phys_addr_t)page->physical_page_number * B_PAGE_SIZE;

		TRACE("map_tmap: asked for free page for pgtable. 0x%lx\n", pgtable);

		// put it in the pgdir
		PPCPagingMethodClassic::PutPageTableInPageDir(&pd[index], pgtable,
			attributes
				| ((attributes & B_USER_PROTECTION) != 0
						? B_WRITE_AREA : B_KERNEL_WRITE_AREA));

		// update any other page directories, if it maps kernel space
		if (index >= FIRST_KERNEL_PGDIR_ENT
			&& index < (FIRST_KERNEL_PGDIR_ENT + NUM_KERNEL_PGDIR_ENTS)) {
			PPCPagingStructuresClassic::UpdateAllPageDirs(index, pd[index]);
		}

		fMapCount++;
	}

	// now, fill in the pentry
	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & PPC_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(va);

	ASSERT_PRINT((pt[index] & PPC_PTE_PRESENT) == 0,
		"virtual address: %#" B_PRIxADDR ", existing pte: %#" B_PRIx32, va,
		pt[index]);

	PPCPagingMethodClassic::PutPageTableEntryInTable(&pt[index], pa, attributes,
		memoryType, fIsKernelMap);

	pinner.Unlock();

	// Note: We don't need to invalidate the TLB for this address, as previously
	// the entry was not present and the TLB doesn't cache those entries.

	fMapCount++;

	return 0;
#endif
}


status_t
PPCVMTranslationMapClassic::Unmap(addr_t start, addr_t end)
{
	page_table_entry *entry;

	start = ROUNDDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

	if (start >= end)
		return B_OK;

	TRACE("unmap_tmap: asked to free pages 0x%lx to 0x%lx\n", start, end);

//	dprintf("vm_translation_map.unmap_tmap: start 0x%lx, end 0x%lx\n", start, end);

	while (start < end) {
		if (RemovePageTableEntry(start))
			fMapCount--;

		start += B_PAGE_SIZE;
	}

	return B_OK;

#if 0//X86

	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("unmap_tmap: asked to free pages 0x%lx to 0x%lx\n", start, end);

	page_directory_entry *pd = fPagingStructures->pgdir_virt;

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & PPC_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & PPC_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); (index < 1024) && (start < end);
				index++, start += B_PAGE_SIZE) {
			if ((pt[index] & PPC_PTE_PRESENT) == 0) {
				// page mapping not valid
				continue;
			}

			TRACE("unmap_tmap: removing page 0x%lx\n", start);

			page_table_entry oldEntry
				= PPCPagingMethodClassic::ClearPageTableEntryFlags(&pt[index],
					PPC_PTE_PRESENT);
			fMapCount--;

			if ((oldEntry & PPC_PTE_ACCESSED) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flags was set, since only then the entry could have
				// been in any TLB.
				InvalidatePage(start);
			}
		}
	} while (start != 0 && start < end);

	return B_OK;
#endif
}


status_t
PPCVMTranslationMapClassic::RemapAddressRange(addr_t *_virtualAddress,
	size_t size, bool unmap)
{
	addr_t virtualAddress = ROUNDDOWN(*_virtualAddress, B_PAGE_SIZE);
	size = ROUNDUP(*_virtualAddress + size - virtualAddress, B_PAGE_SIZE);

	VMAddressSpace *addressSpace = VMAddressSpace::Kernel();

	// reserve space in the address space
	void *newAddress = NULL;
	status_t error = vm_reserve_address_range(addressSpace->ID(), &newAddress,
		B_ANY_KERNEL_ADDRESS, size, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (error != B_OK)
		return error;

	// get the area's first physical page
	page_table_entry *entry = LookupPageTableEntry(virtualAddress);
	if (!entry)
		return B_ERROR;
	phys_addr_t physicalBase = (phys_addr_t)entry->physical_page_number << 12;

	// map the pages
	error = ppc_map_address_range((addr_t)newAddress, physicalBase, size);
	if (error != B_OK)
		return error;

	*_virtualAddress = (addr_t)newAddress;

	// unmap the old pages
	if (unmap)
		ppc_unmap_address_range(virtualAddress, size);

	return B_OK;
}


status_t
PPCVMTranslationMapClassic::DebugMarkRangePresent(addr_t start, addr_t end,
	bool markPresent)
{
	panic("%s: UNIMPLEMENTED", __FUNCTION__);
	return B_ERROR;
#if 0//X86
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	page_directory_entry *pd = fPagingStructures->pgdir_virt;

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & PPC_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & PPC_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); (index < 1024) && (start < end);
				index++, start += B_PAGE_SIZE) {
			if ((pt[index] & PPC_PTE_PRESENT) == 0) {
				if (!markPresent)
					continue;

				PPCPagingMethodClassic::SetPageTableEntryFlags(&pt[index],
					PPC_PTE_PRESENT);
			} else {
				if (markPresent)
					continue;

				page_table_entry oldEntry
					= PPCPagingMethodClassic::ClearPageTableEntryFlags(&pt[index],
						PPC_PTE_PRESENT);

				if ((oldEntry & PPC_PTE_ACCESSED) != 0) {
					// Note, that we only need to invalidate the address, if the
					// accessed flags was set, since only then the entry could
					// have been in any TLB.
					InvalidatePage(start);
				}
			}
		}
	} while (start != 0 && start < end);

	return B_OK;
#endif
}


/*!	Caller must have locked the cache of the page to be unmapped.
	This object shouldn't be locked.
*/
status_t
PPCVMTranslationMapClassic::UnmapPage(VMArea* area, addr_t address,
	bool updatePageQueue)
{
	ASSERT(address % B_PAGE_SIZE == 0);

	RecursiveLocker locker(fLock);

	if (area->cache_type == CACHE_TYPE_DEVICE) {
		if (!RemovePageTableEntry(address))
			return B_ENTRY_NOT_FOUND;

		fMapCount--;
		return B_OK;
	}

	page_table_entry* entry = LookupPageTableEntry(address);
	if (entry == NULL)
		return B_ENTRY_NOT_FOUND;

	page_num_t pageNumber = entry->physical_page_number;
	bool accessed = entry->referenced;
	bool modified = entry->changed;

	RemovePageTableEntry(address);

	fMapCount--;

	locker.Detach();
		// PageUnmapped() will unlock for us

	PageUnmapped(area, pageNumber, accessed, modified, updatePageQueue);

	return B_OK;

#if 0//X86

	ASSERT(address % B_PAGE_SIZE == 0);

	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	TRACE("PPCVMTranslationMapClassic::UnmapPage(%#" B_PRIxADDR ")\n", address);

	RecursiveLocker locker(fLock);

	int index = VADDR_TO_PDENT(address);
	if ((pd[index] & PPC_PDE_PRESENT) == 0)
		return B_ENTRY_NOT_FOUND;

	ThreadCPUPinner pinner(thread_get_current_thread());

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & PPC_PDE_ADDRESS_MASK);

	index = VADDR_TO_PTENT(address);
	page_table_entry oldEntry = PPCPagingMethodClassic::ClearPageTableEntry(
		&pt[index]);

	pinner.Unlock();

	if ((oldEntry & PPC_PTE_PRESENT) == 0) {
		// page mapping not valid
		return B_ENTRY_NOT_FOUND;
	}

	fMapCount--;

	if ((oldEntry & PPC_PTE_ACCESSED) != 0) {
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

	PageUnmapped(area, (oldEntry & PPC_PTE_ADDRESS_MASK) / B_PAGE_SIZE,
		(oldEntry & PPC_PTE_ACCESSED) != 0, (oldEntry & PPC_PTE_DIRTY) != 0,
		updatePageQueue);

	return B_OK;
#endif
}


void
PPCVMTranslationMapClassic::UnmapPages(VMArea* area, addr_t base, size_t size,
	bool updatePageQueue)
{
	panic("%s: UNIMPLEMENTED", __FUNCTION__);
#if 0//X86
	if (size == 0)
		return;

	addr_t start = base;
	addr_t end = base + size - 1;

	TRACE("PPCVMTranslationMapClassic::UnmapPages(%p, %#" B_PRIxADDR ", %#"
		B_PRIxADDR ")\n", area, start, end);

	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	VMAreaMappings queue;

	RecursiveLocker locker(fLock);

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & PPC_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & PPC_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); (index < 1024) && (start < end);
				index++, start += B_PAGE_SIZE) {
			page_table_entry oldEntry
				= PPCPagingMethodClassic::ClearPageTableEntry(&pt[index]);
			if ((oldEntry & PPC_PTE_PRESENT) == 0)
				continue;

			fMapCount--;

			if ((oldEntry & PPC_PTE_ACCESSED) != 0) {
				// Note, that we only need to invalidate the address, if the
				// accessed flags was set, since only then the entry could have
				// been in any TLB.
				InvalidatePage(start);
			}

			if (area->cache_type != CACHE_TYPE_DEVICE) {
				// get the page
				vm_page* page = vm_lookup_page(
					(oldEntry & PPC_PTE_ADDRESS_MASK) / B_PAGE_SIZE);
				ASSERT(page != NULL);

				DEBUG_PAGE_ACCESS_START(page);

				// transfer the accessed/dirty flags to the page
				if ((oldEntry & PPC_PTE_ACCESSED) != 0)
					page->accessed = true;
				if ((oldEntry & PPC_PTE_DIRTY) != 0)
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
#endif
}


void
PPCVMTranslationMapClassic::UnmapArea(VMArea* area, bool deletingAddressSpace,
	bool ignoreTopCachePageFlags)
{
	panic("%s: UNIMPLEMENTED", __FUNCTION__);
#if 0//X86
	if (area->cache_type == CACHE_TYPE_DEVICE || area->wiring != B_NO_LOCK) {
		PPCVMTranslationMapClassic::UnmapPages(area, area->Base(), area->Size(),
			true);
		return;
	}

	bool unmapPages = !deletingAddressSpace || !ignoreTopCachePageFlags;

	page_directory_entry* pd = fPagingStructures->pgdir_virt;

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

			int index = VADDR_TO_PDENT(address);
			if ((pd[index] & PPC_PDE_PRESENT) == 0) {
				panic("page %p has mapping for area %p (%#" B_PRIxADDR "), but "
					"has no page dir entry", page, area, address);
				continue;
			}

			ThreadCPUPinner pinner(thread_get_current_thread());

			page_table_entry* pt
				= (page_table_entry*)fPageMapper->GetPageTableAt(
					pd[index] & PPC_PDE_ADDRESS_MASK);
			page_table_entry oldEntry
				= PPCPagingMethodClassic::ClearPageTableEntry(
					&pt[VADDR_TO_PTENT(address)]);

			pinner.Unlock();

			if ((oldEntry & PPC_PTE_PRESENT) == 0) {
				panic("page %p has mapping for area %p (%#" B_PRIxADDR "), but "
					"has no page table entry", page, area, address);
				continue;
			}

			// transfer the accessed/dirty flags to the page and invalidate
			// the mapping, if necessary
			if ((oldEntry & PPC_PTE_ACCESSED) != 0) {
				page->accessed = true;

				if (!deletingAddressSpace)
					InvalidatePage(address);
			}

			if ((oldEntry & PPC_PTE_DIRTY) != 0)
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
#endif
}


status_t
PPCVMTranslationMapClassic::Query(addr_t va, phys_addr_t *_outPhysical,
	uint32 *_outFlags)
{
	page_table_entry *entry;

	// default the flags to not present
	*_outFlags = 0;
	*_outPhysical = 0;

	entry = LookupPageTableEntry(va);
	if (entry == NULL)
		return B_NO_ERROR;

	// ToDo: check this!
	if (IS_KERNEL_ADDRESS(va))
		*_outFlags |= B_KERNEL_READ_AREA | (entry->page_protection == PTE_READ_ONLY ? 0 : B_KERNEL_WRITE_AREA);
	else
		*_outFlags |= B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | (entry->page_protection == PTE_READ_ONLY ? 0 : B_WRITE_AREA);

	*_outFlags |= entry->changed ? PAGE_MODIFIED : 0;
	*_outFlags |= entry->referenced ? PAGE_ACCESSED : 0;
	*_outFlags |= entry->valid ? PAGE_PRESENT : 0;

	*_outPhysical = entry->physical_page_number * B_PAGE_SIZE;

	return B_OK;

#if 0//X86
	// default the flags to not present
	*_flags = 0;
	*_physical = 0;

	int index = VADDR_TO_PDENT(va);
	page_directory_entry *pd = fPagingStructures->pgdir_virt;
	if ((pd[index] & PPC_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & PPC_PDE_ADDRESS_MASK);
	page_table_entry entry = pt[VADDR_TO_PTENT(va)];

	*_physical = entry & PPC_PDE_ADDRESS_MASK;

	// read in the page state flags
	if ((entry & PPC_PTE_USER) != 0) {
		*_flags |= ((entry & PPC_PTE_WRITABLE) != 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA;
	}

	*_flags |= ((entry & PPC_PTE_WRITABLE) != 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & PPC_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & PPC_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((entry & PPC_PTE_PRESENT) != 0 ? PAGE_PRESENT : 0);

	pinner.Unlock();

	TRACE("query_tmap: returning pa 0x%lx for va 0x%lx\n", *_physical, va);

	return B_OK;
#endif
}


status_t
PPCVMTranslationMapClassic::QueryInterrupt(addr_t virtualAddress,
	phys_addr_t *_physicalAddress, uint32 *_flags)
{
	return PPCVMTranslationMapClassic::Query(virtualAddress, _physicalAddress, _flags);

#if 0//X86
	*_flags = 0;
	*_physical = 0;

	int index = VADDR_TO_PDENT(va);
	page_directory_entry* pd = fPagingStructures->pgdir_virt;
	if ((pd[index] & PPC_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	// map page table entry
	page_table_entry* pt = (page_table_entry*)PPCPagingMethodClassic::Method()
		->PhysicalPageMapper()->InterruptGetPageTableAt(
			pd[index] & PPC_PDE_ADDRESS_MASK);
	page_table_entry entry = pt[VADDR_TO_PTENT(va)];

	*_physical = entry & PPC_PDE_ADDRESS_MASK;

	// read in the page state flags
	if ((entry & PPC_PTE_USER) != 0) {
		*_flags |= ((entry & PPC_PTE_WRITABLE) != 0 ? B_WRITE_AREA : 0)
			| B_READ_AREA;
	}

	*_flags |= ((entry & PPC_PTE_WRITABLE) != 0 ? B_KERNEL_WRITE_AREA : 0)
		| B_KERNEL_READ_AREA
		| ((entry & PPC_PTE_DIRTY) != 0 ? PAGE_MODIFIED : 0)
		| ((entry & PPC_PTE_ACCESSED) != 0 ? PAGE_ACCESSED : 0)
		| ((entry & PPC_PTE_PRESENT) != 0 ? PAGE_PRESENT : 0);

	return B_OK;
#endif
}


status_t
PPCVMTranslationMapClassic::Protect(addr_t start, addr_t end, uint32 attributes,
	uint32 memoryType)
{
	// XXX finish
	return B_ERROR;
#if 0//X86
	start = ROUNDDOWN(start, B_PAGE_SIZE);
	if (start >= end)
		return B_OK;

	TRACE("protect_tmap: pages 0x%lx to 0x%lx, attributes %lx\n", start, end,
		attributes);

	// compute protection flags
	uint32 newProtectionFlags = 0;
	if ((attributes & B_USER_PROTECTION) != 0) {
		newProtectionFlags = PPC_PTE_USER;
		if ((attributes & B_WRITE_AREA) != 0)
			newProtectionFlags |= PPC_PTE_WRITABLE;
	} else if ((attributes & B_KERNEL_WRITE_AREA) != 0)
		newProtectionFlags = PPC_PTE_WRITABLE;

	page_directory_entry *pd = fPagingStructures->pgdir_virt;

	do {
		int index = VADDR_TO_PDENT(start);
		if ((pd[index] & PPC_PDE_PRESENT) == 0) {
			// no page table here, move the start up to access the next page
			// table
			start = ROUNDUP(start + 1, kPageTableAlignment);
			continue;
		}

		Thread* thread = thread_get_current_thread();
		ThreadCPUPinner pinner(thread);

		page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
			pd[index] & PPC_PDE_ADDRESS_MASK);

		for (index = VADDR_TO_PTENT(start); index < 1024 && start < end;
				index++, start += B_PAGE_SIZE) {
			page_table_entry entry = pt[index];
			if ((entry & PPC_PTE_PRESENT) == 0) {
				// page mapping not valid
				continue;
			}

			TRACE("protect_tmap: protect page 0x%lx\n", start);

			// set the new protection flags -- we want to do that atomically,
			// without changing the accessed or dirty flag
			page_table_entry oldEntry;
			while (true) {
				oldEntry = PPCPagingMethodClassic::TestAndSetPageTableEntry(
					&pt[index],
					(entry & ~(PPC_PTE_PROTECTION_MASK
							| PPC_PTE_MEMORY_TYPE_MASK))
						| newProtectionFlags
						| PPCPagingMethodClassic::MemoryTypeToPageTableEntryFlags(
							memoryType),
					entry);
				if (oldEntry == entry)
					break;
				entry = oldEntry;
			}

			if ((oldEntry & PPC_PTE_ACCESSED) != 0) {
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
PPCVMTranslationMapClassic::ClearFlags(addr_t virtualAddress, uint32 flags)
{
	page_table_entry *entry = LookupPageTableEntry(virtualAddress);
	if (entry == NULL)
		return B_NO_ERROR;

	bool modified = false;

	// clear the bits
	if (flags & PAGE_MODIFIED && entry->changed) {
		entry->changed = false;
		modified = true;
	}
	if (flags & PAGE_ACCESSED && entry->referenced) {
		entry->referenced = false;
		modified = true;
	}

	// synchronize
	if (modified) {
		tlbie(virtualAddress);
		eieio();
		tlbsync();
		ppc_sync();
	}

	return B_OK;

#if 0//X86
	int index = VADDR_TO_PDENT(va);
	page_directory_entry* pd = fPagingStructures->pgdir_virt;
	if ((pd[index] & PPC_PDE_PRESENT) == 0) {
		// no pagetable here
		return B_OK;
	}

	uint32 flagsToClear = ((flags & PAGE_MODIFIED) ? PPC_PTE_DIRTY : 0)
		| ((flags & PAGE_ACCESSED) ? PPC_PTE_ACCESSED : 0);

	Thread* thread = thread_get_current_thread();
	ThreadCPUPinner pinner(thread);

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & PPC_PDE_ADDRESS_MASK);
	index = VADDR_TO_PTENT(va);

	// clear out the flags we've been requested to clear
	page_table_entry oldEntry
		= PPCPagingMethodClassic::ClearPageTableEntryFlags(&pt[index],
			flagsToClear);

	pinner.Unlock();

	if ((oldEntry & flagsToClear) != 0)
		InvalidatePage(va);

	return B_OK;
#endif
}


bool
PPCVMTranslationMapClassic::ClearAccessedAndModified(VMArea* area,
	addr_t address, bool unmapIfUnaccessed, bool& _modified)
{
	ASSERT(address % B_PAGE_SIZE == 0);

	// TODO: Implement for real! ATM this is just an approximation using
	// Query(), ClearFlags(), and UnmapPage(). See below!

	RecursiveLocker locker(fLock);

	uint32 flags;
	phys_addr_t physicalAddress;
	if (Query(address, &physicalAddress, &flags) != B_OK
		|| (flags & PAGE_PRESENT) == 0) {
		return false;
	}

	_modified = (flags & PAGE_MODIFIED) != 0;

	if ((flags & (PAGE_ACCESSED | PAGE_MODIFIED)) != 0)
		ClearFlags(address, flags & (PAGE_ACCESSED | PAGE_MODIFIED));

	if ((flags & PAGE_ACCESSED) != 0)
		return true;

	if (!unmapIfUnaccessed)
		return false;

	locker.Unlock();

	UnmapPage(area, address, false);
		// TODO: Obvious race condition: Between querying and unmapping the
		// page could have been accessed. We try to compensate by considering
		// vm_page::{accessed,modified} (which would have been updated by
		// UnmapPage()) below, but that doesn't quite match the required
		// semantics of the method.

	vm_page* page = vm_lookup_page(physicalAddress / B_PAGE_SIZE);
	if (page == NULL)
		return false;

	_modified |= page->modified;

	return page->accessed;

#if 0//X86
	page_directory_entry* pd = fPagingStructures->pgdir_virt;

	TRACE("PPCVMTranslationMapClassic::ClearAccessedAndModified(%#" B_PRIxADDR
		")\n", address);

	RecursiveLocker locker(fLock);

	int index = VADDR_TO_PDENT(address);
	if ((pd[index] & PPC_PDE_PRESENT) == 0)
		return false;

	ThreadCPUPinner pinner(thread_get_current_thread());

	page_table_entry* pt = (page_table_entry*)fPageMapper->GetPageTableAt(
		pd[index] & PPC_PDE_ADDRESS_MASK);

	index = VADDR_TO_PTENT(address);

	// perform the deed
	page_table_entry oldEntry;

	if (unmapIfUnaccessed) {
		while (true) {
			oldEntry = pt[index];
			if ((oldEntry & PPC_PTE_PRESENT) == 0) {
				// page mapping not valid
				return false;
			}

			if (oldEntry & PPC_PTE_ACCESSED) {
				// page was accessed -- just clear the flags
				oldEntry = PPCPagingMethodClassic::ClearPageTableEntryFlags(
					&pt[index], PPC_PTE_ACCESSED | PPC_PTE_DIRTY);
				break;
			}

			// page hasn't been accessed -- unmap it
			if (PPCPagingMethodClassic::TestAndSetPageTableEntry(&pt[index], 0,
					oldEntry) == oldEntry) {
				break;
			}

			// something changed -- check again
		}
	} else {
		oldEntry = PPCPagingMethodClassic::ClearPageTableEntryFlags(&pt[index],
			PPC_PTE_ACCESSED | PPC_PTE_DIRTY);
	}

	pinner.Unlock();

	_modified = (oldEntry & PPC_PTE_DIRTY) != 0;

	if ((oldEntry & PPC_PTE_ACCESSED) != 0) {
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
		(oldEntry & PPC_PTE_ADDRESS_MASK) / B_PAGE_SIZE);

	return false;
#endif
}


PPCPagingStructures*
PPCVMTranslationMapClassic::PagingStructures() const
{
	return fPagingStructures;
}
