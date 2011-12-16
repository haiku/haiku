/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_ARCH_M68K_MMU_H
#define _KERNEL_ARCH_M68K_MMU_H


#include <SupportDefs.h>
#include <string.h>


/*
 * cf.
 * "mc68030 Enhanced 32-bit Microprocessor User's Manual"
 * (3rd edition), Section 9
 * "mc68040 Enhanced 32-bit Microprocessor User's Manual"
 * Section 9
 *
 * The 030 pmmu can support up to 6-level translation tree,
 * each level using an size-selectable index from the
 * virtual address, short (4-bit) and long (8-bit) page table
 * and page entry descriptors, early tree termination, and selectable
 * page size from 256 bytes to 32K.
 * There is optionally a separate Supervisor Root Pointer to separate
 * the user and kernel trees.
 *
 * The 040 pmmu however is way more limited in its abilities.
 * It has a fixed 3-level page tree, with 7/7/6 bit splitting for
 * 4K pages. The opcodes are also different so we will need specific
 * routines. Both supervisor and root pointers must be used so we can't
 * reuse one of them.
 * 
 *
 * We settle to:
 * - 1 bit index for the first level to easily split kernel and user
 * part of the tree, with the possibility to force supervisor only for
 * the kernel tree. The use of SRP to point to a 2nd tree is avoided as
 * it is not available on 68060, plus that makes a spare 64bit reg to
 * stuff things in.
 * - 9 bit page directory
 * - 10 bit page tables
 * - 12 bit page index (4K pages, a common value).
 */



enum descriptor_types {
	DT_INVALID = 0,			// invalid entry
	DT_PAGE,				// page descriptor
	DT_VALID_4,				// short page table descriptor
	DT_VALID_8,				// long page table descriptor
};

#define M68K_PE_DT_MASK	0x00000003
#define DT_MASK	M68K_PE_DT_MASK

#if 0
/* This is the normal layout of the descriptors, as per documentation.
 * When page size > 256, several bits are unused in the LSB of page
 * addresses, which we can use in addition of other unused bits.
 * the structs dedlared later reflect this for 4K pages.
 */

struct short_page_directory_entry {
	// upper 32 bits
	uint32 type : 2;
	uint32 write_protect : 1;
	uint32 used : 1;
	uint32 address : 28;
};

struct long_page_directory_entry {
	// upper 32 bits
	uint32 type : 2;
	uint32 write_protect : 1;
	uint32 used : 1;
	uint32 _zero1 : 4;
	uint32 supervisor : 1;
	uint32 _zero2 : 1;
	uint32 _ones : 6;
	uint32 limit : 15;
	uint32 low_up : 1;						// limit is lower(1)/upper(0)
	// lower 32 bits
	uint32 unused : 4;						// 
	uint32 address : 28;
};

struct short_page_table_entry {
	uint32 type : 2;
	uint32 write_protect : 1;
	uint32 used : 1;
	uint32 modified : 1;
	uint32 _zero1 : 1;
	uint32 cache_inhibit : 1;
	uint32 _zero2 : 1;
	uint32 address : 24;
};

struct long_page_table_entry {
	// upper 32 bits
	uint32 type : 2;
	uint32 write_protect : 1;
	uint32 used : 1;
	uint32 modified : 1;
	uint32 _zero1 : 1;
	uint32 cache_inhibit : 1;
	uint32 _zero2 : 1;
	uint32 supervisor : 1;
	uint32 _zero3 : 1;
	uint32 _ones : 6;
	// limit only used on early table terminators, else unused
	uint32 limit : 15;
	uint32 low_up : 1;						// limit is lower(1)/upper(0)
	// lower 32 bits
	uint32 unused : 8;						// 
	uint32 address : 24;
};
#endif

/* ppc
extern void m68k_get_page_table(page_table_entry_group **_pageTable, size_t *_size);
extern void m68k_set_page_table(page_table_entry_group *pageTable, size_t size);
*/

#endif	/* _KERNEL_ARCH_M68K_MMU_H */
