/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _NEWOS_KERNEL_ARCH_SH4_KERNEL_H
#define _NEWOS_KERNEL_ARCH_SH4_KERNEL_H

#include <arch/sh4/sh4.h>

// memory layout
#define KERNEL_BASE P3_AREA
#define KERNEL_SIZE P3_AREA_LEN
#define KERNEL_TOP  (KERNEL_BASE + (KERNEL_SIZE - 1))

/*
** User space layout is a little special:
** The user space does not completely cover the space not covered by the kernel.
** This is accomplished by starting user space at 1Mb and running to 64kb short of kernel space.
** The lower 1Mb reserved spot makes it easy to find null pointer references and guarantees a
** region wont be placed there. The 64kb region assures a user space thread cannot pass
** a buffer into the kernel as part of a syscall that would cross into kernel space.
*/
#define USER_BASE   (U0_AREA + 0x100000)
#define USER_SIZE   (U0_AREA_LEN - (0x10000 + 0x100000))
#define USER_TOP    (USER_BASE + USER_SIZE)

#define USER_STACK_REGION (USER_BASE + USER_SIZE - USER_STACK_REGION_SIZE)
#define USER_STACK_REGION_SIZE 0x10000000

#endif
