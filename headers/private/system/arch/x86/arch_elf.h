/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_x86_ELF_H
#define _KERNEL_ARCH_x86_ELF_H

/* relocation types */

#define R_386_NONE		0
#define R_386_32		1	/* add symbol value */
#define R_386_PC32		2	/* add PC relative symbol value */
#define R_386_GOT32		3	/* add PC relative GOT offset */
#define R_386_PLT32		4	/* add PC relative PLT offset */
#define R_386_COPY		5	/* copy data from shared object */
#define R_386_GLOB_DAT	6	/* set GOT entry to data address */
#define R_386_JMP_SLOT	7	/* set GOT entry to code address */
#define R_386_RELATIVE	8	/* add load address of shared object */
#define R_386_GOTOFF	9	/* add GOT relative symbol address */
#define R_386_GOTPC		10	/* add PC relative GOT table address */
#define R_386_TLS_DTPMOD32	35
#define R_386_TLS_DTPOFF32	36

#ifdef _BOOT_MODE
# include "../x86_64/arch_elf.h"
#endif

#endif	/* _KERNEL_ARCH_x86_ELF_H */
