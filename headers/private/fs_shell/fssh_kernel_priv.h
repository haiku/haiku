/*
 * Copyright 2002-2005, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _FSSH_KERNEL_H
#define _FSSH_KERNEL_H


#include "fssh_os.h"


/* Passed in buffers from user-space shouldn't point into the kernel */
#if 0
#define IS_USER_ADDRESS(x) \
	((addr_t)(x) < KERNEL_BASE || (addr_t)(x) > KERNEL_TOP)

#define IS_KERNEL_ADDRESS(x) \
	((addr_t)(x) >= KERNEL_BASE && (addr_t)(x) <= KERNEL_TOP)
#endif	// 0

#define DEBUG_KERNEL_STACKS
	// Note, debugging kernel stacks doesn't really work yet. Since the
	// interrupt will also try to use the stack on a page fault, all
	// you get is a double fault.
	// At least, you then know that the stack overflows in this case :)

/** Size of the kernel stack */
#ifndef DEBUG_KERNEL_STACKS
#	define KERNEL_STACK_SIZE		(B_PAGE_SIZE * 2)	// 8 kB
#else
#	define KERNEL_STACK_SIZE		(B_PAGE_SIZE * 3)	// 8 kB + one guard page
#endif
#define KERNEL_STACK_GUARD_PAGES	1

/** Size of the stack given to teams in user space */
#define USER_MAIN_THREAD_STACK_SIZE	(16 * 1024 * 1024)	// 16 MB
#define USER_STACK_SIZE				(256 * 1024)		// 256 kB
#define USER_STACK_GUARD_PAGES		4					// 16 kB

/** Size of the environmental variables space for a process */
#define ENV_SIZE	(B_PAGE_SIZE * 8)


#define ROUNDUP(a, b) (((a) + ((b)-1)) & ~((b)-1))
#define ROUNDOWN(a, b) (((a) / (b)) * (b))


#define CHECK_BIT(a, b) ((a) & (1 << (b)))
#define SET_BIT(a, b) ((a) | (1 << (b)))
#define CLEAR_BIT(a, b) ((a) & (~(1 << (b))))


#endif	/* _FSSH_KERNEL_H */
