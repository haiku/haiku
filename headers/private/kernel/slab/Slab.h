/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#ifndef _SLAB_SLAB_H_
#define _SLAB_SLAB_H_

#include <slab/Depot.h>

#ifdef __cplusplus
#include <slab/Backend.h>
#include <slab/Base.h>
#include <slab/MergedStrategy.h>
#include <slab/HashStrategy.h>
#include <slab/Utilities.h>


extern "C" {
#endif

typedef void *object_cache_t;


object_cache_t
object_cache_create(const char *name, size_t object_size, size_t alignment,
	base_cache_constructor constructor, base_cache_destructor destructor,
	void *cookie);
void *object_cache_alloc(object_cache_t cache);
void *object_cache_alloc_etc(object_cache_t cache, uint32_t flags);
void object_cache_free(object_cache_t cache, void *object);
void object_cache_destroy(object_cache_t cache);

#ifdef __cplusplus
}


template<typename CacheType>
class LocalCache : public CacheType, protected base_depot {
public:
	typedef LocalCache<CacheType> ThisType;
	typedef typename CacheType::Constructor Constructor;
	typedef typename CacheType::Destructor Destructor;

	LocalCache(const char *name, size_t objectSize, size_t alignment,
		Constructor _constructor, Destructor _destructor, void *_cookie)
		: CacheType(name, objectSize, alignment, _constructor, _destructor,
			_cookie)
	{
		fStatus = base_depot_init(this, _ReturnObject);
	}

	~LocalCache()
	{
		base_depot_destroy(this);
	}

	status_t InitCheck() const { return fStatus; }

	void *Alloc(uint32_t flags)
	{
		void *object = base_depot_obtain_from_store(this, base_depot_cpu(this));
		if (object == NULL)
			object = CacheType::AllocateObject(flags);
		return object;
	}

	void Free(void *object)
	{
		if (!base_depot_return_to_store(this, base_depot_cpu(this), object))
			CacheType::ReturnObject(object);
	}

	void Destroy()
	{
		base_depot_make_empty(this);
	}

private:
	void ReturnObject(void *object)
	{
		CacheType::ReturnObject(object);
	}

	static void _ReturnObject(base_depot *self, void *object)
	{
		static_cast<ThisType *>(self)->ReturnObject(object);
	}

	status_t fStatus;
};

#endif /* __cplusplus */

#endif
