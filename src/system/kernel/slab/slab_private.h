/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Hugo Santos, hugosantos@gmail.com
 */
#ifndef SLAB_PRIVATE_H
#define SLAB_PRIVATE_H


#include <stddef.h>

#include <slab/Slab.h>


static const size_t kMinObjectAlignment = 8;


void		request_memory_manager_maintenance();

void*		block_alloc_early(size_t size);
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


static inline void*
slab_internal_alloc(size_t size, uint32 flags)
{
	if (flags & CACHE_DURING_BOOT)
		return block_alloc_early(size);

	return malloc_etc(size, flags);
}


static inline void
slab_internal_free(void* buffer, uint32 flags)
{
	free_etc(buffer, flags);
}


#endif	// SLAB_PRIVATE_H
