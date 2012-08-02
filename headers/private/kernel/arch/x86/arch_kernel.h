/*
 * Copyright 2004-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_ARCH_x86_KERNEL_H
#define _KERNEL_ARCH_x86_KERNEL_H


#ifndef _ASSEMBLER
#	include <arch/cpu.h>
#endif


#ifdef _BOOT_MODE


// 32-bit and 64-bit kernel load addresses.
#define KERNEL_LOAD_BASE		0x80000000
#define KERNEL_LOAD_BASE_64BIT	0xffffffff80000000ll


#elif defined(__x86_64__)


// Base of the kernel address space.
// KERNEL_BASE is the base of the kernel address space. This differs from the
// address where the kernel is loaded to: the kernel is loaded in the top 2GB
// of the virtual address space as required by GCC's kernel code model. The
// whole kernel address space is the top 512GB of the address space.
#define KERNEL_BASE				0xffffff8000000000
#define KERNEL_SIZE				0x8000000000
#define KERNEL_TOP  			(KERNEL_BASE + (KERNEL_SIZE - 1))
#define KERNEL_LOAD_BASE		0xffffffff80000000

// Kernel physical memory map area.
#define KERNEL_PMAP_BASE		0xffffff0000000000
#define KERNEL_PMAP_SIZE		0x8000000000

// Userspace address space layout.
// There is a 2MB hole just before the end of the bottom half of the address
// space. This means that if userland passes in a buffer that crosses into the
// uncanonical address region, it will be caught through a page fault.
#define USER_BASE				0x0
#define USER_BASE_ANY			0x100000
#define USER_SIZE				(0x800000000000 - 0x200000)
#define USER_TOP				(USER_BASE + (USER_SIZE - 1))

#define KERNEL_USER_DATA_BASE	0x7fffefff0000
#define USER_STACK_REGION		0x7ffff0000000
#define USER_STACK_REGION_SIZE	((USER_TOP - USER_STACK_REGION) + 1)


#else	// __x86_64__


// memory layout
#define KERNEL_BASE				0x80000000
#define KERNEL_SIZE				0x80000000
#define KERNEL_TOP				(KERNEL_BASE + (KERNEL_SIZE - 1))

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
#define USER_BASE				0x0
#define USER_BASE_ANY			0x100000
#define USER_SIZE				(KERNEL_BASE - 0x10000)
#define USER_TOP				(USER_BASE + (USER_SIZE - 1))

#define KERNEL_USER_DATA_BASE	0x6fff0000
#define USER_STACK_REGION		0x70000000
#define USER_STACK_REGION_SIZE	((USER_TOP - USER_STACK_REGION) + 1)


#endif	// __x86_64__

#endif	// _KERNEL_ARCH_x86_KERNEL_H
