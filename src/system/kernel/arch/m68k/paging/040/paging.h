/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_M68K_PAGING_040_PAGING_H
#define _KERNEL_ARCH_M68K_PAGING_040_PAGING_H


#include <SupportDefs.h>

#include <int.h>
#include <kernel.h>

#include <arch_040_mmu.h>

/*  (mmu_man) Implementation details on 68030 and others:

	Unlike on x86 we can't just switch the context to another team by just
	setting a register to another page directory, since we only have one
	page table containing both kernel and user address mappings.
	The 030 supports arbitrary layout of the page directory tree, including
	a 1-bit first level (2 entries top level table) that would map kernel
	and user land at a single place. But 040 and later only support a fixed
	splitting of 7/7/6 for 4K pages.

	Since 68k SMP hardware is rare enough we don't want to support them, we
	can take some shortcuts.

	As we don't want a separate user and kernel space, we'll use a single
	table. With the 7/7/6 split the 2nd level would require 32KB of tables,
	which is small enough to not want to use the list hack from x86.
	XXX: we use the hack for now, check later

	Since page directories/tables don't fit exactly a page, we stuff more
	than one per page, and allocate them all at once, and add them at the
	same time to the tree. So we guarantee all higher-level entries modulo
	the number of tables/page are either invalid or present.
 */

// 4 MB of iospace
////#define IOSPACE_SIZE (4*1024*1024)
//#define IOSPACE_SIZE (16*1024*1024)
// 256K = 2^6*4K
//#define IOSPACE_CHUNK_SIZE (NUM_PAGEENT_PER_TBL*B_PAGE_SIZE)

#define PAGE_INVALIDATE_CACHE_SIZE 64

#define FIRST_USER_PGROOT_ENT    (VADDR_TO_PRENT(USER_BASE))
#define FIRST_USER_PGDIR_ENT    (VADDR_TO_PDENT(USER_BASE))
#define NUM_USER_PGROOT_ENTS     (VADDR_TO_PRENT(ROUNDUP(USER_SIZE, B_PAGE_SIZE * 64 * 128)))
#define NUM_USER_PGDIR_ENTS     (VADDR_TO_PDENT(ROUNDUP(USER_SIZE, B_PAGE_SIZE * 64)))
#define FIRST_KERNEL_PGROOT_ENT  (VADDR_TO_PRENT(KERNEL_BASE))
#define FIRST_KERNEL_PGDIR_ENT  (VADDR_TO_PDENT(KERNEL_BASE))
#define NUM_KERNEL_PGROOT_ENTS   (VADDR_TO_PRENT(KERNEL_SIZE))
#define NUM_KERNEL_PGDIR_ENTS   (VADDR_TO_PDENT(KERNEL_SIZE))
//#define IS_KERNEL_MAP(map)		(map->arch_data->rtdir_phys == sKernelPhysicalPageRoot)


// page tables are allocated as groups, so better use them all.
static const size_t kPageTableAlignment = B_PAGE_SIZE
	 * NUM_PAGETBL_PER_PAGE * NUM_PAGEENT_PER_TBL;

static const size_t kPageDirAlignment = B_PAGE_SIZE
	* NUM_PAGEENT_PER_TBL
	* NUM_DIRTBL_PER_PAGE * NUM_DIRENT_PER_TBL;



#if 0

#define VADDR_TO_PDENT(va) (((va) / B_PAGE_SIZE) / 1024)
#define VADDR_TO_PTENT(va) (((va) / B_PAGE_SIZE) % 1024)


// page directory entry bits
#define M68K_PDE_PRESENT				0x00000001
#define M68K_PDE_WRITABLE			0x00000002
#define M68K_PDE_USER				0x00000004
#define M68K_PDE_WRITE_THROUGH		0x00000008
#define M68K_PDE_CACHING_DISABLED	0x00000010
#define M68K_PDE_ACCESSED			0x00000020
#define M68K_PDE_IGNORED1			0x00000040
#define M68K_PDE_RESERVED1			0x00000080
#define M68K_PDE_IGNORED2			0x00000100
#define M68K_PDE_IGNORED3			0x00000200
#define M68K_PDE_IGNORED4			0x00000400
#define M68K_PDE_IGNORED5			0x00000800
#define M68K_PDE_ADDRESS_MASK		0xfffff000

// page table entry bits
#define M68K_PTE_PRESENT				0x00000001
#define M68K_PTE_WRITABLE			0x00000002
#define M68K_PTE_USER				0x00000004
#define M68K_PTE_WRITE_THROUGH		0x00000008
#define M68K_PTE_CACHING_DISABLED	0x00000010
#define M68K_PTE_ACCESSED			0x00000020
#define M68K_PTE_DIRTY				0x00000040
#define M68K_PTE_PAT					0x00000080
#define M68K_PTE_GLOBAL				0x00000100
#define M68K_PTE_IGNORED1			0x00000200
#define M68K_PTE_IGNORED2			0x00000400
#define M68K_PTE_IGNORED3			0x00000800
#define M68K_PTE_ADDRESS_MASK		0xfffff000
#define M68K_PTE_PROTECTION_MASK		(M68K_PTE_WRITABLE | M68K_PTE_USER)
#define M68K_PTE_MEMORY_TYPE_MASK	(M68K_PTE_WRITE_THROUGH \
										| M68K_PTE_CACHING_DISABLED)

#define FIRST_USER_PGDIR_ENT    (VADDR_TO_PDENT(USER_BASE))
#define NUM_USER_PGDIR_ENTS     (VADDR_TO_PDENT(ROUNDUP(USER_SIZE, \
									B_PAGE_SIZE * 1024)))
#define FIRST_KERNEL_PGDIR_ENT  (VADDR_TO_PDENT(KERNEL_BASE))
#define NUM_KERNEL_PGDIR_ENTS   (VADDR_TO_PDENT(KERNEL_SIZE))


static const size_t kPageTableAlignment = 1024 * B_PAGE_SIZE;


typedef uint32 page_table_entry;
typedef uint32 page_directory_entry;

#endif // 0

#endif	// _KERNEL_ARCH_M68K_PAGING_040_PAGING_H
