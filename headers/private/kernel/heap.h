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

#include "kernel_debug_config.h"


#if USE_GUARDED_HEAP_FOR_MALLOC && USE_GUARDED_HEAP_FOR_OBJECT_CACHE

// This requires a lot of up-front memory to boot at all...
#define INITIAL_HEAP_SIZE			128 * 1024 * 1024
// ... and a lot of reserves to keep running.
#define HEAP_GROW_SIZE				128 * 1024 * 1024

#else // USE_GUARDED_HEAP_FOR_MALLOC && USE_GUARDED_HEAP_FOR_OBJECT_CACHE

// allocate 16MB initial heap for the kernel
#define INITIAL_HEAP_SIZE			16 * 1024 * 1024
// grow by another 4MB each time the heap runs out of memory
#define HEAP_GROW_SIZE				4 * 1024 * 1024
// allocate a dedicated 1MB area for dynamic growing
#define HEAP_DEDICATED_GROW_SIZE	1 * 1024 * 1024
// use areas for allocations bigger than 1MB
#define HEAP_AREA_USE_THRESHOLD		1 * 1024 * 1024

#endif // !(USE_GUARDED_HEAP_FOR_MALLOC && USE_GUARDED_HEAP_FOR_OBJECT_CACHE)


// allocation/deallocation flags for {malloc,free}_etc()
#define HEAP_DONT_WAIT_FOR_MEMORY		0x01
#define HEAP_DONT_LOCK_KERNEL_SPACE		0x02
#define HEAP_PRIORITY_VIP				0x04


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


void* memalign_etc(size_t alignment, size_t size, uint32 flags) _ALIGNED_BY_ARG(1);
void free_etc(void* address, uint32 flags);

void* memalign(size_t alignment, size_t size) _ALIGNED_BY_ARG(1);

void deferred_free(void* block);

void* malloc_referenced(size_t size);
void* malloc_referenced_acquire(void* data);
void malloc_referenced_release(void* data);

void heap_add_area(heap_allocator* heap, area_id areaID, addr_t base,
	size_t size);
heap_allocator*	heap_create_allocator(const char* name, addr_t base,
	size_t size, const heap_class* heapClass, bool allocateOnHeap);
void* heap_memalign(heap_allocator* heap, size_t alignment, size_t size) _ALIGNED_BY_ARG(2);
status_t heap_free(heap_allocator* heap, void* address);

#if KERNEL_HEAP_LEAK_CHECK
void heap_set_get_caller(heap_allocator* heap, addr_t (*getCaller)());
#endif

status_t heap_init(addr_t heapBase, size_t heapSize);
status_t heap_init_post_area();
status_t heap_init_post_sem();
status_t heap_init_post_thread();

#ifdef __cplusplus
}
#endif


static inline void*
malloc_etc(size_t size, uint32 flags)
{
	return memalign_etc(0, size, flags);
}


#ifdef __cplusplus

#include <new>

#include <util/SinglyLinkedList.h>


struct malloc_flags {
	uint32	flags;

	malloc_flags(uint32 flags)
		:
		flags(flags)
	{
	}

	malloc_flags(const malloc_flags& other)
		:
		flags(other.flags)
	{
	}
};


inline void*
operator new(size_t size, const malloc_flags& flags) throw()
{
	return malloc_etc(size, flags.flags);
}


inline void*
operator new[](size_t size, const malloc_flags& flags) throw()
{
	return malloc_etc(size, flags.flags);
}


class DeferredDeletable : public SinglyLinkedListLinkImpl<DeferredDeletable> {
public:
	virtual						~DeferredDeletable();
};


void deferred_delete(DeferredDeletable* deletable);


#endif	/* __cplusplus */


#endif	/* _KERNEL_MEMHEAP_H */
