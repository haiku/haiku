/*
 * Copyright 2024, Daniel Martin, dalmemail@gmail.com
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_METHOD_EPT_H
#define KERNEL_ARCH_X86_PAGING_METHOD_EPT_H


#include <atomic>

#include <KernelExport.h>

#include <lock.h>
#include <vm/vm_types.h>

#include "paging.h"
#include "paging/X86PagingMethod.h"
#include "paging/X86PagingStructures.h"
#include "paging/64bit/X86PagingMethod64Bit.h"


class TranslationMapPhysicalPageMapper;
class X86PhysicalPageMapper;
struct vm_page_reservation;


class X86PagingMethodEPT {
public:
								X86PagingMethodEPT();
	virtual						~X86PagingMethodEPT();

	static X86PhysicalPageMapper* PhysicalPageMapper();

	static	uint64*				PageDirectoryForAddress(uint64* virtualPML4,
									addr_t virtualAddress, bool isKernel,
									bool allocateTables,
									vm_page_reservation* reservation,
									TranslationMapPhysicalPageMapper*
										pageMapper, int32& mapCount);
	static	uint64*				PageDirectoryEntryForAddress(
									uint64* virtualPML4, addr_t virtualAddress,
									bool isKernel, bool allocateTables,
									vm_page_reservation* reservation,
									TranslationMapPhysicalPageMapper*
										pageMapper, int32& mapCount);
	static	uint64*				PageTableForAddress(uint64* virtualPML4,
									addr_t virtualAddress, bool isKernel,
									bool allocateTables,
									vm_page_reservation* reservation,
									TranslationMapPhysicalPageMapper*
										pageMapper, int32& mapCount);
	static	uint64*				PageTableEntryForAddress(uint64* virtualPML4,
									addr_t virtualAddress, bool isKernel,
									bool allocateTables,
									vm_page_reservation* reservation,
									TranslationMapPhysicalPageMapper*
										pageMapper, int32& mapCount);

	static	void				PutPageTableEntryInTable(
									uint64* entry, phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									bool globalPage);
	static	void				SetTableEntry(uint64_t* entry,
									uint64_t newEntry);
	static	uint64_t			SetTableEntryFlags(uint64_t* entryPointer,
									uint64_t flags);
	static	uint64				TestAndSetTableEntry(uint64* entry,
									uint64 newEntry, uint64 oldEntry);
	static	uint64_t			ClearTableEntry(uint64_t* entryPointer);
	static	uint64_t			ClearTableEntryFlags(uint64_t* entryPointer,
									uint64_t flags);

	static	uint64				MemoryTypeToPageTableEntryFlags(
									uint32 memoryType);

private:
			X86PhysicalPageMapper* fPhysicalPageMapper;
};


static_assert(sizeof(std::atomic<uint64_t>) == sizeof(uint64_t),
	"Non-trivial representation of atomic uint64_t.");


/*static*/ inline X86PhysicalPageMapper*
X86PagingMethodEPT::PhysicalPageMapper()
{
	return static_cast<X86PagingMethod64Bit*>(gX86PagingMethod)->PhysicalPageMapper();
}


/*static*/ inline void
X86PagingMethodEPT::SetTableEntry(uint64_t* entryPointer, uint64_t newEntry)
{
	auto& entry = *reinterpret_cast<std::atomic<uint64_t>*>(entryPointer);
	entry.store(newEntry, std::memory_order_relaxed);
}


/*static*/ inline uint64_t
X86PagingMethodEPT::SetTableEntryFlags(uint64_t* entryPointer, uint64_t flags)
{
	auto& entry = *reinterpret_cast<std::atomic<uint64_t>*>(entryPointer);
	return entry.fetch_or(flags);
}


/*static*/ inline uint64
X86PagingMethodEPT::TestAndSetTableEntry(uint64* entry, uint64 newEntry, uint64 oldEntry)
{
	return atomic_test_and_set64((int64*)entry, newEntry, oldEntry);
}


/*static*/ inline uint64_t
X86PagingMethodEPT::ClearTableEntry(uint64_t* entryPointer)
{
	auto& entry = *reinterpret_cast<std::atomic<uint64_t>*>(entryPointer);
	return entry.exchange(0);
}


/*static*/ inline uint64_t
X86PagingMethodEPT::ClearTableEntryFlags(uint64_t* entryPointer,
	uint64_t flags)
{
	auto& entry = *reinterpret_cast<std::atomic<uint64_t>*>(entryPointer);
	return entry.fetch_and(~flags);
}


/*static*/ inline uint64
X86PagingMethodEPT::MemoryTypeToPageTableEntryFlags(uint32 memoryType)
{
	switch (memoryType) {
		case B_UNCACHED_MEMORY:
			return EPT_PTE_CACHING_DISABLED;

		case B_WRITE_COMBINING_MEMORY:
			return EPT_PTE_WRITE_COMBINING;

		case B_WRITE_THROUGH_MEMORY:
			return EPT_PTE_WRITE_THROUGH;

		case B_WRITE_PROTECTED_MEMORY:
			return EPT_PTE_WRITE_PROTECT;
		case B_WRITE_BACK_MEMORY:
		default:
			return EPT_PTE_WRITE_BACK;
	}
}


#endif	// KERNEL_ARCH_X86_PAGING_METHOD_EPT_H
