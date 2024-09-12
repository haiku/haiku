/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_PPC_PAGING_460_PPC_PAGING_METHOD_460_H
#define KERNEL_ARCH_PPC_PAGING_460_PPC_PAGING_METHOD_460_H


#include <arch_mmu.h>
//#include "paging/460/paging.h"
#include "paging/PPCPagingMethod.h"
#include "paging/PPCPagingStructures.h"
#include "GenericVMPhysicalPageMapper.h"


class TranslationMapPhysicalPageMapper;


class PPCPagingMethod460 : public PPCPagingMethod {
public:
								PPCPagingMethod460();
	virtual						~PPCPagingMethod460();

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
#if 0//X86
	inline	page_table_entry*	PageHole() const
									{ return fPageHole; }
	inline	page_directory_entry* PageHolePageDir() const
									{ return fPageHolePageDir; }
	inline	uint32				KernelPhysicalPageDirectory() const
									{ return fKernelPhysicalPageDirectory; }
	inline	page_directory_entry* KernelVirtualPageDirectory() const
									{ return fKernelVirtualPageDirectory; }
	inline	PPCPhysicalPageMapper* PhysicalPageMapper() const
									{ return fPhysicalPageMapper; }
	inline	TranslationMapPhysicalPageMapper* KernelPhysicalPageMapper() const
									{ return fKernelPhysicalPageMapper; }
#endif

	inline	page_table_entry_group* PageTable() const
									{ return fPageTable; }
	inline	size_t				PageTableSize() const
									{ return fPageTableSize; }
	inline	uint32				PageTableHashMask() const
									{ return fPageTableHashMask; }

	static	PPCPagingMethod460* Method();

	void						FillPageTableEntry(page_table_entry *entry,
									uint32 virtualSegmentID,
									addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint8 protection, uint32 memoryType,
									bool secondaryHash);


#if 0//X86
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

	static	uint32				MemoryTypeToPageTableEntryFlags(
									uint32 memoryType);
#endif

private:
			//XXX:x86
			struct PhysicalPageSlotPool;
			friend struct PhysicalPageSlotPool;

private:
#if 0//X86
	static	void				_EarlyPreparePageTables(
									page_table_entry* pageTables,
									addr_t address, size_t size);
	static	status_t			_EarlyQuery(addr_t virtualAddress,
									phys_addr_t *_physicalAddress);
#endif

private:
			struct page_table_entry_group *fPageTable;
			size_t				fPageTableSize;
			uint32				fPageTableHashMask;
			area_id				fPageTableArea;

			GenericVMPhysicalPageMapper	fPhysicalPageMapper;


#if 0			//XXX:x86
			page_table_entry*	fPageHole;
			page_directory_entry* fPageHolePageDir;
			uint32				fKernelPhysicalPageDirectory;
			page_directory_entry* fKernelVirtualPageDirectory;
			PPCPhysicalPageMapper* fPhysicalPageMapper;
			TranslationMapPhysicalPageMapper* fKernelPhysicalPageMapper;
#endif
};


/*static*/ inline PPCPagingMethod460*
PPCPagingMethod460::Method()
{
	return static_cast<PPCPagingMethod460*>(gPPCPagingMethod);
}


#if 0//X86
/*static*/ inline page_table_entry
PPCPagingMethod460::SetPageTableEntry(page_table_entry* entry,
	page_table_entry newEntry)
{
	return atomic_set((int32*)entry, newEntry);
}


/*static*/ inline page_table_entry
PPCPagingMethod460::SetPageTableEntryFlags(page_table_entry* entry,
	uint32 flags)
{
	return atomic_or((int32*)entry, flags);
}


/*static*/ inline page_table_entry
PPCPagingMethod460::TestAndSetPageTableEntry(page_table_entry* entry,
	page_table_entry newEntry, page_table_entry oldEntry)
{
	return atomic_test_and_set((int32*)entry, newEntry, oldEntry);
}


/*static*/ inline page_table_entry
PPCPagingMethod460::ClearPageTableEntry(page_table_entry* entry)
{
	return SetPageTableEntry(entry, 0);
}


/*static*/ inline page_table_entry
PPCPagingMethod460::ClearPageTableEntryFlags(page_table_entry* entry, uint32 flags)
{
	return atomic_and((int32*)entry, ~flags);
}


/*static*/ inline uint32
PPCPagingMethod460::MemoryTypeToPageTableEntryFlags(uint32 memoryType)
{
	// ATM we only handle the uncacheable and write-through type explicitly. For
	// all other types we rely on the MTRRs to be set up correctly. Since we set
	// the default memory type to write-back and since the uncacheable type in
	// the PTE overrides any MTRR attribute (though, as per the specs, that is
	// not recommended for performance reasons), this reduces the work we
	// actually *have* to do with the MTRRs to setting the remaining types
	// (usually only write-combining for the frame buffer).
	switch (memoryType) {
		case B_UNCACHED_MEMORY:
			return PPC_PTE_CACHING_DISABLED | PPC_PTE_WRITE_THROUGH;

		case B_WRITE_COMBINING_MEMORY:
			// PPC_PTE_WRITE_THROUGH would be closer, but the combination with
			// MTRR WC is "implementation defined" for Pentium Pro/II.
			return 0;

		case B_WRITE_THROUGH_MEMORY:
			return PPC_PTE_WRITE_THROUGH;

		case B_WRITE_PROTECTED_MEMORY:
		case B_WRITE_BACK_MEMORY:
		default:
			return 0;
	}
}
#endif//X86


#endif	// KERNEL_ARCH_PPC_PAGING_460_PPC_PAGING_METHOD_460_H
