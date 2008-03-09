/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_HEAP_H
#define _KERNEL_HEAP_H

#include <OS.h>

// allocate 16MB initial heap for the kernel
#define INITIAL_HEAP_SIZE			16 * 1024 * 1024
// grow by another 8MB each time the heap runs out of memory
#define HEAP_GROW_SIZE				8 * 1024 * 1024
// allocate a dedicated 2MB area for dynamic growing
#define HEAP_DEDICATED_GROW_SIZE	2 * 1024 * 1024


#ifdef __cplusplus
extern "C" {
#endif

void *memalign(size_t alignment, size_t size);

void deferred_free(void* block);

status_t heap_init(addr_t heapBase, size_t heapSize);
status_t heap_init_post_sem();
status_t heap_init_post_thread();

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_MEMHEAP_H */
