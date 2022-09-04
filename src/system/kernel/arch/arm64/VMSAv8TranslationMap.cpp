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

static constexpr uint64_t kAttrSWDBM = (1UL << 55);
static constexpr uint64_t kAttrUXN = (1UL << 54);
static constexpr uint64_t kAttrPXN = (1UL << 53);
static constexpr uint64_t kAttrDBM = (1UL << 51);
static constexpr uint64_t kAttrNG = (1UL << 11);
static constexpr uint64_t kAttrAF = (1UL << 10);
static constexpr uint64_t kAttrSH1 = (1UL << 9);
static constexpr uint64_t kAttrSH0 = (1UL << 8);
static constexpr uint64_t kAttrAP2 = (1UL << 7);
static constexpr uint64_t kAttrAP1 = (1UL << 6);

uint32_t VMSAv8TranslationMap::fHwFeature;
uint64_t VMSAv8TranslationMap::fMair;


VMSAv8TranslationMap::VMSAv8TranslationMap(
	bool kernel, phys_addr_t pageTable, int pageBits, int vaBits, int minBlockLevel)
	:
	fIsKernel(kernel),
	fPageTable(pageTable),
	fPageBits(pageBits),
	fVaBits(vaBits),
	fMinBlockLevel(minBlockLevel)
{
	dprintf("VMSAv8TranslationMap\n");

	fInitialLevel = CalcStartLevel(fVaBits, fPageBits);
}


VMSAv8TranslationMap::~VMSAv8TranslationMap()
{
	dprintf("~VMSAv8TranslationMap\n");

	// FreeTable(fPageTable, fInitialLevel);
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


void
VMSAv8TranslationMap::FreeTable(phys_addr_t ptPa, int level)
{
	ASSERT(level < 3);

	if (level + 1 < 3) {
		int tableBits = fPageBits - 3;
		uint64_t tableSize = 1UL << tableBits;

		uint64_t* pt = TableFromPa(ptPa);
		for (uint64_t i = 0; i < tableSize; i++) {
			uint64_t pte = pt[i];
			if ((pte & 0x3) == 0x3) {
				FreeTable(pte & kPteAddrMask, level + 1);
			}
		}
	}

	vm_page* page = vm_lookup_page(ptPa >> fPageBits);
	vm_page_set_state(page, PAGE_STATE_FREE);
}


phys_addr_t
VMSAv8TranslationMap::MakeTable(
	phys_addr_t ptPa, int level, int index, vm_page_reservation* reservation)
{
	if (level == 3)
		return 0;

	uint64_t* pte = &TableFromPa(ptPa)[index];
	vm_page* page = NULL;

retry:
	uint64_t oldPte = atomic_get64((int64*) pte);

	int type = oldPte & 0x3;
	if (type == 0x3) {
		return oldPte & kPteAddrMask;
	} else if (reservation != NULL) {
		if (page == NULL)
			page = vm_page_allocate_page(reservation, PAGE_STATE_WIRED | VM_PAGE_ALLOC_CLEAR);
		phys_addr_t newTablePa = page->physical_page_number << fPageBits;

		if (type == 0x1) {
			// If we're replacing existing block mapping convert it to pagetable
			int tableBits = fPageBits - 3;
			int shift = tableBits * (3 - (level + 1)) + fPageBits;
			uint64_t entrySize = 1UL << shift;
			uint64_t tableSize = 1UL << tableBits;

			uint64_t* newTable = TableFromPa(newTablePa);
			uint64_t addr = oldPte & kPteAddrMask;
			uint64_t attr = oldPte & kPteAttrMask;

			for (uint64_t i = 0; i < tableSize; i++) {
				newTable[i] = MakeBlock(addr + i * entrySize, level + 1, attr);
			}
		}

		asm("dsb ish");

		// FIXME: this is not enough on real hardware with SMP
		if ((uint64_t) atomic_test_and_set64((int64*) pte, newTablePa | 0x3, oldPte) != oldPte)
			goto retry;

		return newTablePa;
	}

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
			uint64_t* pte = &TableFromPa(ptPa)[index];

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
					FreeTable(oldPte & kPteAddrMask, level);
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
		attr |= kAttrAP2;

	return attr;
}


uint64_t
VMSAv8TranslationMap::MoveAttrFlags(uint64_t newAttr, uint64_t oldAttr)
{
	if ((oldAttr & kAttrAF) != 0)
		newAttr |= kAttrAF;
	if (((newAttr & oldAttr) & kAttrSWDBM) != 0 && (oldAttr & kAttrAP2) == 0)
		newAttr &= ~kAttrAP2;

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

	if ((attributes & B_READ_AREA) == 0) {
		attr |= kAttrAP2;
		if ((attributes & B_KERNEL_WRITE_AREA) != 0)
			attr |= kAttrSWDBM;
	} else {
		attr |= kAttrAP2 | kAttrAP1;
		if ((attributes & B_WRITE_AREA) != 0)
			attr |= kAttrSWDBM;
	}

	if ((fHwFeature & HW_DIRTY) != 0 && (attr & kAttrSWDBM))
		attr |= kAttrDBM;

	attr |= kAttrSH1 | kAttrSH0;

	attr |= MairIndex(MAIR_NORMAL_WB) << 2;

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
	PageUnmapped(area, pa >> fPageBits, (tmp_pte & kAttrAF) != 0, (tmp_pte & kAttrAP2) == 0,
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
		if ((pte & kAttrAP2) == 0)
			result |= PAGE_MODIFIED;

		if ((pte & kAttrUXN) == 0)
			result |= B_EXECUTE_AREA;
		if ((pte & kAttrPXN) == 0)
			result |= B_KERNEL_EXECUTE_AREA;

		result |= B_KERNEL_READ_AREA;

		if ((pte & kAttrAP1) != 0)
			result |= B_READ_AREA;

		if ((pte & kAttrAP2) == 0 || (pte & kAttrSWDBM) != 0) {
			result |= B_KERNEL_WRITE_AREA;

			if ((pte & kAttrAP1) != 0)
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
