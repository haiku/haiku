/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_PPC_ELF_H
#define _KERNEL_ARCH_PPC_ELF_H

/* relocation types */

#define	R_PPC_NONE				0
#define	R_PPC_ADDR32			1
#define	R_PPC_ADDR24			2
#define	R_PPC_ADDR16			3
#define	R_PPC_ADDR16_LO			4
#define	R_PPC_ADDR16_HI			5
#define	R_PPC_ADDR16_HA			6
#define	R_PPC_ADDR14			7
#define	R_PPC_ADDR14_BRTAKEN	8
#define	R_PPC_ADDR14_BRNTAKEN	9
#define	R_PPC_REL24				10
#define	R_PPC_REL14				11
#define	R_PPC_REL14_BRTAKEN		12
#define	R_PPC_REL14_BRNTAKEN	13
#define	R_PPC_GOT16				14
#define	R_PPC_GOT16_LO			15
#define	R_PPC_GOT16_HI			16
#define	R_PPC_GOT16_HA			17
#define	R_PPC_PLTREL24			18
#define	R_PPC_COPY				19
#define	R_PPC_GLOB_DAT			20
#define	R_PPC_JMP_SLOT			21
#define	R_PPC_RELATIVE			22
#define	R_PPC_LOCAL24PC			23
#define	R_PPC_UADDR32			24
#define	R_PPC_UADDR16			25
#define	R_PPC_REL32				26
#define	R_PPC_PLT32				27
#define	R_PPC_PLTREL32			28
#define	R_PPC_PLT16_LO			29
#define	R_PPC_PLT16_HI			30
#define	R_PPC_PLT16_HA			31
#define	R_PPC_SDAREL16			32
#define	R_PPC_SECTOFF			33
#define	R_PPC_SECTOFF_LO		34
#define	R_PPC_SECTOFF_HI		35
#define	R_PPC_SECTOFF_HA		36
#define	R_PPC_ADDR30			37

#endif	/* _KERNEL_ARCH_PPC_ELF_H */
