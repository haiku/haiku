/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "kernel_debug_config.h"

#if DEBUG_HEAPS

#include <stdlib.h>
#include <ctype.h>

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
static kernel_heap_implementation* sActiveHeaps[2] = { &HEAP_SYMBOL(DEBUG_HEAPS_DEFAULT) };

#if GUARDED_HEAP_CAN_REPLACE_OBJECT_CACHES
struct CacheSelector {
	char name[32];
	bool globPrefix : 1;
	bool globSuffix : 1;
};
static CacheSelector* sGuardedHeapForObjectCaches = NULL;
static int32 sGuardedHeapForObjectCachesCount = 0;
#endif


//	#pragma mark -


static status_t
init_heap(struct kernel_args* args, kernel_heap_implementation* heap)
{
	addr_t heapBase = 0;
	size_t heapSize = heap->initial_size;
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

	return heap->init(args, heapBase, heapSize);
}


#if GUARDED_HEAP_CAN_REPLACE_OBJECT_CACHES
bool
guarded_heap_replaces_object_cache(const char* name)
{
	if (sGuardedHeapForObjectCaches == NULL)
		return false;

	bool match = false;
	for (int32 i = 0; !match && i < sGuardedHeapForObjectCachesCount; i++) {
		CacheSelector& selector = sGuardedHeapForObjectCaches[i];
		if (selector.globPrefix && selector.globSuffix) {
			if (strstr(name, selector.name) != NULL)
				match = true;
		} else if (selector.globPrefix) {
			int32 nameLen = strlen(name), selectorLen = strlen(selector.name);
			if (strcmp(name + (nameLen - selectorLen), selector.name) == 0)
				match = true;
		} else if (selector.globSuffix) {
			if (strncmp(name, selector.name, strlen(selector.name)) == 0)
				match = true;
		} else {
			if (strcmp(name, selector.name) == 0)
				match = true;
		}
	}

	if (match)
		dprintf("using guarded heap for object_cache \"%s\"\n", name);
	return match;
}


static void
init_object_cache_replacements(struct kernel_args* args)
{
	char buffer[1024];
	size_t bufferSize = sizeof(buffer);
	if (get_safemode_option_early(args, "guarded_heap_for_object_caches",
			buffer, &bufferSize) != B_OK)
		return;

	if (sActiveHeaps[0] != &kernel_guarded_heap) {
		// We need to initialize the guarded heap, too.
		sActiveHeaps[1] = &kernel_guarded_heap;
		init_heap(args, sActiveHeaps[1]);
	}

	sGuardedHeapForObjectCaches = (CacheSelector*)
		kernel_guarded_heap.memalign(0, B_PAGE_SIZE, 0);
	memset(sGuardedHeapForObjectCaches, 0, B_PAGE_SIZE);

	// Parse the option.
	const char* option = buffer;
	while (*option != '\0') {
		CacheSelector& selector
			= sGuardedHeapForObjectCaches[sGuardedHeapForObjectCachesCount++];

		size_t nameLength = 0;
		char quoteEnd = '\0';
		bool phraseEnd = false;
		bool seenGlob = false;
		while (!phraseEnd && nameLength < (sizeof(selector.name) - 1)) {
			if (quoteEnd == '\0' && isspace(*option)) {
				option++;
				continue;
			}

			switch (*option) {
				case '"':
				case '\'':
					if (quoteEnd == '\0')
						quoteEnd = *option;
					else if (quoteEnd == *option)
						quoteEnd = '\0';
					break;

				case '\0':
				case ',':
					phraseEnd = true;
					break;

				case '*':
					if (nameLength == 0)
						selector.globPrefix = true;
					else
						seenGlob = true;
					break;

				case '\\':
					option++;
					// fall through
				default:
					if (seenGlob) {
						dprintf("heap_init: error: unsupported glob pattern\n");
						seenGlob = false;
					}
					selector.name[nameLength++] = *option;
					break;
			}
			option++;
		}
		if (seenGlob)
			selector.globSuffix = true;
		if (!phraseEnd) {
			dprintf("heap_init: error: pattern overflow after '%s'\n", selector.name);
			continue;
		}
	}

	dprintf("guarded_heap_for_object_caches: loaded %" B_PRId32 " selectors\n",
		sGuardedHeapForObjectCachesCount);
}
#endif


status_t
heap_init(struct kernel_args* args)
{
	char buffer[32];
	size_t bufferSize = sizeof(buffer);
	if (get_safemode_option_early(args, "kernel_malloc", buffer, &bufferSize) == B_OK) {
		if (strcmp(buffer, "guarded") == 0)
			sActiveHeaps[0] = &kernel_guarded_heap;
		else if (strcmp(buffer, "debug") == 0)
			sActiveHeaps[0] = &kernel_debug_heap;
#if !USE_DEBUG_HEAPS_FOR_ALL_OBJECT_CACHES
		else if (strcmp(buffer, "slab") == 0)
			sActiveHeaps[0] = &kernel_slab_heap;
#endif
		else
			panic("unknown or unavailable kernel heap '%s'!", buffer);
	}
	dprintf("kernel malloc: using %s\n", sActiveHeaps[0]->name);

	status_t status = init_heap(args, sActiveHeaps[0]);
	if (status != B_OK)
		return status;

#if GUARDED_HEAP_CAN_REPLACE_OBJECT_CACHES
	init_object_cache_replacements(args);
#endif

	return B_OK;
}


status_t
heap_init_post_area()
{
	for (size_t i = 0; i < B_COUNT_OF(sActiveHeaps); i++) {
		if (sActiveHeaps[i] == NULL || sActiveHeaps[i]->init_post_area == NULL)
			continue;

		status_t status = sActiveHeaps[i]->init_post_area();
		if (status != B_OK)
			return status;
	}
	return B_OK;
}


status_t
heap_init_post_sem()
{
	for (size_t i = 0; i < B_COUNT_OF(sActiveHeaps); i++) {
		if (sActiveHeaps[i] == NULL || sActiveHeaps[i]->init_post_sem == NULL)
			continue;

		status_t status = sActiveHeaps[i]->init_post_sem();
		if (status != B_OK)
			return status;
	}
	return B_OK;
}


status_t
heap_init_post_thread()
{
	for (size_t i = 0; i < B_COUNT_OF(sActiveHeaps); i++) {
		if (sActiveHeaps[i] == NULL || sActiveHeaps[i]->init_post_thread == NULL)
			continue;

		status_t status = sActiveHeaps[i]->init_post_thread();
		if (status != B_OK)
			return status;
	}
	return B_OK;
}


void*
memalign(size_t alignment, size_t size)
{
	return sActiveHeaps[0]->memalign(alignment, size, 0);
}


void*
memalign_etc(size_t alignment, size_t size, uint32 flags)
{
	return sActiveHeaps[0]->memalign(alignment, size, flags);
}


void
free_etc(void* address, uint32 flags)
{
	return sActiveHeaps[0]->free(address, flags);
}


void
free(void *address)
{
	return sActiveHeaps[0]->free(address, 0);
}


void*
malloc(size_t size)
{
	return sActiveHeaps[0]->memalign(0, size, 0);
}


void*
realloc_etc(void* address, size_t newSize, uint32 flags)
{
	return sActiveHeaps[0]->realloc(address, newSize, flags);
}


void*
realloc(void *address, size_t newSize)
{
	return sActiveHeaps[0]->realloc(address, newSize, 0);
}


extern "C" int
posix_memalign(void** _pointer, size_t alignment, size_t size)
{
	if ((alignment & (sizeof(void*) - 1)) != 0 || _pointer == NULL)
		return B_BAD_VALUE;

	*_pointer = sActiveHeaps[0]->memalign(alignment, size, 0);
	return 0;
}


#if USE_DEBUG_HEAPS_FOR_ALL_OBJECT_CACHES


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
	void* object = sActiveHeaps[0]->memalign(cache->alignment, cache->object_size, flags);
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
	return sActiveHeaps[0]->free(object, flags);
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


#endif	// USE_DEBUG_HEAPS_FOR_ALL_OBJECT_CACHES


#endif	// DEBUG_HEAPS
