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

#include <arm_mmu.h>


#define FIRST_USER_PGDIR_ENT				(VADDR_TO_PDENT(USER_BASE))
#define NUM_USER_PGDIR_ENTS					(VADDR_TO_PDENT(ROUNDUP(USER_SIZE, \
												B_PAGE_SIZE * 1024)))
#define FIRST_KERNEL_PGDIR_ENT				(VADDR_TO_PDENT(KERNEL_BASE))
#define NUM_KERNEL_PGDIR_ENTS				(VADDR_TO_PDENT(KERNEL_SIZE))


static const size_t kPageTableAlignment = 1024 * B_PAGE_SIZE;


typedef uint32 page_table_entry;
typedef uint32 page_directory_entry;


#endif	// _KERNEL_ARCH_ARM_PAGING_32_BIT_PAGING_H
