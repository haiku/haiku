/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_X86_PAGING_H
#define _KERNEL_ARCH_X86_PAGING_H


#include <SupportDefs.h>

#include <heap.h>
#include <int.h>


#define VADDR_TO_PDENT(va) (((va) / B_PAGE_SIZE) / 1024)
#define VADDR_TO_PTENT(va) (((va) / B_PAGE_SIZE) % 1024)


class TranslationMapPhysicalPageMapper;


// page directory entry bits
#define X86_PDE_PRESENT				0x00000001
#define X86_PDE_WRITABLE			0x00000002
#define X86_PDE_USER				0x00000004
#define X86_PDE_WRITE_THROUGH		0x00000008
#define X86_PDE_CACHING_DISABLED	0x00000010
#define X86_PDE_ACCESSED			0x00000020
#define X86_PDE_IGNORED1			0x00000040
#define X86_PDE_RESERVED1			0x00000080
#define X86_PDE_IGNORED2			0x00000100
#define X86_PDE_IGNORED3			0x00000200
#define X86_PDE_IGNORED4			0x00000400
#define X86_PDE_IGNORED5			0x00000800
#define X86_PDE_ADDRESS_MASK		0xfffff000

// page table entry bits
#define X86_PTE_PRESENT				0x00000001
#define X86_PTE_WRITABLE			0x00000002
#define X86_PTE_USER				0x00000004
#define X86_PTE_WRITE_THROUGH		0x00000008
#define X86_PTE_CACHING_DISABLED	0x00000010
#define X86_PTE_ACCESSED			0x00000020
#define X86_PTE_DIRTY				0x00000040
#define X86_PTE_PAT					0x00000080
#define X86_PTE_GLOBAL				0x00000100
#define X86_PTE_IGNORED1			0x00000200
#define X86_PTE_IGNORED2			0x00000400
#define X86_PTE_IGNORED3			0x00000800
#define X86_PTE_ADDRESS_MASK		0xfffff000
#define X86_PTE_PROTECTION_MASK		(X86_PTE_WRITABLE | X86_PTE_USER)
#define X86_PTE_MEMORY_TYPE_MASK	(X86_PTE_WRITE_THROUGH \
										| X86_PTE_CACHING_DISABLED)


typedef uint32 page_table_entry;
typedef uint32 page_directory_entry;


struct X86PagingStructures : DeferredDeletable {
	page_directory_entry*		pgdir_virt;
	uint32						pgdir_phys;
	vint32						ref_count;
	vint32						active_on_cpus;
		// mask indicating on which CPUs the map is currently used

								X86PagingStructures();
	virtual						~X86PagingStructures();

	inline	void				AddReference();
	inline	void				RemoveReference();

	virtual	void				Delete() = 0;
};


void x86_early_prepare_page_tables(page_table_entry* pageTables, addr_t address,
		size_t size);
void x86_put_pgtable_in_pgdir(page_directory_entry* entry,
	phys_addr_t physicalPageTable, uint32 attributes);
void x86_update_all_pgdirs(int index, page_directory_entry entry);


static inline page_table_entry
set_page_table_entry(page_table_entry* entry, page_table_entry newEntry)
{
	return atomic_set((int32*)entry, newEntry);
}


static inline page_table_entry
test_and_set_page_table_entry(page_table_entry* entry,
	page_table_entry newEntry, page_table_entry oldEntry)
{
	return atomic_test_and_set((int32*)entry, newEntry, oldEntry);
}


static inline page_table_entry
clear_page_table_entry(page_table_entry* entry)
{
	return set_page_table_entry(entry, 0);
}


static inline page_table_entry
clear_page_table_entry_flags(page_table_entry* entry, uint32 flags)
{
	return atomic_and((int32*)entry, ~flags);
}


static inline page_table_entry
set_page_table_entry_flags(page_table_entry* entry, uint32 flags)
{
	return atomic_or((int32*)entry, flags);
}


inline void
X86PagingStructures::AddReference()
{
	atomic_add(&ref_count, 1);
}


inline void
X86PagingStructures::RemoveReference()
{
	if (atomic_add(&ref_count, -1) == 1)
		Delete();
}


#endif	// _KERNEL_ARCH_X86_PAGING_H
