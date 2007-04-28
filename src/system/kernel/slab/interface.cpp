/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include <Slab.h>
#include <new>

class AbstractCache {
public:
	virtual ~AbstractCache() {}

	virtual void *Allocate(uint32_t flags) = 0;
	virtual void Return(void *object) = 0;
};


typedef MergedLinkCacheStrategy<AreaBackend> SmallObjectStrategy;
typedef HashCacheStrategy<AreaBackend> LargeObjectStrategy;

typedef Cache<SmallObjectStrategy> SmallObjectCache;
typedef Cache<LargeObjectStrategy> LargeObjectCache;


class SmallObjectAbstractCache : public AbstractCache,
	public LocalCache<SmallObjectCache> {
public:
	typedef LocalCache<SmallObjectCache> Base;

	SmallObjectAbstractCache(const char *name, size_t objectSize,
		size_t alignment, base_cache_constructor constructor,
		base_cache_destructor destructor, void *cookie)
		: Base(name, objectSize, alignment, constructor, destructor, cookie) {}

	void *Allocate(uint32_t flags) { return Base::Alloc(flags); }
	void Return(void *object) { Base::Free(object); }
};


class LargeObjectAbstractCache : public AbstractCache,
	public LocalCache<LargeObjectCache> {
public:
	typedef LocalCache<LargeObjectCache> Base;

	LargeObjectAbstractCache(const char *name, size_t objectSize,
		size_t alignment, base_cache_constructor constructor,
		base_cache_destructor destructor, void *cookie)
		: Base(name, objectSize, alignment, constructor, destructor, cookie) {}

	void *Allocate(uint32_t flags) { return Base::Alloc(flags); }
	void Return(void *object) { Base::Free(object); }
};


object_cache_t
object_cache_create(const char *name, size_t object_size, size_t alignment,
	status_t (*_constructor)(void *, void *), void (*_destructor)(void *,
	void *), void *cookie)
{
	if (object_size == 0)
		return NULL;
	else if (object_size <= 256)
		return new (std::nothrow) SmallObjectAbstractCache(name, object_size,
			alignment, _constructor, _destructor, cookie);

	return new (std::nothrow) LargeObjectAbstractCache(name, object_size,
		alignment, _constructor, _destructor, cookie);
}


void *
object_cache_alloc(object_cache_t cache)
{
	return object_cache_alloc_etc(cache, 0);
}


void *
object_cache_alloc_etc(object_cache_t cache, uint32_t flags)
{
	return ((AbstractCache *)cache)->Allocate(flags);
}


void
object_cache_free(object_cache_t cache, void *object)
{
	((AbstractCache *)cache)->Return(object);
}


void
object_cache_destroy(object_cache_t cache)
{
	delete (AbstractCache *)cache;
}

