/*
 * Copyright 2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include "VMSAv8TranslationMap.h"

#include <algorithm>
#include <slab/Slab.h>
#include <util/AutoLock.h>
#include <util/ThreadAutoLock.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMCache.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>


//#define DO_TRACE
#ifdef DO_TRACE
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


uint32_t VMSAv8TranslationMap::fHwFeature;
uint64_t VMSAv8TranslationMap::fMair;

// ASID Management
static constexpr size_t kAsidBits = 8;
static constexpr size_t kNumAsids = (1 << kAsidBits);
static spinlock sAsidLock = B_SPINLOCK_INITIALIZER;
// A bitmap to track which ASIDs are in use.
static uint64 sAsidBitMap[kNumAsids / 64] = {};
// A mapping from ASID to translation map.
static VMSAv8TranslationMap* sAsidMapping[kNumAsids] = {};


static void
free_asid(size_t asid)
{
	for (size_t i = 0; i < B_COUNT_OF(sAsidBitMap); ++i) {
		if (asid < 64) {
			sAsidBitMap[i] &= ~(uint64_t{1} << asid);
			return;
		}
		asid -= 64;
	}

	panic("Could not free ASID!");
}


static void
flush_tlb_whole_asid(uint64_t asid)
{
	asm("dsb ishst");
	asm("tlbi aside1is, %0" ::"r"(asid << 48));
	asm("dsb ish");
	asm("isb");
}


static size_t
alloc_first_free_asid(void)
{
	int asid = 0;
	for (size_t i = 0; i < B_COUNT_OF(sAsidBitMap); ++i) {
		int avail = __builtin_ffsll(~sAsidBitMap[i]);
		if (avail != 0) {
			sAsidBitMap[i] |= (uint64_t{1} << (avail-1));
			asid += (avail - 1);
			return asid;
		}
		asid += 64;
	}

	return kNumAsids;
}


static bool
is_pte_dirty(uint64_t pte)
{
	if ((pte & kAttrSWDIRTY) != 0)
		return true;

	return (pte & kAttrAPReadOnly) == 0;
}


static uint64_t
set_pte_dirty(uint64_t pte)
{
	if ((pte & kAttrSWDBM) != 0)
		return pte & ~kAttrAPReadOnly;

	return pte | kAttrSWDIRTY;
}


static uint64_t
set_pte_clean(uint64_t pte)
{
	pte &= ~kAttrSWDIRTY;
	return pte | kAttrAPReadOnly;
}


static bool
is_pte_accessed(uint64_t pte)
{
	return (pte & kPteValidMask) != 0 && (pte & kAttrAF) != 0;
}


VMSAv8TranslationMap::VMSAv8TranslationMap(
	bool kernel, phys_addr_t pageTable, int pageBits, int vaBits, int minBlockLevel)
	:
	fIsKernel(kernel),
	fPageTable(pageTable),
	fPageBits(pageBits),
	fVaBits(vaBits),
	fMinBlockLevel(minBlockLevel),
	fASID(kernel ? 0 : -1),
	fRefcount(0)
{
	TRACE("+VMSAv8TranslationMap(%p, %d, 0x%" B_PRIxADDR ", %d, %d, %d)\n", this,
		kernel, pageTable, pageBits, vaBits, minBlockLevel);

	fInitialLevel = CalcStartLevel(fVaBits, fPageBits);
}


VMSAv8TranslationMap::~VMSAv8TranslationMap()
{
	TRACE("-VMSAv8TranslationMap(%p)\n", this);
	TRACE("  fIsKernel: %d, fPageTable: 0x%" B_PRIxADDR ", fASID: %d, fRefcount: %d\n",
		fIsKernel, fPageTable, fASID, fRefcount);

	ASSERT(!fIsKernel);
	ASSERT(fRefcount == 0);
	{
		ThreadCPUPinner pinner(thread_get_current_thread());
		FreeTable(fPageTable, 0, fInitialLevel, [](int level, uint64_t oldPte) {});
	}

	{
		InterruptsSpinLocker locker(sAsidLock);

		if (fASID != -1) {
			sAsidMapping[fASID] = NULL;
			free_asid(fASID);
		}
	}
}


// Switch user map into TTBR0.
// Passing kernel map here configures empty page table.
void
VMSAv8TranslationMap::SwitchUserMap(VMSAv8TranslationMap *from, VMSAv8TranslationMap *to)
{
	InterruptsSpinLocker locker(sAsidLock);

	if (!from->fIsKernel) {
		from->fRefcount--;
	}

	if (!to->fIsKernel) {
		to->fRefcount++;
	} else {
		arch_vm_install_empty_table_ttbr0();
		return;
	}

	ASSERT(to->fPageTable != 0);
	uint64_t ttbr = to->fPageTable | ((fHwFeature & HW_COMMON_NOT_PRIVATE) != 0 ? 1 : 0);

	if (to->fASID != -1) {
		WRITE_SPECIALREG(TTBR0_EL1, ((uint64_t)to->fASID << 48) | ttbr);
		asm("isb");
		return;
	}

	size_t allocatedAsid = alloc_first_free_asid();
	if (allocatedAsid != kNumAsids) {
		to->fASID = allocatedAsid;
		sAsidMapping[allocatedAsid] = to;

		WRITE_SPECIALREG(TTBR0_EL1, (allocatedAsid << 48) | ttbr);
		flush_tlb_whole_asid(allocatedAsid);
		return;
	}

	// ASID 0 is reserved for the kernel.
	for (size_t i = 1; i < kNumAsids; ++i) {
		if (sAsidMapping[i]->fRefcount == 0) {
			sAsidMapping[i]->fASID = -1;
			to->fASID = i;
			sAsidMapping[i] = to;

			WRITE_SPECIALREG(TTBR0_EL1, (i << 48) | ttbr);
			flush_tlb_whole_asid(i);
			return;
		}
	}

	panic("cannot assign ASID");
}


int
VMSAv8TranslationMap::CalcStartLevel(int vaBits, int pageBits)
{
	int level = 4;

	int bitsLeft = vaBits - pageBits;
	while (bitsLeft > 0) {
		int tableBits = pageBits - 3;
		bitsLeft -= tableBits;
		level--;
	}

	ASSERT(level >= 0);

	return level;
}


bool
VMSAv8TranslationMap::Lock()
{
	TRACE("VMSAv8TranslationMap::Lock()\n");
	recursive_lock_lock(&fLock);
	return true;
}


void
VMSAv8TranslationMap::Unlock()
{
	TRACE("VMSAv8TranslationMap::Unlock()\n");
	recursive_lock_unlock(&fLock);
}


addr_t
VMSAv8TranslationMap::MappedSize() const
{
	panic("VMSAv8TranslationMap::MappedSize not implemented");
	return 0;
}


size_t
VMSAv8TranslationMap::MaxPagesNeededToMap(addr_t start, addr_t end) const
{
	size_t result = 0;
	size_t size = end - start + 1;

	for (int i = fInitialLevel; i < 3; i++) {
		int tableBits = fPageBits - 3;
		int shift = tableBits * (3 - i) + fPageBits;
		uint64_t entrySize = 1UL << shift;

		result += size / entrySize + 2;
	}

	return result;
}


uint64_t*
VMSAv8TranslationMap::TableFromPa(phys_addr_t pa)
{
	return reinterpret_cast<uint64_t*>(KERNEL_PMAP_BASE + pa);
}


template<typename EntryRemoved>
void
VMSAv8TranslationMap::FreeTable(phys_addr_t ptPa, uint64_t va, int level,
	EntryRemoved &&entryRemoved)
{
	ASSERT(level < 4);

	int tableBits = fPageBits - 3;
	uint64_t tableSize = 1UL << tableBits;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	int shift = tableBits * (3 - level) + fPageBits;
	uint64_t entrySize = 1UL << shift;

	uint64_t nextVa = va;
	uint64_t* pt = TableFromPa(ptPa);
	for (uint64_t i = 0; i < tableSize; i++) {
		uint64_t oldPte = (uint64_t) atomic_get_and_set64((int64*) &pt[i], 0);

		if (level < 3 && (oldPte & kPteTypeMask) == kPteTypeL012Table) {
			FreeTable(oldPte & kPteAddrMask, nextVa, level + 1, entryRemoved);
		} else if ((oldPte & kPteTypeMask) != 0) {
			uint64_t fullVa = (fIsKernel ? ~vaMask : 0) | nextVa;
			asm("dsb ishst");
			asm("tlbi vaae1is, %0" :: "r" ((fullVa >> 12) & kTLBIMask));
			// Does it correctly flush block entries at level < 3? We don't use them anyway though.
			// TODO: Flush only currently used ASID (using vae1is)
			entryRemoved(level, oldPte);
		}

		nextVa += entrySize;
	}

	asm("dsb ish");

	vm_page* page = vm_lookup_page(ptPa >> fPageBits);
	DEBUG_PAGE_ACCESS_START(page);
	vm_page_set_state(page, PAGE_STATE_FREE);
}


// Make a new page sub-table.
// The parent table is `ptPa`, and the new sub-table's PTE will be at `index`
// in it.
// Returns the physical address of the new table, or the address of the existing
// one if the PTE is already filled.
phys_addr_t
VMSAv8TranslationMap::GetOrMakeTable(phys_addr_t ptPa, int level, int index,
	vm_page_reservation* reservation)
{
	ASSERT(level < 3);

	uint64_t* ptePtr = TableFromPa(ptPa) + index;
	uint64_t oldPte = atomic_get64((int64*) ptePtr);

	int type = oldPte & kPteTypeMask;
	ASSERT(type != kPteTypeL12Block);

	if (type == kPteTypeL012Table) {
		// This is table entry already, just return it
		return oldPte & kPteAddrMask;
	} else if (reservation != nullptr) {
		// Create new table there
		vm_page* page = vm_page_allocate_page(reservation, PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
		phys_addr_t newTablePa = page->physical_page_number << fPageBits;
		DEBUG_PAGE_ACCESS_END(page);

		// We only create mappings at the final level so we don't need to handle
		// splitting block mappings
		ASSERT(type != kPteTypeL12Block);

		// Ensure that writes to page being attached have completed
		asm("dsb ishst");

		uint64_t oldPteRefetch = (uint64_t)atomic_test_and_set64((int64*) ptePtr,
			newTablePa | kPteTypeL012Table, oldPte);
		if (oldPteRefetch != oldPte) {
			// If the old PTE has mutated, it must be because another thread has allocated the
			// sub-table at the same time as us. If that has happened, deallocate the page we
			// setup and use the one they installed instead.
			ASSERT((oldPteRefetch & kPteTypeMask) == kPteTypeL012Table);
			DEBUG_PAGE_ACCESS_START(page);
			vm_page_set_state(page, PAGE_STATE_FREE);
			return oldPteRefetch & kPteAddrMask;
		}

		return newTablePa;
	}

	// There's no existing table and we have no reservation
	return 0;
}


bool
VMSAv8TranslationMap::FlushVAIfAccessed(uint64_t pte, addr_t va)
{
	if (!is_pte_accessed(pte))
		return false;

	InterruptsSpinLocker locker(sAsidLock);
	if ((pte & kAttrNG) == 0) {
		// Flush from all address spaces
		asm("dsb ishst"); // Ensure PTE write completed
		asm("tlbi vaae1is, %0" ::"r"(((va >> 12) & kTLBIMask)));
		asm("dsb ish");
		asm("isb");
	} else if (fASID != -1) {
		asm("dsb ishst"); // Ensure PTE write completed
        asm("tlbi vae1is, %0" ::"r"(((va >> 12) & kTLBIMask) | (uint64_t(fASID) << 48)));
		asm("dsb ish"); // Wait for TLB flush to complete
		asm("isb");
		return true;
	}

	return false;
}


bool
VMSAv8TranslationMap::AttemptPteBreakBeforeMake(uint64_t* ptePtr, uint64_t oldPte, addr_t va)
{
	uint64_t loadedPte = atomic_test_and_set64((int64_t*)ptePtr, 0, oldPte);
	if (loadedPte != oldPte)
		return false;
		
	FlushVAIfAccessed(oldPte, va);

	return true;
}


template<typename UpdatePte>
void
VMSAv8TranslationMap::ProcessRange(phys_addr_t ptPa, int level, addr_t va, size_t size,
    vm_page_reservation* reservation, UpdatePte&& updatePte)
{
	ASSERT(level < 4);
	ASSERT(ptPa != 0);

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((va & pageMask) == 0);

	int tableBits = fPageBits - 3;
	uint64_t tableMask = (1UL << tableBits) - 1;

	int shift = tableBits * (3 - level) + fPageBits;
	uint64_t entrySize = 1UL << shift;
	uint64_t entryMask = entrySize - 1;

	uint64_t alignedDownVa = va & ~entryMask;
	uint64_t alignedUpEnd = (va + size + (entrySize - 1)) & ~entryMask;
	if (level == 3)
		ASSERT(alignedDownVa == va);

    for (uint64_t effectiveVa = alignedDownVa; effectiveVa < alignedUpEnd;
        effectiveVa += entrySize) {
		int index = ((effectiveVa & vaMask) >> shift) & tableMask;
		uint64_t* ptePtr = TableFromPa(ptPa) + index;

		if (level == 3) {
			updatePte(ptePtr, effectiveVa);
		} else {
			phys_addr_t subTable = GetOrMakeTable(ptPa, level, index, reservation);

			// When reservation is null, we can't create a new subtable. This can be intentional,
			// for example when called from Unmap().
			if (subTable == 0)
				continue;

			uint64_t subVa = std::max(effectiveVa, va);
			size_t subSize = std::min(size_t(entrySize - (subVa & entryMask)), size);
            ProcessRange(subTable, level + 1, subVa, subSize, reservation, updatePte);

			size -= subSize;
		}
	}
}


uint8_t
VMSAv8TranslationMap::MairIndex(uint8_t type)
{
	for (int i = 0; i < 8; i++)
		if (((fMair >> (i * 8)) & 0xff) == type)
			return i;

	panic("MAIR entry not found");
	return 0;
}


uint64_t
VMSAv8TranslationMap::GetMemoryAttr(uint32 attributes, uint32 memoryType, bool isKernel)
{
	uint64_t attr = 0;

	if (!isKernel)
		attr |= kAttrNG;

	if ((attributes & B_EXECUTE_AREA) == 0)
		attr |= kAttrUXN;
	if ((attributes & B_KERNEL_EXECUTE_AREA) == 0)
		attr |= kAttrPXN;

	// SWDBM is software reserved bit that we use to mark that
	// writes are allowed, and fault handler should clear kAttrAPReadOnly.
	// In that case kAttrAPReadOnly doubles as not-dirty bit.
	// Additionally dirty state can be stored in SWDIRTY, in order not to lose
	// dirty state when changing protection from RW to RO.

	// All page permissions begin life in RO state.
	attr |= kAttrAPReadOnly;

	// User-Execute implies User-Read, because it would break PAN otherwise
	if ((attributes & B_READ_AREA) != 0 || (attributes & B_EXECUTE_AREA) != 0)
		attr |= kAttrAPUserAccess; // Allow user reads

	if ((attributes & B_WRITE_AREA) != 0 || (attributes & B_KERNEL_WRITE_AREA) != 0)
		attr |= kAttrSWDBM; // Mark as writeable

	// When supported by hardware copy our SWDBM bit into DBM,
	// so that kAttrAPReadOnly is cleared on write attempt automatically
	// without going through fault handler.
	if ((fHwFeature & HW_DIRTY) != 0 && (attr & kAttrSWDBM) != 0)
		attr |= kAttrDBM;

	attr |= kAttrSHInnerShareable; // Inner Shareable

	uint8_t type = MAIR_NORMAL_WB;

	switch (memoryType & B_MEMORY_TYPE_MASK) {
		case B_UNCACHED_MEMORY:
			// TODO: This probably should be nGnRE for PCI
			type = MAIR_DEVICE_nGnRnE;
			break;
		case B_WRITE_COMBINING_MEMORY:
			type = MAIR_NORMAL_NC;
			break;
		case B_WRITE_THROUGH_MEMORY:
			type = MAIR_NORMAL_WT;
			break;
		case B_WRITE_PROTECTED_MEMORY:
			type = MAIR_NORMAL_WT;
			break;
		default:
		case B_WRITE_BACK_MEMORY:
			type = MAIR_NORMAL_WB;
			break;
	}

	attr |= MairIndex(type) << 2;

	return attr;
}


status_t
VMSAv8TranslationMap::Map(addr_t va, phys_addr_t pa, uint32 attributes, uint32 memoryType,
	vm_page_reservation* reservation)
{
	TRACE("VMSAv8TranslationMap::Map(0x%" B_PRIxADDR ", 0x%" B_PRIxADDR
		", 0x%x, 0x%x)\n", va, pa, attributes, memoryType);

	ThreadCPUPinner pinner(thread_get_current_thread());

	ASSERT(ValidateVa(va));
	uint64_t attr = GetMemoryAttr(attributes, memoryType, fIsKernel);

	// During first mapping we need to allocate root table
	if (fPageTable == 0) {
		vm_page* page = vm_page_allocate_page(reservation, PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
		DEBUG_PAGE_ACCESS_END(page);
		fPageTable = page->physical_page_number << fPageBits;
	}

	ProcessRange(fPageTable, fInitialLevel, va, B_PAGE_SIZE, reservation,
		[=](uint64_t* ptePtr, uint64_t effectiveVa) {
			while (true) {
				phys_addr_t effectivePa = effectiveVa - va + pa;
				uint64_t oldPte = atomic_get64((int64*)ptePtr);
				uint64_t newPte = effectivePa | attr | kPteTypeL3Page;

				if (newPte == oldPte)
					return;

				if ((oldPte & kPteValidMask) != 0) {
					// ARM64 requires "break-before-make". We must set the PTE to an invalid
					// entry and flush the TLB as appropriate before we can write the new PTE.
					if (!AttemptPteBreakBeforeMake(ptePtr, oldPte, effectiveVa))
						continue;
				}

				// Install the new PTE
				atomic_set64((int64*)ptePtr, newPte);
				asm("dsb ishst"); // Ensure PTE write completed
				asm("isb");
				break;
			}
		});

	return B_OK;
}


status_t
VMSAv8TranslationMap::Unmap(addr_t start, addr_t end)
{
	TRACE("VMSAv8TranslationMap::Unmap(0x%" B_PRIxADDR ", 0x%" B_PRIxADDR
		")\n", start, end);
	ThreadCPUPinner pinner(thread_get_current_thread());

	size_t size = end - start + 1;
	ASSERT(ValidateVa(start));

	if (fPageTable == 0)
		return B_OK;

	ProcessRange(fPageTable, fInitialLevel, start, size, nullptr,
		[=](uint64_t* ptePtr, uint64_t effectiveVa) {
			ASSERT(effectiveVa <= end);
			uint64_t oldPte = atomic_get_and_set64((int64_t*)ptePtr, 0);
			FlushVAIfAccessed(oldPte, effectiveVa);
		});

	return B_OK;
}


status_t
VMSAv8TranslationMap::UnmapPage(VMArea* area, addr_t address, bool updatePageQueue)
{
	TRACE("VMSAv8TranslationMap::UnmapPage(0x%" B_PRIxADDR "(%s), 0x%"
		B_PRIxADDR ", %d)\n", (addr_t)area, area->name, address,
		updatePageQueue);

	ASSERT(ValidateVa(address));
	ThreadCPUPinner pinner(thread_get_current_thread());
	RecursiveLocker locker(fLock);

	uint64_t oldPte = 0;
	ProcessRange(fPageTable, fInitialLevel, address, B_PAGE_SIZE, nullptr,
		[=, &oldPte](uint64_t* ptePtr, uint64_t effectiveVa) {
			oldPte = atomic_get_and_set64((int64_t*)ptePtr, 0);
			FlushVAIfAccessed(oldPte, effectiveVa);
		});

	if ((oldPte & kPteValidMask) == 0)
		return B_ENTRY_NOT_FOUND;

	pinner.Unlock();
	locker.Detach();
	PageUnmapped(area, (oldPte & kPteAddrMask) >> fPageBits, (oldPte & kAttrAF) != 0,
		is_pte_dirty(oldPte), updatePageQueue);

	return B_OK;
}


void
VMSAv8TranslationMap::UnmapPages(VMArea* area, addr_t address, size_t size, bool updatePageQueue)
{
	TRACE("VMSAv8TranslationMap::UnmapPages(0x%" B_PRIxADDR "(%s), 0x%"
		B_PRIxADDR ", 0x%" B_PRIxSIZE ", %d)\n", (addr_t)area,
		area->name, address, size, updatePageQueue);

	ASSERT(ValidateVa(address));
	VMAreaMappings queue;
	ThreadCPUPinner pinner(thread_get_current_thread());
	RecursiveLocker locker(fLock);

	ProcessRange(fPageTable, fInitialLevel, address, size, nullptr,
		[=, &queue](uint64_t* ptePtr, uint64_t effectiveVa) {
			uint64_t oldPte = atomic_get_and_set64((int64_t*)ptePtr, 0);
			FlushVAIfAccessed(oldPte, effectiveVa);
			if ((oldPte & kPteValidMask) == 0)
				return;

			if (area->cache_type == CACHE_TYPE_DEVICE)
				return;

			// get the page
			vm_page* page = vm_lookup_page((oldPte & kPteAddrMask) >> fPageBits);
			ASSERT(page != NULL);

			DEBUG_PAGE_ACCESS_START(page);

			// transfer the accessed/dirty flags to the page
			page->accessed = (oldPte & kAttrAF) != 0;
			page->modified = is_pte_dirty(oldPte);

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
		});

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


void
VMSAv8TranslationMap::UnmapArea(VMArea* area, bool deletingAddressSpace,
	bool ignoreTopCachePageFlags)
{
	TRACE("VMSAv8TranslationMap::UnmapArea(0x%" B_PRIxADDR "(%s), 0x%"
		B_PRIxADDR ", 0x%" B_PRIxSIZE ", %d, %d)\n", (addr_t)area,
		area->name, area->Base(), area->Size(), deletingAddressSpace,
		ignoreTopCachePageFlags);

	if (area->cache_type == CACHE_TYPE_DEVICE || area->wiring != B_NO_LOCK) {
		UnmapPages(area, area->Base(), area->Size(), true);
		return;
	}

	bool unmapPages = !deletingAddressSpace || !ignoreTopCachePageFlags;

	RecursiveLocker locker(fLock);
	ThreadCPUPinner pinner(thread_get_current_thread());

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
				+ ((page->cache_offset * B_PAGE_SIZE)
				- area->cache_offset);

			uint64_t oldPte = 0;
			ProcessRange(fPageTable, fInitialLevel, address, B_PAGE_SIZE, nullptr,
				[=, &oldPte](uint64_t* ptePtr, uint64_t effectiveVa) {
					oldPte = atomic_get_and_set64((int64_t*)ptePtr, 0);
					if (!deletingAddressSpace)
						FlushVAIfAccessed(oldPte, effectiveVa);
				});

			if ((oldPte & kPteValidMask) == 0) {
				panic("page %p has mapping for area %p "
					"(%#" B_PRIxADDR "), but has no "
					"page table", page, area, address);
				continue;
			}

			// transfer the accessed/dirty flags to the page and
			// invalidate the mapping, if necessary
			if (is_pte_dirty(oldPte))
				page->modified = true;
			if (oldPte & kAttrAF)
				page->accessed = true;

			if (pageFullyUnmapped) {
				DEBUG_PAGE_ACCESS_START(page);

				if (cache->temporary) {
					vm_page_set_state(page,
						PAGE_STATE_INACTIVE);
				} else if (page->modified) {
					vm_page_set_state(page,
						PAGE_STATE_MODIFIED);
				} else {
					vm_page_set_state(page,
						PAGE_STATE_CACHED);
				}

				DEBUG_PAGE_ACCESS_END(page);
			}
		}
	}

	locker.Unlock();

	bool isKernelSpace = area->address_space == VMAddressSpace::Kernel();
	uint32 freeFlags = CACHE_DONT_WAIT_FOR_MEMORY
		| (isKernelSpace ? CACHE_DONT_LOCK_KERNEL_SPACE : 0);

	while (vm_page_mapping* mapping = mappings.RemoveHead())
		vm_free_page_mapping(mapping->page->physical_page_number, mapping, freeFlags);
}


bool
VMSAv8TranslationMap::ValidateVa(addr_t va)
{
	uint64_t vaMask = (1UL << fVaBits) - 1;
	bool kernelAddr = (va & (1UL << 63)) != 0;
	if (kernelAddr != fIsKernel)
		return false;
	if ((va & ~vaMask) != (fIsKernel ? ~vaMask : 0))
		return false;
	return true;
}


status_t
VMSAv8TranslationMap::Query(addr_t va, phys_addr_t* pa, uint32* flags)
{
	*flags = 0;
	*pa = 0;

	uint64_t pageMask = (1UL << fPageBits) - 1;
	va &= ~pageMask;

	ThreadCPUPinner pinner(thread_get_current_thread());
	ASSERT(ValidateVa(va));

	ProcessRange(fPageTable, fInitialLevel, va, B_PAGE_SIZE, nullptr,
		[=](uint64_t* ptePtr, uint64_t effectiveVa) {
			uint64_t pte = atomic_get64((int64_t*)ptePtr);
			*pa = pte & kPteAddrMask;
			*flags |= PAGE_PRESENT | B_KERNEL_READ_AREA;
			if ((pte & kAttrAF) != 0)
				*flags |= PAGE_ACCESSED;
			if (is_pte_dirty(pte))
				*flags |= PAGE_MODIFIED;

			if ((pte & kAttrUXN) == 0)
				*flags |= B_EXECUTE_AREA;
			if ((pte & kAttrPXN) == 0)
				*flags |= B_KERNEL_EXECUTE_AREA;

			if ((pte & kAttrAPUserAccess) != 0)
				*flags |= B_READ_AREA;

			if ((pte & kAttrSWDBM) != 0) {
				*flags |= B_KERNEL_WRITE_AREA;
				if ((pte & kAttrAPUserAccess) != 0)
					*flags |= B_WRITE_AREA;
			}
		});

	return B_OK;
}


status_t
VMSAv8TranslationMap::QueryInterrupt(
	addr_t virtualAddress, phys_addr_t* _physicalAddress, uint32* _flags)
{
	return Query(virtualAddress, _physicalAddress, _flags);
}


status_t
VMSAv8TranslationMap::Protect(addr_t start, addr_t end, uint32 attributes, uint32 memoryType)
{
	TRACE("VMSAv8TranslationMap::Protect(0x%" B_PRIxADDR ", 0x%"
		B_PRIxADDR ", 0x%x, 0x%x)\n", start, end, attributes, memoryType);

	uint64_t attr = GetMemoryAttr(attributes, memoryType, fIsKernel);
	size_t size = end - start + 1;
	ASSERT(ValidateVa(start));

	ThreadCPUPinner pinner(thread_get_current_thread());

	ProcessRange(fPageTable, fInitialLevel, start, size, nullptr,
		[=](uint64_t* ptePtr, uint64_t effectiveVa) {
			ASSERT(effectiveVa <= end);

			// We need to use an atomic compare-swap loop because we must
			// need to clear somes bits while setting others.
			while (true) {
				uint64_t oldPte = atomic_get64((int64_t*)ptePtr);
				uint64_t newPte = oldPte & ~kPteAttrMask;
				newPte |= attr;

				// Preserve access bit.
				newPte |= oldPte & kAttrAF;

				// Preserve the dirty bit.
				if (is_pte_dirty(oldPte))
					newPte = set_pte_dirty(newPte);

				uint64_t oldMemoryType = oldPte & (kAttrShareability | kAttrMemoryAttrIdx);
				uint64_t newMemoryType = newPte & (kAttrShareability | kAttrMemoryAttrIdx);
				if (oldMemoryType != newMemoryType) {
					// ARM64 requires "break-before-make". We must set the PTE to an invalid
					// entry and flush the TLB as appropriate before we can write the new PTE.
					// In this case specifically, it applies any time we change cacheability or
					// shareability.
					if (!AttemptPteBreakBeforeMake(ptePtr, oldPte, effectiveVa))
						continue;

					atomic_set64((int64_t*)ptePtr, newPte);
					asm("dsb ishst"); // Ensure PTE write completed
					asm("isb");

					// No compare-exchange loop required in this case.
					break;
				} else {
					if ((uint64_t)atomic_test_and_set64((int64_t*)ptePtr, newPte, oldPte) == oldPte) {
						FlushVAIfAccessed(oldPte, effectiveVa);
						break;
					}
				}
			}
		});

	return B_OK;
}


status_t
VMSAv8TranslationMap::ClearFlags(addr_t va, uint32 flags)
{
	ASSERT(ValidateVa(va));

	bool clearAF = flags & PAGE_ACCESSED;
	bool setRO = flags & PAGE_MODIFIED;

	if (!clearAF && !setRO)
		return B_OK;

	ThreadCPUPinner pinner(thread_get_current_thread());

	uint64_t oldPte = 0;
	ProcessRange(fPageTable, fInitialLevel, va, B_PAGE_SIZE, nullptr,
		[=, &oldPte](uint64_t* ptePtr, uint64_t effectiveVa) {
			if (clearAF && setRO) {
				// We need to use an atomic compare-swap loop because we must
				// need to clear one bit while setting the other.
				while (true) {
					oldPte = atomic_get64((int64_t*)ptePtr);
					uint64_t newPte = oldPte & ~kAttrAF;
					newPte = set_pte_clean(newPte);

                    if ((uint64_t)atomic_test_and_set64((int64_t*)ptePtr, newPte, oldPte) == oldPte)
						break;
				}
			} else if (clearAF) {
				oldPte = atomic_and64((int64_t*)ptePtr, ~kAttrAF);
			} else {
				while (true) {
					oldPte = atomic_get64((int64_t*)ptePtr);
					if (!is_pte_dirty(oldPte)) {
						// Avoid a TLB flush
						oldPte = 0;
						return;
					}
					uint64_t newPte = set_pte_clean(oldPte);
                    if ((uint64_t)atomic_test_and_set64((int64_t*)ptePtr, newPte, oldPte) == oldPte)
						break;
				}
			}
		});

	FlushVAIfAccessed(oldPte, va);

	return B_OK;
}


bool
VMSAv8TranslationMap::ClearAccessedAndModified(
	VMArea* area, addr_t address, bool unmapIfUnaccessed, bool& _modified)
{
	TRACE("VMSAv8TranslationMap::ClearAccessedAndModified(0x%"
		B_PRIxADDR "(%s), 0x%" B_PRIxADDR ", %d)\n", (addr_t)area,
		area->name, address, unmapIfUnaccessed);
	ASSERT(ValidateVa(address));

	RecursiveLocker locker(fLock);
	ThreadCPUPinner pinner(thread_get_current_thread());

	uint64_t oldPte = 0;
	ProcessRange(fPageTable, fInitialLevel, address, B_PAGE_SIZE, nullptr,
		[=, &oldPte](uint64_t* ptePtr, uint64_t effectiveVa) {
			// We need to use an atomic compare-swap loop because we must
			// first read the old PTE and make decisions based on the AF
			// bit to proceed.
			while (true) {
				oldPte = atomic_get64((int64_t*)ptePtr);
				uint64_t newPte = oldPte & ~kAttrAF;
				newPte = set_pte_clean(newPte);

				// If the page has been not be accessed, then unmap it.
				if (unmapIfUnaccessed && (oldPte & kAttrAF) == 0)
					newPte = 0;

				if ((uint64_t)atomic_test_and_set64((int64_t*)ptePtr, newPte, oldPte) == oldPte)
					break;
			}
			asm("dsb ishst"); // Ensure PTE write completed
		});

	pinner.Unlock();
	_modified = is_pte_dirty(oldPte);

	if (FlushVAIfAccessed(oldPte, address))
		return true;

	if (!unmapIfUnaccessed)
		return false;

	locker.Detach(); // UnaccessedPageUnmapped takes ownership
	phys_addr_t oldPa = oldPte & kPteAddrMask;
	UnaccessedPageUnmapped(area, oldPa >> fPageBits);
	return false;
}


void
VMSAv8TranslationMap::Flush()
{
	// Necessary invalidation is performed during mapping,
	// no need to do anything more here.
}
