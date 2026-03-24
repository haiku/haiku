/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_HEAP_H
#define _KERNEL_HEAP_H


#include <OS.h>

#include "kernel_debug_config.h"


// allocation/deallocation flags for {malloc,free}_etc()
#define HEAP_DONT_WAIT_FOR_MEMORY		0x01
#define HEAP_DONT_LOCK_KERNEL_SPACE		0x02
#define HEAP_PRIORITY_VIP				0x04


#ifdef __cplusplus
extern "C" {
#endif


void* memalign_etc(size_t alignment, size_t size, uint32 flags) _ALIGNED_BY_ARG(1);
void* realloc_etc(void* address, size_t newSize, uint32 flags);
void free_etc(void* address, uint32 flags);

void* memalign(size_t alignment, size_t size) _ALIGNED_BY_ARG(1);

void deferred_free(void* block);

status_t heap_init(struct kernel_args* args);
status_t heap_init_post_area();
status_t heap_init_post_sem();
status_t heap_init_post_thread();

void deferred_free_init();

#ifdef __cplusplus
}
#endif


static inline void*
malloc_etc(size_t size, uint32 flags)
{
	return memalign_etc(0, size, flags);
}


#ifdef __cplusplus

#include <new>

#include <util/SinglyLinkedList.h>


struct malloc_flags {
	uint32	flags;

	malloc_flags(uint32 flags)
		:
		flags(flags)
	{
	}

	malloc_flags(const malloc_flags& other)
		:
		flags(other.flags)
	{
	}
};


inline void*
operator new(size_t size, const malloc_flags& flags) throw()
{
	return malloc_etc(size, flags.flags);
}


inline void*
operator new[](size_t size, const malloc_flags& flags) throw()
{
	return malloc_etc(size, flags.flags);
}


class DeferredDeletable : public SinglyLinkedListLinkImpl<DeferredDeletable> {
public:
	virtual						~DeferredDeletable();
};


void deferred_delete(DeferredDeletable* deletable);


#endif	/* __cplusplus */


#endif	/* _KERNEL_MEMHEAP_H */
