/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_M68K_040_MMU_H
#define _KERNEL_ARCH_M68K_040_MMU_H


#include <SupportDefs.h>
#include <string.h>

#include <arch_mmu.h>

// global pages only available on 040/060
// check this
#define MMU_HAS_GLOBAL_PAGES


enum _mmu040_cache_mode {
	CM_CACHABLE_WRITETHROUGH,
	CM_CACHABLE_COPYBACK,
	CM_DISABLED_SERIALIZED,
	CM_DISABLED,
};

/* This is the normal layout of the descriptors, as per documentation.
 * When page size > 256, several bits are unused in the LSB of page
 * addresses, which we can use in addition of other unused bits.
 * the structs dedlared later reflect this for 4K pages.
 */
											// = names in MC user's manual
											// or comments
struct short_page_directory_entry {
	// upper 32 bits
	uint32 addr : 28;						// address
	uint32 accessed : 1;					// = used
	uint32 write_protect : 1;
	uint32 type : 2;						// DT_*
};

struct short_page_table_entry {
	uint32 addr : 20;						// address
	uint32 user_reserved : 1;
	uint32 global : 1;
	uint32 upa0 : 1;						// User Page Attribute 0
	uint32 upa1 : 1;						// User Page Attribute 1
	uint32 supervisor : 1;
	uint32 cache_mode : 2;
	uint32 dirty : 1;						// = modified
	uint32 accessed : 1;					// = used
	uint32 write_protect : 1;
	uint32 type : 2;
};

/* rarely used */
struct short_indirect_entry {
	// upper 32 bits
	uint32 addr : 30;						// address
	uint32 type : 2;						// DT_*
};

/* for clarity:
   - the top level page directory will be called "page root", (root or rtdir)
   - the 2nd level will be "page directory" like on x86, (pgdir)
   - the 3rd level is a "page table" as on x86. (pgtbl)
*/

/* struct types suffixed with _s */
typedef struct short_page_directory_entry page_root_entry_s;
typedef struct short_page_directory_entry page_directory_entry_s;
typedef struct short_page_table_entry page_table_entry_s;
typedef struct short_indirect_entry page_indirect_entry_s;

/* scalar storage type that maps them */
typedef uint32 page_root_entry;
typedef uint32 page_directory_entry;
typedef uint32 page_table_entry;
typedef uint32 page_indirect_entry;

#define DT_ROOT DT_VALID_4
#define DT_DIR DT_VALID_4
//#define DT_PAGE DT_PAGE :)
#define DT_INDIRECT DT_VALID_4

/* bitmask access */

/* those are valid for all entries */
#define M68K_PE_READONLY			0x00000004
#define M68K_PE_ACCESSED			0x00000008

#define M68K_PRE_READONLY		T	M68K_PE_READONLY
#define M68K_PRE_ACCESSED			M68K_PE_ACCESSED
#define M68K_PRE_ADDRESS_MASK		0xfffffff0

#define M68K_PDE_READONLY			M68K_PE_READONLY
#define M68K_PDE_ACCESSED			M68K_PE_ACCESSED
#define M68K_PDE_ADDRESS_MASK		0xfffffff0

#define M68K_PTE_READONLY			M68K_PE_READONLY
#define M68K_PTE_ACCESSED			M68K_PE_ACCESSED
#define M68K_PTE_DIRTY				0x00000010
#define M68K_PTE_CACHE_MODE_MASK	0x00000060
#define M68K_PTE_CACHE_MODE_SHIFT	5

#define M68K_PTE_SUPERVISOR			0x00000080
#define M68K_PTE_UPA1				0x00000100
#define M68K_PTE_UPA0				0x00000200
#define M68K_PTE_GLOBAL				0x00000400
#define M68K_PTE_USER_RESERVED		0x00000800
#define M68K_PTE_ADDRESS_MASK		0xfffff000


#define M68K_PIE_ADDRESS_MASK		0xfffffffc



/* default scalar values for entries */
#define DFL_ROOTENT_VAL 0x00000000
#define DFL_DIRENT_VAL 0x00000000
#define DFL_PAGEENT_VAL 0x00000000

#define NUM_ROOTENT_PER_TBL 128
#define NUM_DIRENT_PER_TBL 128
#define NUM_PAGEENT_PER_TBL 64

/* unlike x86, the root/dir/page table sizes are different than B_PAGE_SIZE
 * so we will have to fit more than one on a page to avoid wasting space.
 * We will allocate a group of tables with the one we want inside, and
 * add them from the aligned index needed, to make it easy to free them.
 */

#define SIZ_ROOTTBL (NUM_ROOTENT_PER_TBL * sizeof(page_root_entry))
#define SIZ_DIRTBL (NUM_DIRENT_PER_TBL * sizeof(page_directory_entry))
#define SIZ_PAGETBL (NUM_PAGEENT_PER_TBL * sizeof(page_table_entry))

//#define NUM_ROOTTBL_PER_PAGE (B_PAGE_SIZE / SIZ_ROOTTBL)
#define NUM_DIRTBL_PER_PAGE (B_PAGE_SIZE / SIZ_DIRTBL)
#define NUM_PAGETBL_PER_PAGE (B_PAGE_SIZE / SIZ_PAGETBL)

/* macros to get the physical page or table number and address of tables from
 * descriptors */

// TA: table address
// PN: page number
// PA: page address
// PO: page offset (offset of table in page)
// PI: page index (index of table relative to page start)

// from a root entry
#define PRE_TYPE(e)  	((e) & M68K_PE_DT_MASK)
#define PRE_TO_TA(e)	(((uint32)(e)) & ~((1<<9)-1))
#define PRE_TO_PN(e)	(((uint32)(e)) >> 12)
#define PRE_TO_PA(e)	(((uint32)(e)) & ~((1<<12)-1))
//#define PRE_TO_PO(e)	(((uint32)(e)) & ((1<<12)-1))
//#define PRE_TO_PI(e)	((((uint32)(e)) & ((1<<12)-1)) / SIZ_DIRTBL)
#define TA_TO_PREA(a)	((a) & M68K_PRE_ADDRESS_MASK)

// from a directory entry
#define PDE_TYPE(e)  	((e) & M68K_PE_DT_MASK)
#define PDE_TO_TA(e)	(((uint32)(e)) & ~((1<<8)-1))
#define PDE_TO_PN(e)	(((uint32)(e)) >> 12)
#define PDE_TO_PA(e)	(((uint32)(e)) & ~((1<<12)-1))
//#define PDE_TO_PO(e)	(((uint32)(e)) & ((1<<12)-1))
//#define PDE_TO_PI(e)	((((uint32)(e)) & ((1<<12)-1)) / SIZ_PAGETBL)
#define TA_TO_PDEA(a)	((a) & M68K_PDE_ADDRESS_MASK)

// from a table entry
#define PTE_TYPE(e)  	((e) & M68K_PE_DT_MASK)
#define PTE_TO_TA(e)	(((uint32)(e)) & ~((1<<12)-1))
#define PTE_TO_PN(e)	(((uint32)(e)) >> 12)
#define PTE_TO_PA(e)	(((uint32)(e)) & ~((1<<12)-1))
#define TA_TO_PTEA(a)	((a) & M68K_PTE_ADDRESS_MASK)

// from an indirect entry
#define PIE_TYPE(e)  	((e) & M68K_PE_DT_MASK)
#define PIE_TO_TA(e)	(((uint32)(e)) & ~((1<<2)-1))
#define PIE_TO_PN(e)	(((uint32)(e)) >> 12)
#define PIE_TO_PA(e)	(((uint32)(e)) & ~((1<<12)-1))
#define PIE_TO_PO(e)	(((uint32)(e)) & ((1<<12)-(1<<2)))
#define TA_TO_PIEA(a)	((a) & M68K_PIE_ADDRESS_MASK)

/* 7/7/6 split */
#define VADDR_TO_PRENT(va)	(((va) / B_PAGE_SIZE) / (64*128))
#define VADDR_TO_PDENT(va)	((((va) / B_PAGE_SIZE) / 64) % 128)
#define VADDR_TO_PTENT(va)	(((va) / B_PAGE_SIZE) % 64)

#endif	/* _KERNEL_ARCH_M68K_040_MMU_H */
