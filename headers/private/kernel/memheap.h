/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_MEMHEAP_H
#define _KERNEL_MEMHEAP_H

#include <OS.h>

struct kernel_args;

#define HEAP_SIZE	0x00400000
	// 4 MB heap for the kernel


#ifdef __cplusplus
extern "C" {
#endif

void *memalign(size_t alignment, size_t size);

status_t heap_init(addr_t heapBase);
status_t heap_init_postsem(struct kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_MEMHEAP_H */
