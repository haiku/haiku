/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef _KERNEL_ARCH_M68K_040_MMU_H
#define _KERNEL_ARCH_M68K_040_MMU_H


#include <SupportDefs.h>
#include <string.h>

#include <arch_mmu.h>

// global pages only available on 040/060
// check this
//#define MMU_HAS_GLOBAL_PAGES

/* This is the normal layout of the descriptors, as per documentation.
 * When page size > 256, several bits are unused in the LSB of page
 * addresses, which we can use in addition of other unused bits.
 * the structs dedlared later reflect this for 4K pages.
 */
											// = names in MC user's manual
											// or comments
struct short_page_directory_entry {
	// upper 32 bits
	uint32 type : 2;						// DT_*
	uint32 write_protect : 1;
	uint32 accessed : 1;					// = used
	uint32 addr : 28;						// address
};

struct long_page_directory_entry {
	// upper 32 bits
	uint32 type : 2;
	uint32 write_protect : 1;
	uint32 accessed : 1;					// = used
	uint32 _zero1 : 4;
	uint32 supervisor : 1;
	uint32 _zero2 : 1;
	uint32 _ones : 6;
	uint32 limit : 15;
	uint32 low_up : 1;						// limit is lower(1)/upper(0)
	// lower 32 bits
	uint32 unused : 4;						// 
	uint32 addr : 28;						// address
};

struct short_page_table_entry {
	uint32 type : 2;
	uint32 write_protect : 1;
	uint32 accessed : 1;					// = used
	uint32 dirty : 1;						// = modified
	uint32 _zero1 : 1;
	uint32 cache_disabled : 1;				// = cache_inhibit
	uint32 _zero2 : 1;
	uint32 addr : 24;						// address
};

struct long_page_table_entry {
	// upper 32 bits
	uint32 type : 2;
	uint32 write_protect : 1;
	uint32 accessed : 1;					// = used
	uint32 dirty : 1;						// = modified
	uint32 _zero1 : 1;
	uint32 cache_disabled : 1;				// = cache_inhibit
	uint32 _zero2 : 1;
	uint32 supervisor : 1;
	uint32 _zero3 : 1;
	uint32 _ones : 6;
	// limit only used on early table terminators, else unused
	uint32 limit : 15;
	uint32 low_up : 1;						// limit is lower(1)/upper(0)
	// lower 32 bits
	uint32 unused : 8;						// 
	uint32 addr : 24;						// address
};

/* rarely used */
struct short_indirect_entry {
	// upper 32 bits
	uint32 type : 2;						// DT_*
	uint32 addr : 30;						// address
};

struct long_indirect_entry {
	// upper 32 bits
	uint32 type : 2;
	uint32 unused1 : 30;
	// lower 32 bits
	uint32 unused2 : 2;						// 
	uint32 addr : 30;						// address
};

/* for clarity:
   - the top level page directory will be called "page root", (root or rtdir)
   - the 2nd level will be "page directory" like on x86, (pgdir)
   - the 3rd level is a "page table" as on x86. (pgtbl)
*/

typedef struct short_page_directory_entry page_root_entry;
typedef struct short_page_directory_entry page_directory_entry;
typedef struct long_page_table_entry page_table_entry;
typedef struct long_indirect_entry page_indirect_entry;

/* scalar storage type that maps them */
typedef uint32 page_root_entry_scalar;
typedef uint32 page_directory_entry_scalar;
typedef uint64 page_table_entry_scalar;
typedef uint64 page_indirect_entry_scalar;

#define DT_ROOT DT_VALID_4
#define DT_DIR DT_VALID_8
//#define DT_PAGE DT_PAGE :)
#define DT_INDIRECT DT_VALID_8

/* default scalar values for entries */
#define DFL_ROOTENT_VAL 0x00000000
#define DFL_DIRENT_VAL 0x00000000
// limit disabled, 6bits at 1
// (limit isn't used on that level, but just in case)
#define DFL_PAGEENT_VAL 0x7FFFFC0000000000LL

#define NUM_ROOTENT_PER_TBL 128
#define NUM_DIRENT_PER_TBL 128
#define NUM_PAGEENT_PER_TBL 64

/* unlike x86, the root/dir/page table sizes are different than B_PAGE_SIZE
 * so we will have to fit more than one on a page to avoid wasting space.
 * We will allocate a group of tables with the one we want inside, and
 * add them from the aligned index needed, to make it easy to free them.
 */

#define SIZ_ROOTTBL (128 * sizeof(page_root_entry))
#define SIZ_DIRTBL (128 * sizeof(page_directory_entry))
#define SIZ_PAGETBL (64 * sizeof(page_table_entry))

//#define NUM_ROOTTBL_PER_PAGE (B_PAGE_SIZE / SIZ_ROOTTBL)
#define NUM_DIRTBL_PER_PAGE (B_PAGE_SIZE / SIZ_DIRTBL)
#define NUM_PAGETBL_PER_PAGE (B_PAGE_SIZE / SIZ_PAGETBL)

/* macros to get the physical page or table number and address of tables from
 * descriptors */
#if 0
/* XXX:
   suboptimal:
   struct foo {
   int a:2;
   int b:30;
   } v = {...};
   *(int *)0 = (v.b) << 2;
   generates:
   sarl    $2, %eax
   sall    $2, %eax
   We use a cast + bitmasking, since all address fields are already shifted
*/
// from a root entry
#define PREA_TO_TA(a) ((a) << 4)
#define PREA_TO_PN(a) ((a) >> (12-4))
#define PREA_TO_PA(a) ((a) << 4)
#define TA_TO_PREA(a) ((a) >> 4)
//...
#endif

// TA: table address
// PN: page number
// PA: page address
// PO: page offset (offset of table in page)
// PI: page index (index of table relative to page start)

// from a root entry
#define PRE_TO_TA(a) ((*(uint32 *)(&(a))) & ~((1<<4)-1))
#define PRE_TO_PN(e) ((*(uint32 *)(&(e))) >> 12)
#define PRE_TO_PA(e) ((*(uint32 *)(&(e))) & ~((1<<12)-1))
//#define PRE_TO_PO(e) ((*(uint32 *)(&(e))) & ((1<<12)-1))
//#define PRE_TO_PI(e) (((*(uint32 *)(&(e))) & ((1<<12)-1)) / SIZ_DIRTBL)
#define TA_TO_PREA(a) ((a) >> 4)
// from a directory entry
#define PDE_TO_TA(a) ((*(uint32 *)(&(a))) & ~((1<<4)-1))
#define PDE_TO_PN(e) ((*(uint32 *)(&(e))) >> 12)
#define PDE_TO_PA(e) ((*(uint32 *)(&(e))) & ~((1<<12)-1))
//#define PDE_TO_PO(e) ((*(uint32 *)(&(e))) & ((1<<12)-1))
//#define PDE_TO_PI(e) (((*(uint32 *)(&(e))) & ((1<<12)-1)) / SIZ_PAGETBL)
#define TA_TO_PDEA(a) ((a) >> 4)
// from a table entry
#define PTE_TO_TA(a) ((((uint32 *)(&(a)))[1]) & ~((1<<8)-1))
#define PTE_TO_PN(e) ((((uint32 *)(&(e)))[1]) >> 12)
#define PTE_TO_PA(e) ((((uint32 *)(&(e)))[1]) & ~((1<<12)-1))
#define TA_TO_PTEA(a) ((a) >> 8)
// from an indirect entry
#define PIE_TO_TA(a) ((((uint32 *)(&(a)))[1]) & ~((1<<2)-1))
#define PIE_TO_PN(e) ((((uint32 *)(&(e)))[1]) >> 12)
#define PIE_TO_PA(e) ((((uint32 *)(&(e)))[1]) & ~((1<<12)-1))
#define PIE_TO_PO(e) ((((uint32 *)(&(e)))[1]) & ((1<<12)-(1<<2)))
#define TA_TO_PIEA(a) ((a) >> 2)

#endif	/* _KERNEL_ARCH_M68K_040_MMU_H */
