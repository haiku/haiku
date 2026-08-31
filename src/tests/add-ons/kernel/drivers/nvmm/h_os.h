/*
 * Copyright (c) 2018-2026 Maxime Villard, m00nbsd.net
 * All rights reserved.
 *
 * This code is part of the NVMM hypervisor.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef NVMM_H_OS_H_
#define NVMM_H_OS_H_

#include <sys/types.h>
#include <sys/mman.h>
#if !defined(__HAIKU__)
#include <machine/segments.h>
#include <machine/psl.h>
#else
#include "haiku_defs.h"
#endif

/* CR0/CR4/MSR_EFER bits in use. */
#define CR0_PE		__BIT(0)
#define CR0_MP		__BIT(1)
#define CR0_TS		__BIT(3)
#define CR0_NE		__BIT(5)
#define CR0_WP		__BIT(16)
#define CR0_AM		__BIT(18)
#define CR0_PG		__BIT(31)
#define CR4_PAE		__BIT(5)
#define EFER_SCE	__BIT(0)
#define EFER_LME	__BIT(8)
#define EFER_LMA	__BIT(10)

#define PSL_MBO		0x00000002	/* Must be one bits */
#define SDT_SYS386BSY	11		/* System 386 TSS busy */

typedef uint64_t pt_entry_t;

#define PTE_P		0x0000000000000001	/* Present */
#define PTE_W		0x0000000000000002	/* Write */
#define PTE_U		0x0000000000000004	/* User */
#define PTE_PWT		0x0000000000000008	/* Write-Through */
#define PTE_PCD		0x0000000000000010	/* Cache-Disable */
#define PTE_A		0x0000000000000020	/* Accessed */
#define PTE_D		0x0000000000000040	/* Dirty */
#define PTE_PAT		0x0000000000000080	/* PAT on 4KB Pages */
#define PTE_PS		0x0000000000000080	/* Large Page Size */
#define PTE_G		0x0000000000000100	/* Global Translation */
#define PTE_AVL1	0x0000000000000200	/* Ignored by Hardware */
#define PTE_AVL2	0x0000000000000400	/* Ignored by Hardware */
#define PTE_AVL3	0x0000000000000800	/* Ignored by Hardware */
#define PTE_LGPAT	0x0000000000001000	/* PAT on Large Pages */
#define PTE_NX		0x8000000000000000	/* No Execute */

#define PAGE_SIZE	4096

/* Stolen from x86/pmap.c */
#define PATENTRY(n, type)	(type << ((n) * 8))
#define PAT_UC		0x0ULL
#define PAT_WC		0x1ULL
#define PAT_WT		0x4ULL
#define PAT_WP		0x5ULL
#define PAT_WB		0x6ULL
#define PAT_UCMINUS	0x7ULL
#define MSR_PAT_VALUE	\
	(PATENTRY(0, PAT_WB) | \
	 PATENTRY(1, PAT_WT) | \
	 PATENTRY(2, PAT_UCMINUS) | \
	 PATENTRY(3, PAT_UC) | \
	 PATENTRY(4, PAT_WB) | \
	 PATENTRY(5, PAT_WT) | \
	 PATENTRY(6, PAT_UCMINUS) | \
	 PATENTRY(7, PAT_UC))

#endif /* NVMM_H_OS_H_ */
