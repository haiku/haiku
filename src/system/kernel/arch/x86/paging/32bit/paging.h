/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_X86_PAGING_32_BIT_PAGING_H
#define _KERNEL_ARCH_X86_PAGING_32_BIT_PAGING_H


#include <SupportDefs.h>

#include <int.h>
#include <kernel.h>


#define VADDR_TO_PDENT(va) (((va) / B_PAGE_SIZE) / 1024)
#define VADDR_TO_PTENT(va) (((va) / B_PAGE_SIZE) % 1024)


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

#define FIRST_USER_PGDIR_ENT    (VADDR_TO_PDENT(USER_BASE))
#define NUM_USER_PGDIR_ENTS     (VADDR_TO_PDENT(ROUNDUP(USER_SIZE, \
									B_PAGE_SIZE * 1024)))
#define FIRST_KERNEL_PGDIR_ENT  (VADDR_TO_PDENT(KERNEL_BASE))
#define NUM_KERNEL_PGDIR_ENTS   (VADDR_TO_PDENT(KERNEL_SIZE))


static const size_t kPageTableAlignment = 1024 * B_PAGE_SIZE;


typedef uint32 page_table_entry;
typedef uint32 page_directory_entry;


#endif	// _KERNEL_ARCH_X86_PAGING_32_BIT_PAGING_H
