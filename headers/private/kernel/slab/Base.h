/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#ifndef _SLAB_BASE_SLAB_H_
#define _SLAB_BASE_SLAB_H_

#include <stdint.h>

#include <OS.h>
#include <util/list.h>

#ifdef __cplusplus
#include <utility> // pair<>

extern "C" {
#endif

static const int kMinimumSlabItems = 32;

typedef void (*base_cache_constructor)(void *cookie, void *object);
typedef void (*base_cache_destructor)(void *cookie, void *object);

/* base Slab implementation, opaque to the backend used. */

typedef struct base_cache {
	char name[32];
	size_t object_size;
	size_t cache_color_cycle;
	struct list partial, full;
	base_cache_constructor constructor;
	base_cache_destructor destructor;
	void *cookie;
} base_cache;

typedef struct cache_object_link {
	struct cache_object_link *next;
} cache_object_link;

typedef struct cache_slab {
	void *pages;
	size_t count, size;
	cache_object_link *free;
	struct list_link link;
} cache_slab;

void base_cache_init(base_cache *cache, const char *name, size_t object_size,
	size_t alignment, base_cache_constructor constructor,
	base_cache_destructor destructor, void *cookie);

cache_object_link *base_cache_allocate_object(base_cache *cache);
int base_cache_return_object(base_cache *cache, cache_slab *slab,
	cache_object_link *link);

cache_slab *base_cache_construct_slab(base_cache *cache, cache_slab *slab,
	void *pages, size_t byte_count, cache_object_link *(*get_link)(
		void *parent, void *object), void *parent);
void base_cache_destruct_slab(base_cache *cache, cache_slab *slab);

#ifdef __cplusplus
}

typedef std::pair<cache_slab *, cache_object_link *> CacheObjectInfo;

// Slab implementation, glues together the frontend, backend as
// well as the Slab strategy used.
template<typename Strategy>
class Cache : protected base_cache {
public:
	typedef base_cache_constructor	Constructor;
	typedef base_cache_destructor	Destructor;

	Cache(const char *name, size_t objectSize, size_t alignment,
		Constructor constructor, Destructor destructor, void *cookie)
		: fStrategy(this)
	{
		base_cache_init(this, name, objectSize, alignment, constructor,
			destructor, cookie);
	}

	void *AllocateObject(uint32_t flags)
	{
		if (list_is_empty(&partial)) {
			cache_slab *newSlab = fStrategy.NewSlab(flags);
			if (newSlab == NULL)
				return NULL;
			list_add_item(&partial, newSlab);
		}

		return fStrategy.Object(base_cache_allocate_object(this));
	}

	void ReturnObject(void *object)
	{
		CacheObjectInfo location = fStrategy.ObjectInformation(object);

		if (base_cache_return_object(this, location.first, location.second))
			fStrategy.ReturnSlab(location.first);
	}

private:
	Strategy fStrategy;
};

#endif

#endif
