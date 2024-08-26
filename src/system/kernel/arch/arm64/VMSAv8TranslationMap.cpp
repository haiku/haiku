/*
 * Copyright 2022 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#include "VMSAv8TranslationMap.h"

#include <util/AutoLock.h>
#include <util/ThreadAutoLock.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>


static constexpr uint64_t kPteAddrMask = (((1UL << 36) - 1) << 12);
static constexpr uint64_t kPteAttrMask = ~(kPteAddrMask | 0x3);

static constexpr uint64_t kPteTypeMask = 0x3;
static constexpr uint64_t kPteTypeL012Table = 0x3;
static constexpr uint64_t kPteTypeL012Block = 0x1;
static constexpr uint64_t kPteTypeL3Page = 0x3;

static constexpr uint64_t kAttrSWDBM = (1UL << 55);
static constexpr uint64_t kAttrUXN = (1UL << 54);
static constexpr uint64_t kAttrPXN = (1UL << 53);
static constexpr uint64_t kAttrDBM = (1UL << 51);
static constexpr uint64_t kAttrNG = (1UL << 11);
static constexpr uint64_t kAttrAF = (1UL << 10);
static constexpr uint64_t kAttrSHInnerShareable = (3UL << 8);
static constexpr uint64_t kAttrAPReadOnly = (1UL << 7);
static constexpr uint64_t kAttrAPUserAccess = (1UL << 6);

static constexpr uint64_t kTLBIMask = ((1UL << 44) - 1);

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


uint64_t
VMSAv8TranslationMap::MakeBlock(phys_addr_t pa, int level, uint64_t attr)
{
	ASSERT(level >= fMinBlockLevel && level < 4);

	return pa | attr | (level == 3 ? 0x3 : 0x1);
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
VMSAv8TranslationMap::MakeTable(
	phys_addr_t ptPa, int level, int index, vm_page_reservation* reservation)
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
		ASSERT(type != kPteTypeL012Block);

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
VMSAv8TranslationMap::MapRange(phys_addr_t ptPa, int level, addr_t va, phys_addr_t pa, size_t size,
	VMSAv8TranslationMap::VMAction action, uint64_t attr, vm_page_reservation* reservation)
{
	ASSERT(level < 4);
	ASSERT(ptPa != 0);
	ASSERT(reservation != NULL || action != VMAction::MAP);

	int tableBits = fPageBits - 3;
	uint64_t tableMask = (1UL << tableBits) - 1;

	int shift = tableBits * (3 - level) + fPageBits;
	uint64_t entrySize = 1UL << shift;

	uint64_t entryMask = entrySize - 1;
	uint64_t nextVa = va;
	uint64_t end = va + size;
	int index;

	// Handle misaligned header that straddles entry boundary in next-level table
	if ((va & entryMask) != 0) {
		uint64_t aligned = (va & ~entryMask) + entrySize;
		if (end > aligned) {
			index = (va >> shift) & tableMask;
			phys_addr_t table = MakeTable(ptPa, level, index, reservation);
			MapRange(table, level + 1, va, pa, aligned - va, action, attr, reservation);
			nextVa = aligned;
		}
	}

	// Handle fully aligned and appropriately sized chunks
	while (nextVa + entrySize <= end) {
		phys_addr_t targetPa = pa + (nextVa - va);
		index = (nextVa >> shift) & tableMask;

		bool blockAllowed = false;
		if (action == VMAction::MAP)
			blockAllowed = (level >= fMinBlockLevel && (targetPa & entryMask) == 0);
		if (action == VMAction::SET_ATTR || action == VMAction::CLEAR_FLAGS)
			blockAllowed = (MakeTable(ptPa, level, index, NULL) == 0);
		if (action == VMAction::UNMAP)
			blockAllowed = true;

		if (blockAllowed) {
			// Everything is aligned, we can make block mapping there
			uint64_t* pte = TableFromPa(ptPa) + index;

		retry:
			uint64_t oldPte = atomic_get64((int64*) pte);

			if (action == VMAction::MAP || (oldPte & 0x1) != 0) {
				uint64_t newPte = 0;
				if (action == VMAction::MAP) {
					newPte = MakeBlock(targetPa, level, attr);
				} else if (action == VMAction::SET_ATTR) {
					newPte = MakeBlock(oldPte & kPteAddrMask, level, MoveAttrFlags(attr, oldPte));
				} else if (action == VMAction::CLEAR_FLAGS) {
					newPte = MakeBlock(oldPte & kPteAddrMask, level, ClearAttrFlags(oldPte, attr));
				} else if (action == VMAction::UNMAP) {
					newPte = 0;
					tmp_pte = oldPte;
				}

				// FIXME: this might not be enough on real hardware with SMP for some cases
				if ((uint64_t) atomic_test_and_set64((int64*) pte, newPte, oldPte) != oldPte)
					goto retry;

				if (level < 3 && (oldPte & 0x3) == 0x3) {
					// If we're replacing existing pagetable clean it up
					FreeTable(oldPte & kPteAddrMask, nextVa, level + 1,
						[](int level, uint64_t oldPte) {});
				}
			}
		} else {
			// Otherwise handle mapping in next-level table
			phys_addr_t table = MakeTable(ptPa, level, index, reservation);
			MapRange(table, level + 1, nextVa, targetPa, entrySize, action, attr, reservation);
		}
		nextVa += entrySize;
	}

	// Handle misaligned tail area (or entirety of small area) in next-level table
	if (nextVa < end) {
		index = (nextVa >> shift) & tableMask;
		phys_addr_t table = MakeTable(ptPa, level, index, reservation);
		MapRange(
			table, level + 1, nextVa, pa + (nextVa - va), end - nextVa, action, attr, reservation);
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
VMSAv8TranslationMap::ClearAttrFlags(uint64_t attr, uint32 flags)
{
	attr &= kPteAttrMask;

	if ((flags & PAGE_ACCESSED) != 0)
		attr &= ~kAttrAF;

	if ((flags & PAGE_MODIFIED) != 0 && (attr & kAttrSWDBM) != 0)
		attr |= kAttrAPReadOnly;

	return attr;
}


uint64_t
VMSAv8TranslationMap::MoveAttrFlags(uint64_t newAttr, uint64_t oldAttr)
{
	if ((oldAttr & kAttrAF) != 0)
		newAttr |= kAttrAF;
	if (((newAttr & oldAttr) & kAttrSWDBM) != 0 && (oldAttr & kAttrAPReadOnly) == 0)
		newAttr &= ~kAttrAPReadOnly;

	return newAttr;
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

	if (!fPageTable) {
		vm_page* page = vm_page_allocate_page(reservation, PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
		fPageTable = page->physical_page_number << fPageBits;
	}

	MapRange(
		fPageTable, fInitialLevel, va & vaMask, pa, B_PAGE_SIZE, VMAction::MAP, attr, reservation);

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

	MapRange(fPageTable, fInitialLevel, start & vaMask, 0, size, VMAction::UNMAP, 0, NULL);

	return B_OK;
}


status_t
VMSAv8TranslationMap::UnmapPage(VMArea* area, addr_t address, bool updatePageQueue)
{

	ThreadCPUPinner pinner(thread_get_current_thread());
	RecursiveLocker locker(fLock);

	// TODO: replace this kludge

	phys_addr_t pa;
	uint64_t pte;
	if (!WalkTable(fPageTable, fInitialLevel, address, &pa, &pte))
		return B_ENTRY_NOT_FOUND;

	uint64_t vaMask = (1UL << fVaBits) - 1;
	MapRange(fPageTable, fInitialLevel, address & vaMask, 0, B_PAGE_SIZE, VMAction::UNMAP, 0, NULL);

	pinner.Unlock();
	locker.Detach();
	PageUnmapped(area, pa >> fPageBits, (tmp_pte & kAttrAF) != 0, (tmp_pte & kAttrAPReadOnly) == 0,
		updatePageQueue);

	return B_OK;
}


bool
VMSAv8TranslationMap::WalkTable(
	phys_addr_t ptPa, int level, addr_t va, phys_addr_t* pa, uint64_t* rpte)
{
	int tableBits = fPageBits - 3;
	uint64_t tableMask = (1UL << tableBits) - 1;

	int shift = tableBits * (3 - level) + fPageBits;
	uint64_t entrySize = 1UL << shift;
	uint64_t entryMask = entrySize - 1;

	int index = (va >> shift) & tableMask;

	uint64_t pte = TableFromPa(ptPa)[index];
	int type = pte & 0x3;

	if ((type & 0x1) == 0)
		return false;

	uint64_t addr = pte & kPteAddrMask;
	if (level < 3) {
		if (type == 0x3) {
			return WalkTable(addr, level + 1, va, pa, rpte);
		} else {
			*pa = addr | (va & entryMask);
			*rpte = pte;
		}
	} else {
		ASSERT(type == 0x3);
		*pa = addr;
		*rpte = pte;
	}

	return true;
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
	ThreadCPUPinner pinner(thread_get_current_thread());

	ASSERT(ValidateVa(va));

	uint64_t pte = 0;
	bool ret = WalkTable(fPageTable, fInitialLevel, va, pa, &pte);

	uint32 result = 0;

	if (ret) {
		result |= PAGE_PRESENT;

		if ((pte & kAttrAF) != 0)
			result |= PAGE_ACCESSED;
		if ((pte & kAttrAPReadOnly) == 0)
			result |= PAGE_MODIFIED;

		if ((pte & kAttrUXN) == 0)
			result |= B_EXECUTE_AREA;
		if ((pte & kAttrPXN) == 0)
			result |= B_KERNEL_EXECUTE_AREA;

		result |= B_KERNEL_READ_AREA;

		if ((pte & kAttrAPUserAccess) != 0)
			result |= B_READ_AREA;

		if ((pte & kAttrAPReadOnly) == 0 || (pte & kAttrSWDBM) != 0) {
			result |= B_KERNEL_WRITE_AREA;

			if ((pte & kAttrAPUserAccess) != 0)
				result |= B_WRITE_AREA;
		}
	}

	*flags = result;
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

	size_t size = end - start + 1;

	uint64_t pageMask = (1UL << fPageBits) - 1;
	uint64_t vaMask = (1UL << fVaBits) - 1;

	ASSERT((start & pageMask) == 0);
	ASSERT((size & pageMask) == 0);
	ASSERT(ValidateVa(start));

	uint64_t attr = GetMemoryAttr(attributes, memoryType, fIsKernel);
	MapRange(fPageTable, fInitialLevel, start & vaMask, 0, size, VMAction::SET_ATTR, attr, NULL);

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

	MapRange(
		fPageTable, fInitialLevel, va & vaMask, 0, B_PAGE_SIZE, VMAction::CLEAR_FLAGS, flags, NULL);

	return B_OK;
}


bool
VMSAv8TranslationMap::ClearAccessedAndModified(
	VMArea* area, addr_t address, bool unmapIfUnaccessed, bool& _modified)
{
	panic("VMSAv8TranslationMap::ClearAccessedAndModified not implemented\n");
	return B_OK;
}


void
VMSAv8TranslationMap::Flush()
{
	ThreadCPUPinner pinner(thread_get_current_thread());

	arch_cpu_global_TLB_invalidate();
}
