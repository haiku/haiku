/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */
#ifndef SLAB_PRIVATE_H
#define SLAB_PRIVATE_H


#include <stddef.h>


//#define TRACE_SLAB
#ifdef TRACE_SLAB
#define TRACE_CACHE(cache, format, args...) \
	dprintf("Cache[%p, %s] " format "\n", cache, cache->name , ##args)
#else
#define TRACE_CACHE(cache, format, bananas...) do { } while (0)
#endif


#define COMPONENT_PARANOIA_LEVEL	OBJECT_CACHE_PARANOIA
#include <debug_paranoia.h>

struct ObjectCache;

void*		slab_internal_alloc(size_t size, uint32 flags);
void		slab_internal_free(void *_buffer);

void*		block_alloc(size_t size, uint32 flags);
void		block_free(void *block);
void		block_allocator_init_boot();
void		block_allocator_init_rest();


template<typename Type>
static inline Type*
_pop(Type*& head)
{
	Type* oldHead = head;
	head = head->next;
	return oldHead;
}


template<typename Type>
static inline void
_push(Type*& head, Type* object)
{
	object->next = head;
	head = object;
}


#endif	// SLAB_PRIVATE_H
