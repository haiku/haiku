/*
 * Copyright 2008-2010, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef OBJECT_CACHE_H
#define OBJECT_CACHE_H


#include <condition_variable.h>
#include <lock.h>
#include <slab/ObjectDepot.h>
#include <slab/Slab.h>
#include <util/DoublyLinkedList.h>

#include "kernel_debug_config.h"
#include "slab_debug.h"


class AllocationTrackingInfo;
struct ResizeRequest;


struct object_link {
	struct object_link* next;
};

struct slab : DoublyLinkedListLinkImpl<slab> {
	void*			pages;
	size_t			size;		// total number of objects
	size_t			count;		// free objects
	size_t			offset;
	object_link*	free;
#if SLAB_OBJECT_CACHE_ALLOCATION_TRACKING
	AllocationTrackingInfo*	tracking;
#endif
};

typedef DoublyLinkedList<slab> SlabList;

struct ObjectCacheResizeEntry {
	ConditionVariable	condition;
	thread_id			thread;
};

struct ObjectCache : DoublyLinkedListLinkImpl<ObjectCache> {
			char				name[32];
			mutex				lock;
			size_t				object_size;
			size_t				alignment;
			size_t				cache_color_cycle;
			SlabList			empty;
			SlabList			partial;
			SlabList			full;
			size_t				total_objects;		// total number of objects
			size_t				used_count;			// used objects
			size_t				empty_count;		// empty slabs
			size_t				pressure;
			size_t				min_object_reserve;
									// minimum number of free objects

			size_t				slab_size;
			size_t				usage;
			size_t				maximum;
			uint32				flags;

			ResizeRequest*		resize_request;

			ObjectCacheResizeEntry* resize_entry_can_wait;
			ObjectCacheResizeEntry* resize_entry_dont_wait;

			DoublyLinkedListLink<ObjectCache> maintenance_link;
			bool				maintenance_pending;
			bool				maintenance_in_progress;
			bool				maintenance_resize;
			bool				maintenance_delete;

			void*				cookie;
			object_cache_constructor constructor;
			object_cache_destructor destructor;
			object_cache_reclaimer reclaimer;

			object_depot		depot;

public:
	virtual						~ObjectCache();

			status_t			Init(const char* name, size_t objectSize,
									size_t alignment, size_t maximum,
									size_t magazineCapacity,
									size_t maxMagazineCount, uint32 flags,
									void* cookie,
									object_cache_constructor constructor,
									object_cache_destructor destructor,
									object_cache_reclaimer reclaimer);
	virtual	void				Delete() = 0;

	virtual	slab*				CreateSlab(uint32 flags) = 0;
	virtual	void				ReturnSlab(slab* slab, uint32 flags) = 0;
	virtual slab*				ObjectSlab(void* object) const = 0;

			slab*				InitSlab(slab* slab, void* pages,
									size_t byteCount, uint32 flags);
			void				UninitSlab(slab* slab);

			void				ReturnObjectToSlab(slab* source, void* object,
									uint32 flags);
			void*				ObjectAtIndex(slab* source, int32 index) const;

			bool				Lock()	{ return mutex_lock(&lock) == B_OK; }
			void				Unlock()	{ mutex_unlock(&lock); }

			status_t			AllocatePages(void** pages, uint32 flags);
			void				FreePages(void* pages);
			status_t			EarlyAllocatePages(void** pages, uint32 flags);
			void				EarlyFreePages(void* pages);

#if PARANOID_KERNEL_FREE
			bool				AssertObjectNotFreed(void* object);
#endif

			status_t			AllocateTrackingInfos(slab* slab,
									size_t byteCount, uint32 flags);
			void				FreeTrackingInfos(slab* slab, uint32 flags);

#if SLAB_OBJECT_CACHE_ALLOCATION_TRACKING
			AllocationTrackingInfo*
								TrackingInfoFor(void* object) const;
#endif
};


static inline void*
link_to_object(object_link* link, size_t objectSize)
{
	return ((uint8*)link) - (objectSize - sizeof(object_link));
}


static inline object_link*
object_to_link(void* object, size_t objectSize)
{
	return (object_link*)(((uint8*)object)
		+ (objectSize - sizeof(object_link)));
}


static inline void*
lower_boundary(const void* object, size_t byteCount)
{
	return (void*)((addr_t)object & ~(byteCount - 1));
}


static inline bool
check_cache_quota(ObjectCache* cache)
{
	if (cache->maximum == 0)
		return true;

	return (cache->usage + cache->slab_size) <= cache->maximum;
}


#if !SLAB_OBJECT_CACHE_ALLOCATION_TRACKING

inline status_t
ObjectCache::AllocateTrackingInfos(slab* slab, size_t byteCount, uint32 flags)
{
	return B_OK;
}


inline void
ObjectCache::FreeTrackingInfos(slab* slab, uint32 flags)
{
}

#endif // !SLAB_OBJECT_CACHE_ALLOCATION_TRACKING

#endif	// OBJECT_CACHE_H
