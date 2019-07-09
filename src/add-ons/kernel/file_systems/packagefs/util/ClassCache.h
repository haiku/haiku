/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef CLASSCACHE_H
#define CLASSCACHE_H


#include <slab/Slab.h>


#define CLASS_CACHE(CLASS) \
	static object_cache* s##CLASS##Cache = NULL; \
	\
	void* \
	CLASS::operator new(size_t size) \
	{ \
		if (size != sizeof(CLASS)) \
			panic("unexpected size passed to operator new!"); \
		if (s##CLASS##Cache == NULL) { \
			s##CLASS##Cache = create_object_cache("packagefs " #CLASS "s", \
				sizeof(CLASS), 8, NULL, NULL, NULL); \
		} \
	\
		return object_cache_alloc(s##CLASS##Cache, 0); \
	} \
	\
	void \
	CLASS::operator delete(void* block) \
	{ \
		object_cache_free(s##CLASS##Cache, block, 0); \
	}


#endif	// CLASSCACHE_H
