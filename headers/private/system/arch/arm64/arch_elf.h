/*
 * Copyright 2019 Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_ARCH_ARM64_ELF_H
#define _KERNEL_ARCH_ARM64_ELF_H

#define	R_AARCH64_NONE		0	/* No relocation */
#define	R_AARCH64_ABS64		257	/* Absolute offset */
#define	R_AARCH64_ABS32		258	/* Absolute, 32-bit overflow check */
#define	R_AARCH64_ABS16		259	/* Absolute, 16-bit overflow check */
#define	R_AARCH64_PREL64	260	/* PC relative */
#define	R_AARCH64_PREL32	261	/* PC relative, 32-bit overflow check */
#define	R_AARCH64_PREL16	262	/* PC relative, 16-bit overflow check */
#define	R_AARCH64_COPY		1024	/* Copy data from shared object */
#define	R_AARCH64_GLOB_DAT	1025	/* Set GOT entry to data address */
#define	R_AARCH64_JUMP_SLOT	1026	/* Set GOT entry to code address */
#define	R_AARCH64_RELATIVE 	1027	/* Add load address of shared object */
#define	R_AARCH64_TLS_DTPREL64	1028
#define	R_AARCH64_TLS_DTPMOD64	1029
#define	R_AARCH64_TLS_TPREL64 	1030
#define	R_AARCH64_TLSDESC 	1031	/* Identify the TLS descriptor */
#define	R_AARCH64_IRELATIVE	1032

#endif	/* _KERNEL_ARCH_ARM64_ELF_H */
