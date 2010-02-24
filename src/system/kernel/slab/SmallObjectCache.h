/*
 * Copyright 2008, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef SMALL_OBJECT_CACHE_H
#define SMALL_OBJECT_CACHE_H


#include "ObjectCache.h"


struct SmallObjectCache : ObjectCache {
	static	SmallObjectCache*	Create(const char* name, size_t object_size,
									size_t alignment, size_t maximum,
									size_t magazineCapacity,
									size_t maxMagazineCount,
									uint32 flags, void* cookie,
									object_cache_constructor constructor,
									object_cache_destructor destructor,
									object_cache_reclaimer reclaimer);
	virtual	void				Delete();

	virtual	slab*				CreateSlab(uint32 flags);
	virtual	void				ReturnSlab(slab* slab, uint32 flags);
	virtual slab*				ObjectSlab(void* object) const;
};


#endif	// SMALL_OBJECT_CACHE_H
