/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_METHOD_PAE_H
#define KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_METHOD_PAE_H


#include <KernelExport.h>

#include <lock.h>
#include <vm/vm_types.h>

#include "paging/pae/paging.h"
#include "paging/X86PagingMethod.h"
#include "paging/X86PagingStructures.h"


#if B_HAIKU_PHYSICAL_BITS == 64


class TranslationMapPhysicalPageMapper;
class X86PhysicalPageMapper;


class X86PagingMethodPAE final : public X86PagingMethod {
public:
								X86PagingMethodPAE();
	virtual						~X86PagingMethodPAE();

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

			void*				Allocate32BitPage(
									phys_addr_t& _physicalAddress,
									void*& _handle);
			void				Free32BitPage(void* address,
									phys_addr_t physicalAddress, void* handle);

	inline	X86PhysicalPageMapper* PhysicalPageMapper() const
									{ return fPhysicalPageMapper; }
	inline	TranslationMapPhysicalPageMapper* KernelPhysicalPageMapper() const
									{ return fKernelPhysicalPageMapper; }

	inline	pae_page_directory_pointer_table_entry*
									KernelVirtualPageDirPointerTable() const;
	inline	phys_addr_t			KernelPhysicalPageDirPointerTable() const;
	inline	pae_page_directory_entry* const* KernelVirtualPageDirs() const
									{ return fKernelVirtualPageDirs; }
	inline	const phys_addr_t*	KernelPhysicalPageDirs() const
									{ return fKernelPhysicalPageDirs; }

	static	X86PagingMethodPAE*	Method();

	static	void				PutPageTableInPageDir(
									pae_page_directory_entry* entry,
									phys_addr_t physicalTable,
									uint32 attributes);
	static	void				PutPageTableEntryInTable(
									pae_page_table_entry* entry,
									phys_addr_t physicalAddress,
									uint32 attributes, uint32 memoryType,
									bool globalPage);
	static	uint64_t			SetTableEntry(uint64_t* entry,
									uint64_t newEntry);
	static	uint64_t			SetTableEntryFlags(uint64_t* entry,
									uint64_t flags);
	static	uint64_t			TestAndSetTableEntry(uint64_t* entry,
									uint64_t newEntry, uint64_t oldEntry);
	static	uint64_t			ClearTableEntry(uint64_t* entry);
	static	uint64_t			ClearTableEntryFlags(uint64_t* entry,
									uint64_t flags);

	static	pae_page_directory_entry* PageDirEntryForAddress(
									pae_page_directory_entry* const* pdpt,
									addr_t address);

	static	uint64				MemoryTypeToPageTableEntryFlags(
									uint32 memoryType);

private:
			struct ToPAESwitcher;
			struct PhysicalPageSlotPool;
			friend struct PhysicalPageSlotPool;

private:
	inline	int32				_GetInitialPoolCount();

			bool				_EarlyQuery(addr_t virtualAddress,
									phys_addr_t* _physicalAddress);
			pae_page_table_entry* _EarlyGetPageTable(phys_addr_t address);

private:
			X86PhysicalPageMapper* fPhysicalPageMapper;
			TranslationMapPhysicalPageMapper* fKernelPhysicalPageMapper;

			void*				fEarlyPageStructures;
			size_t				fEarlyPageStructuresSize;
			pae_page_directory_pointer_table_entry*
									fKernelVirtualPageDirPointerTable;
			phys_addr_t			fKernelPhysicalPageDirPointerTable;
			pae_page_directory_entry* fKernelVirtualPageDirs[4];
			phys_addr_t			fKernelPhysicalPageDirs[4];
			addr_t				fFreeVirtualSlot;
			pae_page_table_entry* fFreeVirtualSlotPTE;

			mutex				fFreePagesLock;
			vm_page*			fFreePages;
			page_num_t			fFreePagesCount;
};


pae_page_directory_pointer_table_entry*
X86PagingMethodPAE::KernelVirtualPageDirPointerTable() const
{
	return fKernelVirtualPageDirPointerTable;
}


phys_addr_t
X86PagingMethodPAE::KernelPhysicalPageDirPointerTable() const
{
	return fKernelPhysicalPageDirPointerTable;
}


/*static*/ inline X86PagingMethodPAE*
X86PagingMethodPAE::Method()
{
	return static_cast<X86PagingMethodPAE*>(gX86PagingMethod);
}


/*static*/ inline pae_page_directory_entry*
X86PagingMethodPAE::PageDirEntryForAddress(
	pae_page_directory_entry* const* pdpt, addr_t address)
{
	return pdpt[address >> 30]
		+ (address / kPAEPageTableRange) % kPAEPageDirEntryCount;
}


/*static*/ inline uint64_t
X86PagingMethodPAE::SetTableEntry(uint64_t* entry, uint64_t newEntry)
{
	return atomic_get_and_set64((int64*)entry, newEntry);
}


/*static*/ inline uint64_t
X86PagingMethodPAE::SetTableEntryFlags(uint64_t* entry, uint64_t flags)
{
	return atomic_or64((int64*)entry, flags);
}


/*static*/ inline uint64_t
X86PagingMethodPAE::TestAndSetTableEntry(uint64_t* entry,
	uint64_t newEntry, uint64_t oldEntry)
{
	return atomic_test_and_set64((int64*)entry, newEntry, oldEntry);
}


/*static*/ inline uint64_t
X86PagingMethodPAE::ClearTableEntry(uint64_t* entry)
{
	return SetTableEntry(entry, 0);
}


/*static*/ inline uint64_t
X86PagingMethodPAE::ClearTableEntryFlags(uint64_t* entry, uint64_t flags)
{
	return atomic_and64((int64*)entry, ~flags);
}


/*static*/ inline uint64
X86PagingMethodPAE::MemoryTypeToPageTableEntryFlags(uint32 memoryType)
{
	switch (memoryType) {
		case B_UNCACHED_MEMORY:
			return X86_PAE_PTE_CACHING_DISABLED | X86_PAE_PTE_WRITE_THROUGH;

		case B_WRITE_COMBINING_MEMORY:
			if (x86_use_pat())
				return X86_PAE_PTE_PAT;

			// X86_PTE_WRITE_THROUGH would be closer, but the combination with
			// MTRR WC is "implementation defined" for Pentium Pro/II.
			return 0;

		case B_WRITE_THROUGH_MEMORY:
			return X86_PAE_PTE_WRITE_THROUGH;

		case B_WRITE_PROTECTED_MEMORY:
		case B_WRITE_BACK_MEMORY:
		default:
			return 0;
	}
}


#endif	// B_HAIKU_PHYSICAL_BITS == 64


#endif	// KERNEL_ARCH_X86_PAGING_PAE_X86_PAGING_METHOD_PAE_H
