/*
 * Copyright 2014, Paweł Dziepak, pdziepak@quarnos.org.
 * Copyright 2012, Alex Smith, alex@alex-smith.me.uk.
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_METHOD_64BIT_H
#define KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_METHOD_64BIT_H


#include <atomic>

#include <KernelExport.h>

#include <lock.h>
#include <vm/vm_types.h>

#include "paging/64bit/paging.h"
#include "paging/X86PagingMethod.h"
#include "paging/X86PagingStructures.h"


class TranslationMapPhysicalPageMapper;
class X86PhysicalPageMapper;
struct vm_page_reservation;


class X86PagingMethod64Bit final : public X86PagingMethod {
public:
								X86PagingMethod64Bit();
	virtual						~X86PagingMethod64Bit();

	virtual	status_t			Init(kernel_args* args,
									VMPhysicalPageMapper** _physicalPageMapper);
	virtual	status_t			InitPostArea(kernel_args* args);

	virtual	status_t			CreateTranslationMap(bool kernel,
									VMTranslationMap** _map);

	virtual	status_t			MapEarly(kernel_args* args,
									addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint8 attributes,
									page_num_t (*get_free_page)(kernel_args*));

	virtual	bool				IsKernelPageAccessible(addr_t virtualAddress,
									uint32 protection);

	inline	X86PhysicalPageMapper* PhysicalPageMapper() const
									{ return fPhysicalPageMapper; }
	inline	TranslationMapPhysicalPageMapper* KernelPhysicalPageMapper() const
									{ return fKernelPhysicalPageMapper; }

	inline	uint64*				KernelVirtualPML4() const
									{ return fKernelVirtualPML4; }
	inline	phys_addr_t			KernelPhysicalPML4() const
									{ return fKernelPhysicalPML4; }

	static	X86PagingMethod64Bit* Method();

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
	static	void				_EnableExecutionDisable(void* dummy, int cpu);

			phys_addr_t			fKernelPhysicalPML4;
			uint64*				fKernelVirtualPML4;

			X86PhysicalPageMapper* fPhysicalPageMapper;
			TranslationMapPhysicalPageMapper* fKernelPhysicalPageMapper;
};


static_assert(sizeof(std::atomic<uint64_t>) == sizeof(uint64_t),
	"Non-trivial representation of atomic uint64_t.");


/*static*/ inline X86PagingMethod64Bit*
X86PagingMethod64Bit::Method()
{
	return static_cast<X86PagingMethod64Bit*>(gX86PagingMethod);
}


/*static*/ inline void
X86PagingMethod64Bit::SetTableEntry(uint64_t* entryPointer, uint64_t newEntry)
{
	auto& entry = *reinterpret_cast<std::atomic<uint64_t>*>(entryPointer);
	entry.store(newEntry, std::memory_order_relaxed);
}


/*static*/ inline uint64_t
X86PagingMethod64Bit::SetTableEntryFlags(uint64_t* entryPointer, uint64_t flags)
{
	auto& entry = *reinterpret_cast<std::atomic<uint64_t>*>(entryPointer);
	return entry.fetch_or(flags);
}


/*static*/ inline uint64
X86PagingMethod64Bit::TestAndSetTableEntry(uint64* entry, uint64 newEntry, uint64 oldEntry)
{
	return atomic_test_and_set64((int64*)entry, newEntry, oldEntry);
}


/*static*/ inline uint64_t
X86PagingMethod64Bit::ClearTableEntry(uint64_t* entryPointer)
{
	auto& entry = *reinterpret_cast<std::atomic<uint64_t>*>(entryPointer);
	return entry.exchange(0);
}


/*static*/ inline uint64_t
X86PagingMethod64Bit::ClearTableEntryFlags(uint64_t* entryPointer,
	uint64_t flags)
{
	auto& entry = *reinterpret_cast<std::atomic<uint64_t>*>(entryPointer);
	return entry.fetch_and(~flags);
}


/*static*/ inline uint64
X86PagingMethod64Bit::MemoryTypeToPageTableEntryFlags(uint32 memoryType)
{
	// ATM we only handle the uncacheable and write-through type explicitly. For
	// all other types we rely on the MTRRs to be set up correctly. Since we set
	// the default memory type to write-back and since the uncacheable type in
	// the PTE overrides any MTRR attribute (though, as per the specs, that is
	// not recommended for performance reasons), this reduces the work we
	// actually *have* to do with the MTRRs to setting the remaining types
	// (usually only write-combining for the frame buffer).
	switch (memoryType) {
		case B_MTR_UC:
			return X86_64_PTE_CACHING_DISABLED | X86_64_PTE_WRITE_THROUGH;

		case B_MTR_WC:
			// X86_PTE_WRITE_THROUGH would be closer, but the combination with
			// MTRR WC is "implementation defined" for Pentium Pro/II.
			return 0;

		case B_MTR_WT:
			return X86_64_PTE_WRITE_THROUGH;

		case B_MTR_WP:
		case B_MTR_WB:
		default:
			return 0;
	}
}


#endif	// KERNEL_ARCH_X86_PAGING_64BIT_X86_PAGING_METHOD_64BIT_H
