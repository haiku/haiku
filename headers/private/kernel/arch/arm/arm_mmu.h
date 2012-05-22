/*
 * Copyright 2010-2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Francois Revol
 *		Ithamar R. Adema, ithamar.adema@team-embedded.nl
 *		Alexander von Gluck, kallisti5@unixzen.com
 */
#ifndef _ARCH_ARM_ARM_MMU_H
#define _ARCH_ARM_ARM_MMU_H


/*
 * generic arm mmu definitions
 */

/*
 * L1 defines for the page directory (page table walk methods)
 */
#define MMU_L1_TYPE_FAULT		0x0
	// MMU Fault
	// 31                                                  2 10
	// |                                                    |00|
#define MMU_L1_TYPE_SECTION		0x2
	// Single step table walk, 4096 entries
	// 1024K pages, 16K consumed
	// 31                   20 19   12 11  10 9 8      5 432 10
	// | page table address   |  0?   |  AP  |0| domain |1CB|10|
#define MMU_L1_TYPE_FINE		0x3
	// Three(?) step table walk, 1024 entries
	// 1K, 4K, 64K pages, 4K consumed
	// 31                           12 11     9 8      5 432 10
	// | page table address           |   0?   | domain |100|11|
#define MMU_L1_TYPE_COARSE		0x1
	// Two step table walk, 256 entries
	// 4K(Haiku), 64K pages, 1K consumed
	// 31                                  10 9 8      5 432 10
	// | page table address                  |0| domain |000|01|


// the domain is not used so and the ? is implementation specified... have not
// found it in the cortex A8 reference... so I set t to 0
// page table must obviously be on multiple of 1KB

/*
 * L2-Page descriptors... now things get really complicated...
 * there are three different types of pages large pages (64KB) and small(4KB)
 * and "small extended".
 * only small extende is used by now....
 * and there is a new and a old format of page table entries
 * I will use the old format...
 */

#define MMU_L2_TYPE_SMALLEXT 0x3
/* for new format entries (cortex-a8) */
#define MMU_L2_TYPE_SMALLNEW 0x2

// for B C and TEX see ARM arm B4-11
#define MMU_L2_FLAG_B 0x4
#define MMU_L2_FLAG_C 0x8
#define MMU_L2_FLAG_TEX 0	// use 0b000 as TEX
#define MMU_L2_FLAG_AP_RW 0x30	// allow read and write for user and system
// #define MMU_L2_FLAG_AP_


#define MMU_L1_TABLE_SIZE (4096 * 4)
	//4096 entries (one entry per MB) -> 16KB
#define MMU_L2_COARSE_TABLE_SIZE (256 * 4)
	//256 entries (one entry per 4KB) -> 1KB


/*
 * definitions for CP15 r1
 */

#define CR_R1_MMU 0x1		// enable MMU
#define CP_R1_XP 0x800000	// if XP=0 then use backwards comaptible
				// translation tables

#define VADDR_TO_PDENT(va)	((va) >> 20)
#define VADDR_TO_PTENT(va)	(((va) & 0xff000) >> 12)
#define VADDR_TO_PGOFF(va)	((va) & 0x0fff)

#define ARM_PDE_ADDRESS_MASK	0xfffffc00
#define ARM_PTE_ADDRESS_MASK	0xfffff000

#endif /* _ARCH_ARM_ARM_MMU_H */
