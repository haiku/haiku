/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_M68K_ELF_H
#define _KERNEL_ARCH_M68K_ELF_H

/* relocation types */

#define	R_68K_NONE				0
#define	R_68K_32 1            /* Direct 32 bit  */
#define	R_68K_16 2            /* Direct 16 bit  */
#define	R_68K_8 3             /* Direct 8 bit  */
#define	R_68K_PC32 4          /* PC relative 32 bit */
#define	R_68K_PC16 5          /* PC relative 16 bit */
#define	R_68K_PC8 6           /* PC relative 8 bit */
#define	R_68K_GOT32 7         /* 32 bit PC relative GOT entry */
#define	R_68K_GOT16 8         /* 16 bit PC relative GOT entry */
#define	R_68K_GOT8 9          /* 8 bit PC relative GOT entry */
#define	R_68K_GOT32O 10       /* 32 bit GOT offset */
#define	R_68K_GOT16O 11       /* 16 bit GOT offset */
#define	R_68K_GOT8O 12        /* 8 bit GOT offset */
#define	R_68K_PLT32 13        /* 32 bit PC relative PLT address */
#define	R_68K_PLT16 14        /* 16 bit PC relative PLT address */
#define	R_68K_PLT8 15         /* 8 bit PC relative PLT address */
#define	R_68K_PLT32O 16       /* 32 bit PLT offset */
#define	R_68K_PLT16O 17       /* 16 bit PLT offset */
#define	R_68K_PLT8O 18        /* 8 bit PLT offset */
#define	R_68K_COPY 19         /* Copy symbol at runtime */
#define	R_68K_GLOB_DAT 20     /* Create GOT entry */
#define	R_68K_JMP_SLOT 21     /* Create PLT entry */
#define	R_68K_RELATIVE 22     /* Adjust by program base */
/* These are GNU extensions to enable C++ vtable garbage collection.  */
#define	R_68K_GNU_VTINHERIT 23
#define	R_68K_GNU_VTENTRY 24

#endif	/* _KERNEL_ARCH_M68K_ELF_H */
