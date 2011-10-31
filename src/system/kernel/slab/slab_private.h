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

#include <slab/Slab.h>


//#define TRACE_SLAB
#ifdef TRACE_SLAB
#define TRACE_CACHE(cache, format, args...) \
	dprintf("Cache[%p, %s] " format "\n", cache, cache->name , ##args)
#else
#define TRACE_CACHE(cache, format, bananas...) do { } while (0)
#endif


#define COMPONENT_PARANOIA_LEVEL	OBJECT_CACHE_PARANOIA
#include <debug_paranoia.h>



static const size_t kMinObjectAlignment = 8;


struct ObjectCache;
struct object_depot;


void		request_memory_manager_maintenance();

void*		block_alloc(size_t size, size_t alignment, uint32 flags);
void*		block_alloc_early(size_t size);
void		block_free(void* block, uint32 flags);
void		block_allocator_init_boot();
void		block_allocator_init_rest();

void		dump_object_depot(object_depot* depot);
int			dump_object_depot(int argCount, char** args);
int			dump_depot_magazine(int argCount, char** args);


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


static inline void*
slab_internal_alloc(size_t size, uint32 flags)
{
	if (flags & CACHE_DURING_BOOT)
		return block_alloc_early(size);

	return block_alloc(size, 0, flags);
}


static inline void
slab_internal_free(void* buffer, uint32 flags)
{
	block_free(buffer, flags);
}


#if PARANOID_KERNEL_MALLOC || PARANOID_KERNEL_FREE
static inline void*
fill_block(void* buffer, size_t size, uint32 pattern)
{
	if (buffer == NULL)
		return NULL;

	size &= ~(sizeof(pattern) - 1);
	for (size_t i = 0; i < size / sizeof(pattern); i++)
		((uint32*)buffer)[i] = pattern;

	return buffer;
}
#endif


static inline void*
fill_allocated_block(void* buffer, size_t size)
{
#if PARANOID_KERNEL_MALLOC
	return fill_block(buffer, size, 0xcccccccc);
#else
	return buffer;
#endif
}


static inline void*
fill_freed_block(void* buffer, size_t size)
{
#if PARANOID_KERNEL_FREE
	return fill_block(buffer, size, 0xdeadbeef);
#else
	return buffer;
#endif
}

#endif	// SLAB_PRIVATE_H
