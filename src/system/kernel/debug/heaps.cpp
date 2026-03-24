/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "kernel_debug_config.h"

#if DEBUG_HEAPS

#include <stdlib.h>
#include <heap.h>

#include <vm/vm.h>
#include <vm/vm_page.h>
#include <slab/Slab.h>
#include <safemode.h>

#include "heaps.h"


//#define TRACE_HEAPS
#ifdef TRACE_HEAPS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define HEAP_SYMBOL_NAME(NAME) kernel_##NAME##_heap
#define HEAP_SYMBOL(NAME) HEAP_SYMBOL_NAME(NAME)
static kernel_heap_implementation* sHeap = &HEAP_SYMBOL(DEBUG_HEAPS_DEFAULT);


//	#pragma mark -


status_t
heap_init(struct kernel_args* args)
{
	char buffer[32];
	size_t bufferSize = sizeof(buffer);
	if (get_safemode_option_early(args, "kernel_malloc", buffer, &bufferSize) == B_OK) {
		if (strcmp(buffer, "guarded") == 0)
			sHeap = &kernel_guarded_heap;
		else if (strcmp(buffer, "debug") == 0)
			sHeap = &kernel_debug_heap;
#if !USE_DEBUG_HEAPS_FOR_OBJECT_CACHE
		else if (strcmp(buffer, "slab") == 0)
			sHeap = &kernel_slab_heap;
#endif
		else
			panic("unknown or unavailable kernel heap '%s'!", buffer);
	}
	dprintf("kernel malloc: using %s\n", sHeap->name);

	addr_t heapBase = 0;
	size_t heapSize = sHeap->initial_size;
	if (heapSize != 0) {
		// try to accomodate low memory systems
		while (heapSize > (vm_page_num_pages() * B_PAGE_SIZE) / 8)
			heapSize /= 2;
		if (heapSize < 1024 * 1024)
			panic("heap_init: go buy some RAM please.");

		// map in the new heap and initialize it
		heapBase = vm_allocate_early(args, heapSize, heapSize,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, 0);
		TRACE(("heap at 0x%lx\n", heapBase));
	}

	return sHeap->init(args, heapBase, heapSize);
}


status_t
heap_init_post_area()
{
	if (sHeap->init_post_area == NULL)
		return B_OK;
	return sHeap->init_post_area();
}


status_t
heap_init_post_sem()
{
	if (sHeap->init_post_sem == NULL)
		return B_OK;
	return sHeap->init_post_sem();
}


status_t
heap_init_post_thread()
{
	if (sHeap->init_post_thread == NULL)
		return B_OK;
	return sHeap->init_post_thread();
}


void*
memalign(size_t alignment, size_t size)
{
	return sHeap->memalign(alignment, size, 0);
}


void*
memalign_etc(size_t alignment, size_t size, uint32 flags)
{
	return sHeap->memalign(alignment, size, flags);
}


void
free_etc(void* address, uint32 flags)
{
	return sHeap->free(address, flags);
}


void
free(void *address)
{
	return sHeap->free(address, 0);
}


void*
malloc(size_t size)
{
	return sHeap->memalign(0, size, 0);
}


void*
realloc_etc(void* address, size_t newSize, uint32 flags)
{
	return sHeap->realloc(address, newSize, flags);
}


void*
realloc(void *address, size_t newSize)
{
	return sHeap->realloc(address, newSize, 0);
}


extern "C" int
posix_memalign(void** _pointer, size_t alignment, size_t size)
{
	if ((alignment & (sizeof(void*) - 1)) != 0 || _pointer == NULL)
		return B_BAD_VALUE;

	*_pointer = sHeap->memalign(alignment, size, 0);
	return 0;
}


#if USE_DEBUG_HEAPS_FOR_OBJECT_CACHE


// #pragma mark - Slab API


struct ObjectCache {
	size_t object_size;
	size_t alignment;

	void* cookie;
	object_cache_constructor constructor;
	object_cache_destructor destructor;
};


object_cache*
create_object_cache(const char* name, size_t object_size, uint32 flags)
{
	return create_object_cache_etc(name, object_size, 0, 0, 0, 0, flags,
		NULL, NULL, NULL, NULL);
}


object_cache*
create_object_cache_etc(const char*, size_t objectSize, size_t alignment, size_t, size_t,
	size_t, uint32, void* cookie, object_cache_constructor ctor, object_cache_destructor dtor,
	object_cache_reclaimer)
{
	ObjectCache* cache = new ObjectCache;
	if (cache == NULL)
		return NULL;

	cache->object_size = objectSize;
	cache->alignment = alignment;
	cache->cookie = cookie;
	cache->constructor = ctor;
	cache->destructor = dtor;
	return cache;
}


void
delete_object_cache(object_cache* cache)
{
	delete cache;
}


status_t
object_cache_set_minimum_reserve(object_cache* cache, size_t objectCount)
{
	return B_OK;
}


void*
object_cache_alloc(object_cache* cache, uint32 flags)
{
	void* object = sHeap->memalign(cache->alignment, cache->object_size, flags);
	if (object == NULL)
		return NULL;

	if (cache->constructor != NULL)
		cache->constructor(cache->cookie, object);
	return object;
}


void
object_cache_free(object_cache* cache, void* object, uint32 flags)
{
	if (cache->destructor != NULL)
		cache->destructor(cache->cookie, object);
	return sHeap->free(object, flags);
}


status_t
object_cache_reserve(object_cache* cache, size_t objectCount, uint32 flags)
{
	return B_OK;
}


void
object_cache_get_usage(object_cache* cache, size_t* _allocatedMemory)
{
	*_allocatedMemory = 0;
}


void
request_memory_manager_maintenance()
{
}


void
slab_init(kernel_args* args)
{
}


void
slab_init_post_area()
{
}


void
slab_init_post_sem()
{
}


void
slab_init_post_thread()
{
}


#endif	// USE_DEBUG_HEAPS_FOR_OBJECT_CACHE


#endif	// DEBUG_HEAPS
