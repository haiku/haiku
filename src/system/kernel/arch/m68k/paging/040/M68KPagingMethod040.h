/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_M68K_PAGING_32_BIT_M68K_PAGING_METHOD_32_BIT_H
#define KERNEL_ARCH_M68K_PAGING_32_BIT_M68K_PAGING_METHOD_32_BIT_H


#include "paging/040/paging.h"
#include "paging/M68KPagingMethod.h"
#include "paging/M68KPagingStructures.h"


class TranslationMapPhysicalPageMapper;
class M68KPhysicalPageMapper;


class M68KPagingMethod040 : public M68KPagingMethod {
public:
								M68KPagingMethod040();
	virtual						~M68KPagingMethod040();

	virtual	status_t			Init(kernel_args* args,
									VMPhysicalPageMapper** _physicalPageMapper);
	virtual	status_t			InitPostArea(kernel_args* args);

	virtual	status_t			CreateTranslationMap(bool kernel,
									VMTranslationMap** _map);

	virtual	status_t			MapEarly(kernel_args* args,
									addr_t virtualAddress,
									phys_addr_t physicalAddress,
									uint8 attributes,
									phys_addr_t (*get_free_page)(kernel_args*));

	virtual	bool				IsKernelPageAccessible(addr_t virtualAddress,
									uint32 protection);

	virtual void				SetPageRoot(uint32 pageRoot);


#if 0
	inline	page_table_entry*	PageHole() const
									{ return fPageHole; }
	inline	page_directory_entry* PageHolePageDir() const
									{ return fPageHolePageDir; }
#endif
	inline	uint32				KernelPhysicalPageRoot() const
									{ return fKernelPhysicalPageRoot; }
	inline	page_directory_entry* KernelVirtualPageRoot() const
									{ return fKernelVirtualPageRoot; }
	inline	M68KPhysicalPageMapper* PhysicalPageMapper() const
									{ return fPhysicalPageMapper; }
	inline	TranslationMapPhysicalPageMapper* KernelPhysicalPageMapper() const
									{ return fKernelPhysicalPageMapper; }

	static	M68KPagingMethod040* Method();

	static	void				PutPageDirInPageRoot(
									page_root_entry* entry,
									phys_addr_t pgdirPhysical,
									uint32 attributes);
	static	void				PutPageTableInPageDir(
									page_directory_entry* entry,
									phys_addr_t pgtablePhysical,
									uint32 attributes);
	static	void				PutPageTableEntryInTable(
									page_table_entry* entry,
									phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									bool globalPage);
#if 1
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
#endif

	static	uint32				MemoryTypeToPageTableEntryFlags(
									uint32 memoryType);

private:
			struct PhysicalPageSlotPool;
			friend struct PhysicalPageSlotPool;

private:
	static	void				_EarlyPreparePageTables(
									page_table_entry* pageTables,
									addr_t address, size_t size);
	static	status_t			_EarlyQuery(addr_t virtualAddress,
									phys_addr_t *_physicalAddress);

private:
#if 0
			page_table_entry*	fPageHole;
			page_directory_entry* fPageHolePageDir;
#endif
			uint32				fKernelPhysicalPageRoot;
			page_directory_entry* fKernelVirtualPageRoot;

			M68KPhysicalPageMapper* fPhysicalPageMapper;
			TranslationMapPhysicalPageMapper* fKernelPhysicalPageMapper;
};


/*static*/ inline M68KPagingMethod040*
M68KPagingMethod040::Method()
{
	return static_cast<M68KPagingMethod040*>(gM68KPagingMethod);
}


#if 1
/*static*/ inline page_table_entry
M68KPagingMethod040::SetPageTableEntry(page_table_entry* entry,
	page_table_entry newEntry)
{
	return atomic_set((int32*)entry, newEntry);
}


/*static*/ inline page_table_entry
M68KPagingMethod040::SetPageTableEntryFlags(page_table_entry* entry,
	uint32 flags)
{
	return atomic_or((int32*)entry, flags);
}


/*static*/ inline page_table_entry
M68KPagingMethod040::TestAndSetPageTableEntry(page_table_entry* entry,
	page_table_entry newEntry, page_table_entry oldEntry)
{
	return atomic_test_and_set((int32*)entry, newEntry, oldEntry);
}


/*static*/ inline page_table_entry
M68KPagingMethod040::ClearPageTableEntry(page_table_entry* entry)
{
	return SetPageTableEntry(entry, DFL_PAGEENT_VAL);
}


/*static*/ inline page_table_entry
M68KPagingMethod040::ClearPageTableEntryFlags(page_table_entry* entry, uint32 flags)
{
	return atomic_and((int32*)entry, ~flags);
}
#endif

/*static*/ inline uint32
M68KPagingMethod040::MemoryTypeToPageTableEntryFlags(uint32 memoryType)
{
	// x86:
	// ATM we only handle the uncacheable and write-through type explicitly. For
	// all other types we rely on the MTRRs to be set up correctly. Since we set
	// the default memory type to write-back and since the uncacheable type in
	// the PTE overrides any MTRR attribute (though, as per the specs, that is
	// not recommended for performance reasons), this reduces the work we
	// actually *have* to do with the MTRRs to setting the remaining types
	// (usually only write-combining for the frame buffer).
#warning M68K: Check this
	switch (memoryType) {
		case B_MTR_UC:
			return CM_DISABLED_SERIALIZED | CM_CACHABLE_WRITETHROUGH;

		case B_MTR_WC:
			return 0;

		case B_MTR_WT:
			return CM_CACHABLE_WRITETHROUGH;

		case B_MTR_WP:
		case B_MTR_WB:
		default:
			return 0;
	}
}


#endif	// KERNEL_ARCH_M68K_PAGING_32_BIT_M68K_PAGING_METHOD_32_BIT_H
