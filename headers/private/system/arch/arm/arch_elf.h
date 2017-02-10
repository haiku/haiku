/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_ARM_ELF_H
#define _KERNEL_ARCH_ARM_ELF_H



/* ARM relocs.  */
#define R_ARM_NONE              0       /* No reloc */
#define R_ARM_PC24              1       /* PC relative 26 bit branch */
#define R_ARM_ABS32             2       /* Direct 32 bit  */
#define R_ARM_REL32             3       /* PC relative 32 bit */
#define R_ARM_PC13              4
#define R_ARM_ABS16             5       /* Direct 16 bit */
#define R_ARM_ABS12             6       /* Direct 12 bit */
#define R_ARM_THM_ABS5          7
#define R_ARM_ABS8              8       /* Direct 8 bit */
#define R_ARM_SBREL32           9
#define R_ARM_THM_PC22          10
#define R_ARM_THM_PC8           11
#define R_ARM_AMP_VCALL9        12
#define R_ARM_SWI24             13
#define R_ARM_THM_SWI8          14
#define R_ARM_XPC25             15
#define R_ARM_THM_XPC22         16
#define R_ARM_COPY              20      /* Copy symbol at runtime */
#define R_ARM_GLOB_DAT          21      /* Create GOT entry */
#define R_ARM_JMP_SLOT          22      /* Create PLT entry */
#define R_ARM_RELATIVE          23      /* Adjust by program base */
#define R_ARM_GOTOFF            24      /* 32 bit offset to GOT */
#define R_ARM_GOTPC             25      /* 32 bit PC relative offset to GOT */
#define R_ARM_GOT32             26      /* 32 bit GOT entry */
#define R_ARM_PLT32             27      /* 32 bit PLT address */
#define R_ARM_CALL              28
#define R_ARM_JUMP24            29
#define R_ARM_GNU_VTENTRY       100
#define R_ARM_GNU_VTINHERIT     101
#define R_ARM_THM_PC11          102     /* thumb unconditional branch */
#define R_ARM_THM_PC9           103     /* thumb conditional branch */
#define R_ARM_RXPC25            249
#define R_ARM_RSBREL32          250
#define R_ARM_THM_RPC22         251
#define R_ARM_RREL32            252
#define R_ARM_RABS22            253
#define R_ARM_RPC24             254
#define R_ARM_RBASE             255






#endif	/* _KERNEL_ARCH_M68K_ELF_H */
