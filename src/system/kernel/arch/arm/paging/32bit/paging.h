/*
 * Copyright 2010, Ithamar R. Adema, ithamar.adema@team-embedded.nl
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_ARM_PAGING_32_BIT_PAGING_H
#define _KERNEL_ARCH_ARM_PAGING_32_BIT_PAGING_H


#include <SupportDefs.h>

#include <int.h>
#include <kernel.h>

#define VADDR_TO_PDENT(va) ((va) >> 20)
#define VADDR_TO_PTENT(va) (((va) & 0xff000) >> 12)
#define VADDR_TO_PGOFF(va) ((va) & 0x0fff)

// page directory entry bits
#define ARM_PDE_TYPE_MASK			0x00000003
#define ARM_PDE_TYPE_COARSE_L2_PAGE_TABLE	0x00000001
#define ARM_PDE_TYPE_SECTION			0x00000002
#define ARM_PDE_TYPE_FINE_L2_PAGE_TABLE		0x00000003

#define ARM_PDE_ADDRESS_MASK			0xfffffc00

// page table entry bits
#define ARM_PTE_TYPE_MASK			0x00000003
#define ARM_PTE_TYPE_LARGE_PAGE			0x00000001
#define ARM_PTE_TYPE_SMALL_PAGE			0x00000002
#define ARM_PTE_TYPE_EXT_SMALL_PAGE		0x00000003

#define ARM_PTE_ADDRESS_MASK		0xfffff000

#define FIRST_USER_PGDIR_ENT    (VADDR_TO_PDENT(USER_BASE))
#define NUM_USER_PGDIR_ENTS     (VADDR_TO_PDENT(ROUNDUP(USER_SIZE, \
									B_PAGE_SIZE * 1024)))
#define FIRST_KERNEL_PGDIR_ENT  (VADDR_TO_PDENT(KERNEL_BASE))
#define NUM_KERNEL_PGDIR_ENTS   (VADDR_TO_PDENT(KERNEL_SIZE))


static const size_t kPageTableAlignment = 1024 * B_PAGE_SIZE;


typedef uint32 page_table_entry;
typedef uint32 page_directory_entry;


#endif	// _KERNEL_ARCH_ARM_PAGING_32_BIT_PAGING_H
