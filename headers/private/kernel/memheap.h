/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_MEMHEAP_H
#define _KERNEL_MEMHEAP_H

#include <kernel.h>

struct kernel_args;

#define HEAP_SIZE	0x00400000
	// 4 MB heap for the kernel


int   heap_init(addr new_heap_base);
int   heap_init_postsem(struct kernel_args *ka);

#endif	/* _KERNEL_MEMHEAP_H */
