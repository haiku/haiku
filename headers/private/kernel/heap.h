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
// grow by another 4MB each time the heap runs out of memory
#define HEAP_GROW_SIZE				4 * 1024 * 1024
// allocate a dedicated 1MB area for dynamic growing
#define HEAP_DEDICATED_GROW_SIZE	1 * 1024 * 1024
// use areas for allocations bigger than 1MB
#define HEAP_AREA_USE_THRESHOLD		1 * 1024 * 1024

// store size, thread and team info at the end of each allocation block
#define KERNEL_HEAP_LEAK_CHECK 0


typedef struct heap_class_s {
	const char *name;
	uint32		initial_percentage;
	size_t		max_allocation_size;
	size_t		page_size;
	size_t		min_bin_size;
	size_t		bin_alignment;
	uint32		min_count_per_page;
	size_t		max_waste_per_page;
} heap_class;

typedef struct heap_allocator_s heap_allocator;


#ifdef __cplusplus
extern "C" {
#endif

// malloc_nogrow disallows waiting for a grow to happen - only to be used by
// vm functions that may deadlock on a triggered area creation
void *malloc_nogrow(size_t size);

void *memalign(size_t alignment, size_t size);

void deferred_free(void* block);

void* malloc_referenced(size_t size);
void* malloc_referenced_acquire(void* data);
void malloc_referenced_release(void* data);

heap_allocator*	heap_create_allocator(const char* name, addr_t base,
	size_t size, const heap_class* heapClass);
void* heap_memalign(heap_allocator* heap, size_t alignment, size_t size);
status_t heap_free(heap_allocator* heap, void* address);

#if KERNEL_HEAP_LEAK_CHECK
void heap_set_get_caller(heap_allocator* heap, addr_t (*getCaller)());
#endif

status_t heap_init(addr_t heapBase, size_t heapSize);
status_t heap_init_post_sem();
status_t heap_init_post_thread();

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#include <new>

static const struct nogrow_t {
} nogrow = {};

inline void*
operator new(size_t size, const nogrow_t& nogrow) throw()
{
	return malloc_nogrow(size);
}

#endif	/* __cplusplus */


#endif	/* _KERNEL_MEMHEAP_H */
