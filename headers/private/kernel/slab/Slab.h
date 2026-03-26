/*
 * Copyright 2008-2010, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef _SLAB_SLAB_H_
#define _SLAB_SLAB_H_


#include <heap.h>


enum {
	/* object_cache_{alloc,free}() flags */
	CACHE_DONT_WAIT_FOR_MEMORY		= HEAP_DONT_WAIT_FOR_MEMORY,
	CACHE_DONT_LOCK_KERNEL_SPACE	= HEAP_DONT_LOCK_KERNEL_SPACE,
	CACHE_PRIORITY_VIP				= HEAP_PRIORITY_VIP,
	CACHE_ALLOC_FLAGS				= CACHE_DONT_WAIT_FOR_MEMORY
										| CACHE_DONT_LOCK_KERNEL_SPACE
										| CACHE_PRIORITY_VIP,

	/* create_object_cache flags */
	CACHE_NO_DEPOT					= (1 << 27),
	CACHE_UNLOCKED_PAGES			= (1 << 28),	// unsupported
	CACHE_LARGE_SLAB				= (1 << 29),

	/* internal */
	CACHE_ALIGN_ON_SIZE				= (1 << 30),
	CACHE_DURING_BOOT				= (1 << 31)
};

struct ObjectCache;
typedef struct ObjectCache object_cache;

typedef status_t (*object_cache_constructor)(void* cookie, void* object);
typedef void (*object_cache_destructor)(void* cookie, void* object);
typedef void (*object_cache_reclaimer)(void* cookie, int32 level);


#ifdef __cplusplus
extern "C" {
#endif

object_cache* create_object_cache(const char* name, size_t objectSize,
	uint32 flags);
object_cache* create_object_cache_etc(const char* name, size_t objectSize,
	size_t alignment, size_t maxByteUsage, size_t magazineCapacity,
	size_t maxMagazineCount, uint32 flags, void* cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer);

void delete_object_cache(object_cache* cache);

status_t object_cache_set_minimum_reserve(object_cache* cache,
	size_t objectCount);

void* object_cache_alloc(object_cache* cache, uint32 flags);
void object_cache_free(object_cache* cache, void* object, uint32 flags);

status_t object_cache_reserve(object_cache* cache, size_t object_count,
	uint32 flags);

void object_cache_get_usage(object_cache* cache, size_t* _allocatedMemory);

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#include <new>


inline void*
operator new(size_t size, ObjectCache* objectCache, uint32 flags) throw()
{
	return object_cache_alloc(objectCache, flags);
}


template<typename Type>
inline void
object_cache_delete(ObjectCache* objectCache, Type* object, uint32 flags = 0)
{
	if (object != NULL) {
		object->~Type();
		object_cache_free(objectCache, object, flags);
	}
}

#endif	// __cplusplus


#endif	/* _SLAB_SLAB_H_ */
