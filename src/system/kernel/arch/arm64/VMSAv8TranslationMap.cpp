/*
 * Copyright 2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include "VMSAv8TranslationMap.h"

#include <algorithm>
#include <util/AutoLock.h>
#include <util/ThreadAutoLock.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>

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


VMSAv8TranslationMap::VMSAv8TranslationMap(
	bool kernel, phys_addr_t pageTable, int pageBits, int vaBits, int minBlockLevel)
	:
	fIsKernel(kernel),
	fPageTable(pageTable),
	fPageBits(pageBits),
	fVaBits(vaBits),
	fMinBlockLevel(minBlockLevel),
	fASID(-1),
	fRefcount(0)
{
	dprintf("VMSAv8TranslationMap\n");

	fInitialLevel = CalcStartLevel(fVaBits, fPageBits);
}


VMSAv8TranslationMap::~VMSAv8TranslationMap()
{
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
	SpinLocker locker(sAsidLock);

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
		asm("isb");
		return;
	}

	for (size_t i = 0; i < kNumAsids; ++i) {
		if (sAsidMapping[i]->fRefcount == 0) {
			sAsidMapping[i]->fASID = -1;
			to->fASID = i;
			sAsidMapping[i] = to;

			WRITE_SPECIALREG(TTBR0_EL1, (i << 48) | ttbr);
			asm("dsb ishst");
			asm("tlbi aside1is, %0" :: "r" (i << 48));
			asm("dsb ish");
			asm("isb");
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
	recursive_lock_lock(&fLock);
	return true;
}


void
VMSAv8TranslationMap::Unlock()
{
	if (recursive_lock_get_recursion(&fLock) == 1) {
		// we're about to release it for the last time
		Flush();
	}
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


void
VMSAv8TranslationMap::FlushVAFromTLBByASID(addr_t va)
{
	SpinLocker locker(sAsidLock);
	if (fASID != 0) {
        asm("tlbi vae1is, %0" ::"r"(((va >> 12) & kTLBIMask) | (uint64_t(fASID) << 48)));
		asm("dsb ish"); // Wait for TLB flush to complete
	}
}


void
VMSAv8TranslationMap::PerformPteBreakBeforeMake(uint64_t* ptePtr, addr_t va)
{
	atomic_set64((int64*)ptePtr, 0);
	asm("dsb ishst"); // Ensure PTE write completed
	FlushVAFromTLBByASID(va);
}


template<typename UpdatePte>
void
VMSAv8TranslationMap::ProcessRange(phys_addr_t ptPa, int level, addr_t va, size_t size,
    vm_page_reservation* reservation, UpdatePte&& updatePte)
{
	ASSERT(level < 4);
	ASSERT(ptPa != 0);

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
		int index = (effectiveVa >> shift) & tableMask;
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

	if (memoryType & B_MTR_UC)
		type = MAIR_DEVICE_nGnRnE; // TODO: This probably should be nGnRE for PCI
	else if (memoryType & B_MTR_WC)
		type = MAIR_NORMAL_NC;
	else if (memoryType & B_MTR_WT)
		type = MAIR_NORMAL_WT;
	else if (memoryType & B_MTR_WP)
		type = MAIR_NORMAL_WT;
	else if (memoryType & B_MTR_WB)
		type = MAIR_NORMAL_WB;

	attr |= MairIndex(type) << 2;

	return attr;
}


status_t
VMSAv8TranslationMap::Map(addr_t va, phys_addr_t pa, uint32 attributes, uint32 memoryType,
	vm_page_reservation* reservation)
{
	ThreadCPUPinner pinner(thread_get_current_thread());

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((va & pageMask) == 0);
	ASSERT((pa & pageMask) == 0);
	ASSERT(ValidateVa(va));

	uint64_t attr = GetMemoryAttr(attributes, memoryType, fIsKernel);

	// During first mapping we need to allocate root table
	if (fPageTable == 0) {
		vm_page* page = vm_page_allocate_page(reservation, PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
		DEBUG_PAGE_ACCESS_END(page);
		fPageTable = page->physical_page_number << fPageBits;
	}

	ProcessRange(fPageTable, 0, va & vaMask, B_PAGE_SIZE, reservation,
		[=](uint64_t* ptePtr, uint64_t effectiveVa) {
			phys_addr_t effectivePa = effectiveVa - (va & vaMask) + pa;
			uint64_t oldPte = atomic_get64((int64*)ptePtr);
			uint64_t newPte = effectivePa | attr | kPteTypeL3Page;

			if (newPte == oldPte)
				return;

            if ((newPte & kPteValidMask) != 0 && (oldPte & kPteValidMask) != 0) {
				// ARM64 requires "break-before-make". We must set the PTE to an invalid
				// entry and flush the TLB as appropriate before we can write the new PTE.
				PerformPteBreakBeforeMake(ptePtr, effectiveVa);
			}

			// Install the new PTE
            atomic_set64((int64*)ptePtr, newPte);
			asm("dsb ishst"); // Ensure PTE write completed
		});

	return B_OK;
}


status_t
VMSAv8TranslationMap::Unmap(addr_t start, addr_t end)
{
	ThreadCPUPinner pinner(thread_get_current_thread());

	size_t size = end - start + 1;

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((start & pageMask) == 0);
	ASSERT((size & pageMask) == 0);
	ASSERT(ValidateVa(start));

	if (fPageTable == 0)
		return B_OK;

	ProcessRange(fPageTable, 0, start & vaMask, size, nullptr,
		[=](uint64_t* ptePtr, uint64_t effectiveVa) {
			uint64_t oldPte = atomic_and64((int64_t*)ptePtr, ~kPteValidMask);
			if ((oldPte & kPteValidMask) != 0) {
				asm("dsb ishst"); // Ensure PTE write completed
				FlushVAFromTLBByASID(effectiveVa);
			}
		});

	return B_OK;
}


status_t
VMSAv8TranslationMap::UnmapPage(VMArea* area, addr_t address, bool updatePageQueue)
{
	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((address & pageMask) == 0);
	ASSERT(ValidateVa(address));

	ThreadCPUPinner pinner(thread_get_current_thread());
	RecursiveLocker locker(fLock);

	uint64_t oldPte = 0;
	ProcessRange(fPageTable, 0, address & vaMask, B_PAGE_SIZE, nullptr,
		[=, &oldPte](uint64_t* ptePtr, uint64_t effectiveVa) {
			oldPte = atomic_get_and_set64((int64_t*)ptePtr, 0);
			asm("dsb ishst");
			if ((oldPte & kAttrAF) != 0)
				FlushVAFromTLBByASID(effectiveVa);
		});

	pinner.Unlock();
	locker.Detach();
	PageUnmapped(area, (oldPte & kPteAddrMask) >> fPageBits, (oldPte & kAttrAF) != 0,
		(oldPte & kAttrAPReadOnly) == 0, updatePageQueue);

	return B_OK;
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

	ThreadCPUPinner pinner(thread_get_current_thread());

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((va & pageMask) == 0);
	ASSERT(ValidateVa(va));

	ProcessRange(fPageTable, 0, va & vaMask, B_PAGE_SIZE, nullptr,
		[=](uint64_t* ptePtr, uint64_t effectiveVa) {
			uint64_t pte = atomic_get64((int64_t*)ptePtr);
			*pa = pte & kPteAddrMask;
			*flags |= PAGE_PRESENT | B_KERNEL_READ_AREA;
			if ((pte & kAttrAF) != 0)
				*flags |= PAGE_ACCESSED;
			if ((pte & kAttrAPReadOnly) == 0)
				*flags |= PAGE_MODIFIED;

			if ((pte & kAttrUXN) == 0)
				*flags |= B_EXECUTE_AREA;
			if ((pte & kAttrPXN) == 0)
				*flags |= B_KERNEL_EXECUTE_AREA;

			if ((pte & kAttrAPUserAccess) != 0)
				*flags |= B_READ_AREA;

			if ((pte & kAttrAPReadOnly) == 0 || (pte & kAttrSWDBM) != 0) {
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
	ThreadCPUPinner pinner(thread_get_current_thread());
	uint64_t attr = GetMemoryAttr(attributes, memoryType, fIsKernel);
	size_t size = end - start + 1;

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((start & pageMask) == 0);
	ASSERT((size & pageMask) == 0);
	ASSERT(ValidateVa(start));

	ProcessRange(fPageTable, 0, start & vaMask, size, nullptr,
		[=](uint64_t* ptePtr, uint64_t effectiveVa) {
			// We need to use an atomic compare-swap loop because we must
			// need to clear somes bits while setting others.
			while (true) {
				uint64_t oldPte = atomic_get64((int64_t*)ptePtr);
				uint64_t newPte = oldPte & ~kPteAttrMask;
				newPte |= attr;

                if ((uint64_t)atomic_test_and_set64((int64_t*)ptePtr, newPte, oldPte) == oldPte) {
					asm("dsb ishst"); // Ensure PTE write completed
					if ((oldPte & kAttrAF) != 0)
						FlushVAFromTLBByASID(effectiveVa);
					break;
				}
			}
		});

	return B_OK;
}


status_t
VMSAv8TranslationMap::ClearFlags(addr_t va, uint32 flags)
{
	ThreadCPUPinner pinner(thread_get_current_thread());

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((va & pageMask) == 0);
	ASSERT(ValidateVa(va));

	bool clearAF = flags & kAttrAF;
	bool setRO = flags & kAttrAPReadOnly;

	if (!clearAF && !setRO)
		return B_OK;

	ProcessRange(fPageTable, 0, va & vaMask, B_PAGE_SIZE, nullptr,
		[=](uint64_t* ptePtr, uint64_t effectiveVa) {
			if (clearAF && setRO) {
				// We need to use an atomic compare-swap loop because we must
				// need to clear one bit while setting the other.
				while (true) {
					uint64_t oldPte = atomic_get64((int64_t*)ptePtr);
					uint64_t newPte = oldPte & ~kAttrAF;
					newPte |= kAttrAPReadOnly;

                    if ((uint64_t)atomic_test_and_set64((int64_t*)ptePtr, newPte, oldPte) == oldPte)
						break;
				}
			} else if (clearAF) {
				atomic_and64((int64_t*)ptePtr, ~kAttrAPReadOnly);
			} else {
				atomic_or64((int64_t*)ptePtr, kAttrAPReadOnly);
			}
			asm("dsb ishst"); // Ensure PTE write completed
		});

	FlushVAFromTLBByASID(va);
	return B_OK;
}


bool
VMSAv8TranslationMap::ClearAccessedAndModified(
	VMArea* area, addr_t address, bool unmapIfUnaccessed, bool& _modified)
{
	RecursiveLocker locker(fLock);
	ThreadCPUPinner pinner(thread_get_current_thread());

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((address & pageMask) == 0);
	ASSERT(ValidateVa(address));

	uint64_t oldPte = 0;
	ProcessRange(fPageTable, 0, address & vaMask, B_PAGE_SIZE, nullptr,
		[=, &_modified, &oldPte](uint64_t* ptePtr, uint64_t effectiveVa) {
			// We need to use an atomic compare-swap loop because we must
			// first read the old PTE and make decisions based on the AF
			// bit to proceed.
			while (true) {
				oldPte = atomic_get64((int64_t*)ptePtr);
				uint64_t newPte = oldPte & ~kAttrAF;
				newPte |= kAttrAPReadOnly;

				// If the page has been not be accessed, then unmap it.
				if (unmapIfUnaccessed && (oldPte & kAttrAF) == 0)
					newPte = 0;

				if ((uint64_t)atomic_test_and_set64((int64_t*)ptePtr, newPte, oldPte) == oldPte)
					break;
			}
			asm("dsb ishst"); // Ensure PTE write completed
		});

	pinner.Unlock();
	_modified = (oldPte & kAttrAPReadOnly) == 0;
	if ((oldPte & kAttrAF) != 0) {
		FlushVAFromTLBByASID(address);
		return true;
	}

	if (!unmapIfUnaccessed)
		return false;

	fMapCount--;

	locker.Detach(); // UnaccessedPageUnmapped takes ownership
	phys_addr_t oldPa = oldPte & kPteAddrMask;
	UnaccessedPageUnmapped(area, oldPa >> fPageBits);
	return false;
}


void
VMSAv8TranslationMap::Flush()
{
	ThreadCPUPinner pinner(thread_get_current_thread());

	arch_cpu_global_TLB_invalidate();
}
