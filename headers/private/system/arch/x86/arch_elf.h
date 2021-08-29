/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_x86_ELF_H
#define _KERNEL_ARCH_x86_ELF_H

/* relocation types */

#define	R_386_NONE			0	/* No relocation. */
#define	R_386_32			1	/* Add symbol value. */
#define	R_386_PC32			2	/* Add PC-relative symbol value. */
#define	R_386_GOT32			3	/* Add PC-relative GOT offset. */
#define	R_386_PLT32			4	/* Add PC-relative PLT offset. */
#define	R_386_COPY			5	/* Copy data from shared object. */
#define	R_386_GLOB_DAT		6	/* Set GOT entry to data address. */
#define	R_386_JMP_SLOT		7	/* Set GOT entry to code address. */
#define	R_386_RELATIVE		8	/* Add load address of shared object. */
#define	R_386_GOTOFF		9	/* Add GOT-relative symbol address. */
#define	R_386_GOTPC			10	/* Add PC-relative GOT table address. */
#define	R_386_32PLT			11
#define	R_386_TLS_TPOFF		14	/* Negative offset in static TLS block */
#define	R_386_TLS_IE		15	/* Absolute address of GOT for -ve static TLS */
#define	R_386_TLS_GOTIE		16	/* GOT entry for negative static TLS block */
#define	R_386_TLS_LE		17	/* Negative offset relative to static TLS */
#define	R_386_TLS_GD		18	/* 32 bit offset to GOT (index,off) pair */
#define	R_386_TLS_LDM		19	/* 32 bit offset to GOT (index,zero) pair */
#define	R_386_16			20
#define	R_386_PC16			21
#define	R_386_8				22
#define	R_386_PC8			23
#define	R_386_TLS_GD_32		24	/* 32 bit offset to GOT (index,off) pair */
#define	R_386_TLS_GD_PUSH	25	/* pushl instruction for Sun ABI GD sequence */
#define	R_386_TLS_GD_CALL	26	/* call instruction for Sun ABI GD sequence */
#define	R_386_TLS_GD_POP	27	/* popl instruction for Sun ABI GD sequence */
#define	R_386_TLS_LDM_32	28	/* 32 bit offset to GOT (index,zero) pair */
#define	R_386_TLS_LDM_PUSH	29	/* pushl instruction for Sun ABI LD sequence */
#define	R_386_TLS_LDM_CALL	30	/* call instruction for Sun ABI LD sequence */
#define	R_386_TLS_LDM_POP	31	/* popl instruction for Sun ABI LD sequence */
#define	R_386_TLS_LDO_32	32	/* 32 bit offset from start of TLS block */
#define	R_386_TLS_IE_32		33	/* 32 bit offset to GOT static TLS offset entry */
#define	R_386_TLS_LE_32		34	/* 32 bit offset within static TLS block */
#define	R_386_TLS_DTPMOD32	35	/* GOT entry containing TLS index */
#define	R_386_TLS_DTPOFF32	36	/* GOT entry containing TLS offset */
#define	R_386_TLS_TPOFF32	37	/* GOT entry of -ve static TLS offset */
#define	R_386_SIZE32		38
#define	R_386_TLS_GOTDESC	39
#define	R_386_TLS_DESC_CALL	40
#define	R_386_TLS_DESC		41
#define	R_386_IRELATIVE		42	/* PLT entry resolved indirectly at runtime */
#define	R_386_GOT32X		43

#ifdef _BOOT_MODE
# include "../x86_64/arch_elf.h"
#endif

#endif	/* _KERNEL_ARCH_x86_ELF_H */
