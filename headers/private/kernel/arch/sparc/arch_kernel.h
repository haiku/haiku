/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Copyright 2019, Adrien Destugues, pulkomandy@pulkomandy.tk
** Distributed under the terms of the MIT License.
*/
#ifndef _KERNEL_ARCH_SPARC_KERNEL_H
#define _KERNEL_ARCH_SPARC_KERNEL_H

#include <arch/cpu.h>

// memory layout
#define KERNEL_BASE 0x80000000
#define KERNEL_SIZE 0x80000000
#define KERNEL_TOP  (KERNEL_BASE + (KERNEL_SIZE - 1))

/*
** User space layout is a little special:
** The user space does not completely cover the space not covered by the kernel.
** This is accomplished by starting user space at 1Mb and running to 64kb short of kernel space.
** The lower 1Mb reserved spot makes it easy to find null pointer references and guarantees a
** region wont be placed there. The 64kb region assures a user space thread cannot pass
** a buffer into the kernel as part of a syscall that would cross into kernel space.
*/
#define USER_BASE     0x100000
#define USER_BASE_ANY USER_BASE
#define USER_SIZE     (0x80000000 - (0x10000 + 0x100000))
#define USER_TOP      (USER_BASE + (USER_SIZE - 1))

#define KERNEL_USER_DATA_BASE	0x60000000
#define USER_STACK_REGION              0x70000000
#define USER_STACK_REGION_SIZE ((USER_TOP - USER_STACK_REGION) + 1)

#endif	/* _KERNEL_ARCH_SPARC_KERNEL_H */

