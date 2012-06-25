/*
 * Copyright 2004-2008, Haiku Inc. All rights reserved.
 * Distributes under the terms of the MIT license.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_KERNEL_H
#define _KERNEL_ARCH_x86_KERNEL_H

#ifndef _ASSEMBLER
#	include <arch/cpu.h>
#endif

// memory layout
#define KERNEL_BASE 0x80000000
#define KERNEL_SIZE 0x80000000
#define KERNEL_TOP  (KERNEL_BASE + (KERNEL_SIZE - 1))

#ifdef _BOOT_MODE
# define KERNEL_BASE_64BIT 0xffffffff80000000
#endif

/* User space layout is a little special:
 * The user space does not completely cover the space not covered by the
 * kernel. There is a gap of 64kb between the user and kernel space. The 64kb
 * region assures a user space thread cannot pass a buffer into the kernel as
 * part of a syscall that would cross into kernel space.
 * Furthermore no areas are placed in the lower 1Mb unless the application
 * explicitly requests it to find null pointer references.
 * TODO: introduce the 1Mb lower barrier again - it's only used for vm86 mode,
 *	and this should be moved into the kernel (and address space) completely.
 */
#define USER_BASE     0x00
#define USER_BASE_ANY 0x100000
#define USER_SIZE     (KERNEL_BASE - 0x10000)
#define USER_TOP      (USER_BASE + USER_SIZE)

#define KERNEL_USER_DATA_BASE	0x6fff0000
#define USER_STACK_REGION		0x70000000
#define USER_STACK_REGION_SIZE	(USER_TOP - USER_STACK_REGION)

#endif	/* _KERNEL_ARCH_x86_KERNEL_H */
