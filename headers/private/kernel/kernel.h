/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_KERNEL_H
#define _KERNEL_KERNEL_H


#include <arch_kernel.h>

/* Passed in buffers from user-space shouldn't point into the kernel */
#define CHECK_USER_ADDRESS(x) \
	((addr)(x) < KERNEL_BASE || (addr)(x) > KERNEL_TOP)

/** Size of the kernel stack */
#define KSTACK_SIZE	(PAGE_SIZE * 2)

/** Size of the stack given to teams in user space */
#define STACK_SIZE	(PAGE_SIZE * 16)

/** Size of the environmental variables space for a process */
#define ENV_SIZE	(PAGE_SIZE * 8)


#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))
#define ROUNDOWN(a, b) (((a) / (b)) * (b))


#define CHECK_BIT(a, b) ((a) & (1 << (b)))
#define SET_BIT(a, b) ((a) | (1 << (b)))
#define CLEAR_BIT(a, b) ((a) & (~(1 << (b))))


#endif	/* _KERNEL_KERNEL_H */
