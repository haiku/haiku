/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under terms of the MIT License.
 */
#ifndef _KERNEL_DEBUG_HEAPS_H
#define _KERNEL_DEBUG_HEAPS_H


#include <OS.h>


struct kernel_heap_implementation {
	const char*	 name;
	const size_t initial_size;
	const size_t grow_size;

	status_t	(*init)(struct kernel_args* args, addr_t base, size_t size);
	status_t	(*init_post_area)();
	status_t	(*init_post_sem)();
	status_t	(*init_post_thread)();

	void*		(*memalign)(size_t alignment, size_t size, uint32 flags);
	void*		(*realloc)(void* address, size_t newSize, uint32 flags);
	void		(*free)(void* address, uint32 flags);
};

extern kernel_heap_implementation kernel_slab_heap;
extern kernel_heap_implementation kernel_guarded_heap;
extern kernel_heap_implementation kernel_debug_heap;

bool guarded_heap_replaces_object_cache(const char* name);


#endif	// _KERNEL_DEBUG_HEAPS_H
