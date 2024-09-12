/*
 * Copyright 2010, Ithamar R. Adema, ithamar.adema@team-embedded.nl
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_ARM_PAGING_32_BIT_ARM_PAGING_METHOD_32_BIT_H
#define KERNEL_ARCH_ARM_PAGING_32_BIT_ARM_PAGING_METHOD_32_BIT_H


#include "paging/32bit/paging.h"
#include "paging/ARMPagingMethod.h"
#include "paging/ARMPagingStructures.h"

#include <vm/VMAddressSpace.h>


class TranslationMapPhysicalPageMapper;
class ARMPhysicalPageMapper;


class ARMPagingMethod32Bit : public ARMPagingMethod {
public:
								ARMPagingMethod32Bit();
	virtual						~ARMPagingMethod32Bit();

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

	inline	uint32				KernelPhysicalPageDirectory() const
									{ return fKernelPhysicalPageDirectory; }
	inline	page_directory_entry* KernelVirtualPageDirectory() const
									{ return fKernelVirtualPageDirectory; }
	inline	ARMPhysicalPageMapper* PhysicalPageMapper() const
									{ return fPhysicalPageMapper; }
	inline	TranslationMapPhysicalPageMapper* KernelPhysicalPageMapper() const
									{ return fKernelPhysicalPageMapper; }

	static	ARMPagingMethod32Bit* Method();

	static	void				PutPageTableInPageDir(
									page_directory_entry* entry,
									phys_addr_t pgtablePhysical,
									uint32 attributes);
	static	void				PutPageTableEntryInTable(
									page_table_entry* entry,
									phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									bool globalPage);
	static	page_table_entry	SetPageTableEntry(page_table_entry* entry,
									page_table_entry newEntry);
	static	page_table_entry	SetPageTableEntryFlags(page_table_entry* entry,
									uint32 flags);
	static	page_table_entry	TestAndSetPageTableEntry(
									page_table_entry* entry,
									page_table_entry newEntry,
									page_table_entry oldEntry);
	static	page_table_entry	ClearPageTableEntry(page_table_entry* entry);
	static	page_table_entry	ClearPageTableEntryFlags(
									page_table_entry* entry, uint32 flags);
	static	page_table_entry	SetAndClearPageTableEntryFlags(page_table_entry* entry,
									uint32 flagsToSet, uint32 flagsToClear);

	static	uint32				AttributesToPageTableEntryFlags(
									uint32 attributes);
	static	uint32				PageTableEntryFlagsToAttributes(
									uint32 pageTableEntry);
	static	uint32				MemoryTypeToPageTableEntryFlags(
									uint32 memoryType);

private:
			struct PhysicalPageSlotPool;
			friend struct PhysicalPageSlotPool;

private:
	inline	int32				_GetInitialPoolCount();

	static	void				_EarlyPreparePageTables(
									page_table_entry* pageTables,
									addr_t address, size_t size);
	static	status_t			_EarlyQuery(addr_t virtualAddress,
									phys_addr_t *_physicalAddress);

private:
			uint32				fKernelPhysicalPageDirectory;
			page_directory_entry* fKernelVirtualPageDirectory;

			ARMPhysicalPageMapper* fPhysicalPageMapper;
			TranslationMapPhysicalPageMapper* fKernelPhysicalPageMapper;
};


/*static*/ inline ARMPagingMethod32Bit*
ARMPagingMethod32Bit::Method()
{
	return static_cast<ARMPagingMethod32Bit*>(gARMPagingMethod);
}


/*static*/ inline page_table_entry
ARMPagingMethod32Bit::SetPageTableEntry(page_table_entry* entry,
	page_table_entry newEntry)
{
	return atomic_get_and_set((int32*)entry, newEntry);
}


/*static*/ inline page_table_entry
ARMPagingMethod32Bit::SetPageTableEntryFlags(page_table_entry* entry,
	uint32 flags)
{
	return atomic_or((int32*)entry, flags);
}


/*static*/ inline page_table_entry
ARMPagingMethod32Bit::TestAndSetPageTableEntry(page_table_entry* entry,
	page_table_entry newEntry, page_table_entry oldEntry)
{
	return atomic_test_and_set((int32*)entry, newEntry, oldEntry);
}


/*static*/ inline page_table_entry
ARMPagingMethod32Bit::ClearPageTableEntry(page_table_entry* entry)
{
	return SetPageTableEntry(entry, 0);
}


/*static*/ inline page_table_entry
ARMPagingMethod32Bit::ClearPageTableEntryFlags(page_table_entry* entry, uint32 flags)
{
	return atomic_and((int32*)entry, ~flags);
}


/*static*/ inline page_table_entry
ARMPagingMethod32Bit::SetAndClearPageTableEntryFlags(page_table_entry* entry, uint32 flagsToSet, uint32 flagsToClear)
{
	page_table_entry originalValue = *entry;

	while (true) {
		page_table_entry oldEntry = atomic_test_and_set((int32*)entry,
			(originalValue & ~flagsToClear) | flagsToSet, originalValue);
		if (oldEntry == originalValue)
			break;
		originalValue = oldEntry;
	}

	return originalValue;
}


/*static*/ inline uint32
ARMPagingMethod32Bit::AttributesToPageTableEntryFlags(uint32 attributes)
{
	int apFlags;

	if ((attributes & B_READ_AREA) != 0) {
		// user accessible
		apFlags = ARM_MMU_L2_FLAG_AP1;
		if ((attributes & B_WRITE_AREA) == 0)
			apFlags |= ARM_MMU_L2_FLAG_AP2;
		else
			apFlags |= ARM_MMU_L2_FLAG_HAIKU_SWDBM;
	} else if ((attributes & B_KERNEL_WRITE_AREA) == 0) {
		// kernel ro
		apFlags = ARM_MMU_L2_FLAG_AP2;
	} else {
		// kernel rw
		apFlags = ARM_MMU_L2_FLAG_HAIKU_SWDBM;
	}

	if (((attributes & B_KERNEL_EXECUTE_AREA) == 0) &&
			((attributes & B_EXECUTE_AREA) == 0)) {
		apFlags |= ARM_MMU_L2_FLAG_XN;
	}

	if ((attributes & PAGE_ACCESSED) != 0)
		apFlags |= ARM_MMU_L2_FLAG_AP0;

	if ((attributes & PAGE_MODIFIED) == 0)
		apFlags |= ARM_MMU_L2_FLAG_AP2;

	return apFlags;
}


/*static*/ inline uint32
ARMPagingMethod32Bit::PageTableEntryFlagsToAttributes(uint32 pageTableEntry)
{
	uint32 attributes;

	if ((pageTableEntry & ARM_MMU_L2_FLAG_HAIKU_SWDBM) != 0) {
		if ((pageTableEntry & ARM_MMU_L2_FLAG_AP1) != 0) {
			attributes = B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | B_WRITE_AREA;
		} else {
			attributes = B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA;
		}
	} else {
		if ((pageTableEntry & ARM_MMU_L2_FLAG_AP1) != 0)
			attributes = B_KERNEL_READ_AREA | B_READ_AREA;
		else
			attributes = B_KERNEL_READ_AREA;
	}

	if ((pageTableEntry & ARM_MMU_L2_FLAG_AP0) != 0)
		attributes |= PAGE_ACCESSED;

	if ((pageTableEntry & ARM_MMU_L2_FLAG_AP2) == 0)
		attributes |= PAGE_MODIFIED;

	if ((pageTableEntry & ARM_MMU_L2_FLAG_XN) == 0) {
		if ((attributes & B_KERNEL_READ_AREA) != 0)
			attributes |= B_KERNEL_EXECUTE_AREA;
		if ((attributes & B_READ_AREA) != 0)
			attributes |= B_EXECUTE_AREA;
	}

	return attributes;
}


/*static*/ inline uint32
ARMPagingMethod32Bit::MemoryTypeToPageTableEntryFlags(uint32 memoryType)
{
	switch (memoryType) {
		case B_UNCACHED_MEMORY:
			// Strongly Ordered
			return 0;
		case B_WRITE_COMBINING_MEMORY:
			// Shareable Device Memory
			return ARM_MMU_L2_FLAG_B;
		case B_WRITE_THROUGH_MEMORY:
			// Outer and Inner Write-Through, no Write-Allocate
			return ARM_MMU_L2_FLAG_C;
		case B_WRITE_PROTECTED_MEMORY:
		case B_WRITE_BACK_MEMORY:
		default:
			// Outer and Inner Write-Back, no Write-Allocate
			return ARM_MMU_L2_FLAG_B | ARM_MMU_L2_FLAG_C;
	}
}


#endif	// KERNEL_ARCH_ARM_PAGING_32_BIT_ARM_PAGING_METHOD_32_BIT_H
