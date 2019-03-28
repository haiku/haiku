/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_SPARC_KERNEL_H
#define _KERNEL_ARCH_SPARC_KERNEL_H

#include <arch/cpu.h>

// memory layout
#define KERNEL_LOAD_BASE_64_BIT 0xffffffff80000000ll

// Base of the kernel address space.
// KERNEL_BASE is the base of the kernel address space. This differs from the
// address where the kernel is loaded to: the kernel is loaded in the top 2GB
// of the virtual address space as required by GCC's kernel code model. The
// whole kernel address space is the top 512GB of the address space.
#define KERNEL_BASE				0xffffff0000000000
#define KERNEL_SIZE				0x10000000000
#define KERNEL_TOP  			(KERNEL_BASE + (KERNEL_SIZE - 1))


// Userspace address space layout.
// There is a 2MB hole just before the end of the bottom half of the address
// space. This means that if userland passes in a buffer that crosses into the
// uncanonical address region, it will be caught through a page fault.
#define USER_BASE				0x100000
#define USER_BASE_ANY			USER_BASE
#define USER_SIZE				(0x800000000000 - (0x200000 + USER_BASE))
#define USER_TOP				(USER_BASE + (USER_SIZE - 1))

#define KERNEL_USER_DATA_BASE	0x7f0000000000
#define USER_STACK_REGION		0x7f0000000000
#define USER_STACK_REGION_SIZE	((USER_TOP - USER_STACK_REGION) + 1)

#endif	/* _KERNEL_ARCH_SPARC_KERNEL_H */

