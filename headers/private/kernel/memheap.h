/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_MEMHEAP_H
#define _KERNEL_MEMHEAP_H


#include <OS.h>

struct kernel_args;


#define HEAP_SIZE	0x02000000
	// 32 MB heap for the kernel (!)


#ifdef __cplusplus
extern "C" {
#endif

void *memalign(size_t alignment, size_t size);

status_t heap_init(addr_t heapBase);
status_t heap_init_post_sem(struct kernel_args *args);
status_t heap_init_post_thread(struct kernel_args *args);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_MEMHEAP_H */
