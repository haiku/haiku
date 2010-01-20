/*
 * Copyright 2008, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <slab/Slab.h>

#include <algorithm>
#include <new>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <condition_variable.h>
#include <kernel.h>
#include <low_resource_manager.h>
#include <slab/ObjectDepot.h>
#include <smp.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/khash.h>
#include <vm/vm.h>
#include <vm/VMAddressSpace.h>

#include "HashedObjectCache.h"
#include "slab_private.h"
#include "SmallObjectCache.h"


typedef DoublyLinkedList<ObjectCache> ObjectCacheList;


struct ResizeRequest : DoublyLinkedListLinkImpl<ResizeRequest> {
	ResizeRequest(ObjectCache* cache)
		:
		cache(cache),
		pending(false),
		delete_when_done(false)
	{
	}

	ObjectCache*	cache;
	bool			pending;
	bool			delete_when_done;
};

typedef DoublyLinkedList<ResizeRequest> ResizeRequestQueue;


static ObjectCacheList sObjectCaches;
static mutex sObjectCacheListLock = MUTEX_INITIALIZER("object cache list");

static uint8* sInitialBegin;
static uint8* sInitialLimit;
static uint8* sInitialPointer;

static status_t object_cache_reserve_internal(ObjectCache* cache,
	size_t object_count, uint32 flags, bool unlockWhileAllocating);

static mutex sResizeRequestsLock
	= MUTEX_INITIALIZER("object cache resize requests");
static ResizeRequestQueue sResizeRequests;
static ConditionVariable sResizeRequestsCondition;


#if OBJECT_CACHE_TRACING


namespace ObjectCacheTracing {

class ObjectCacheTraceEntry : public AbstractTraceEntry {
	public:
		ObjectCacheTraceEntry(ObjectCache* cache)
			:
			fCache(cache)
		{
		}

	protected:
		ObjectCache*	fCache;
};


class Create : public ObjectCacheTraceEntry {
	public:
		Create(const char* name, size_t objectSize, size_t alignment,
				size_t maxByteUsage, uint32 flags, void* cookie,
				ObjectCache* cache)
			:
			ObjectCacheTraceEntry(cache),
			fObjectSize(objectSize),
			fAlignment(alignment),
			fMaxByteUsage(maxByteUsage),
			fFlags(flags),
			fCookie(cookie)
		{
			fName = alloc_tracing_buffer_strcpy(name, 64, false);
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("object cache create: name: \"%s\", object size: %lu, "
				"alignment: %lu, max usage: %lu, flags: 0x%lx, cookie: %p "
				"-> cache: %p", fName, fObjectSize, fAlignment, fMaxByteUsage,
					fFlags, fCookie, fCache);
		}

	private:
		const char*	fName;
		size_t		fObjectSize;
		size_t		fAlignment;
		size_t		fMaxByteUsage;
		uint32		fFlags;
		void*		fCookie;
};


class Delete : public ObjectCacheTraceEntry {
	public:
		Delete(ObjectCache* cache)
			:
			ObjectCacheTraceEntry(cache)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("object cache delete: %p", fCache);
		}
};


class Alloc : public ObjectCacheTraceEntry {
	public:
		Alloc(ObjectCache* cache, uint32 flags, void* object)
			:
			ObjectCacheTraceEntry(cache),
			fFlags(flags),
			fObject(object)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("object cache alloc: cache: %p, flags: 0x%lx -> "
				"object: %p", fCache, fFlags, fObject);
		}

	private:
		uint32		fFlags;
		void*		fObject;
};


class Free : public ObjectCacheTraceEntry {
	public:
		Free(ObjectCache* cache, void* object)
			:
			ObjectCacheTraceEntry(cache),
			fObject(object)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("object cache free: cache: %p, object: %p", fCache,
				fObject);
		}

	private:
		void*		fObject;
};


class Reserve : public ObjectCacheTraceEntry {
	public:
		Reserve(ObjectCache* cache, size_t count, uint32 flags)
			:
			ObjectCacheTraceEntry(cache),
			fCount(count),
			fFlags(flags)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("object cache reserve: cache: %p, count: %lu, "
				"flags: 0x%lx", fCache, fCount, fFlags);
		}

	private:
		uint32		fCount;
		uint32		fFlags;
};


}	// namespace ObjectCacheTracing

#	define T(x)	new(std::nothrow) ObjectCacheTracing::x

#else
#	define T(x)
#endif	// OBJECT_CACHE_TRACING


// #pragma mark -


static int
dump_slabs(int argc, char* argv[])
{
	kprintf("%10s %22s %8s %8s %6s %8s %8s %8s\n", "address", "name",
		"objsize", "usage", "empty", "usedobj", "total", "flags");

	ObjectCacheList::Iterator it = sObjectCaches.GetIterator();

	while (it.HasNext()) {
		ObjectCache* cache = it.Next();

		kprintf("%p %22s %8lu %8lu %6lu %8lu %8lu %8lx\n", cache, cache->name,
			cache->object_size, cache->usage, cache->empty_count,
			cache->used_count, cache->usage / cache->object_size,
			cache->flags);
	}

	return 0;
}


static int
dump_cache_info(int argc, char* argv[])
{
	if (argc < 2) {
		kprintf("usage: cache_info [address]\n");
		return 0;
	}

	ObjectCache* cache = (ObjectCache*)strtoul(argv[1], NULL, 16);

	kprintf("name: %s\n", cache->name);
	kprintf("lock: %p\n", &cache->lock);
	kprintf("object_size: %lu\n", cache->object_size);
	kprintf("cache_color_cycle: %lu\n", cache->cache_color_cycle);
	kprintf("used_count: %lu\n", cache->used_count);
	kprintf("empty_count: %lu\n", cache->empty_count);
	kprintf("pressure: %lu\n", cache->pressure);
	kprintf("slab_size: %lu\n", cache->slab_size);
	kprintf("usage: %lu\n", cache->usage);
	kprintf("maximum: %lu\n", cache->maximum);
	kprintf("flags: 0x%lx\n", cache->flags);
	kprintf("cookie: %p\n", cache->cookie);

	return 0;
}


// #pragma mark -


void*
slab_internal_alloc(size_t size, uint32 flags)
{
	if (flags & CACHE_DURING_BOOT) {
		if ((sInitialPointer + size) > sInitialLimit) {
			panic("slab_internal_alloc: ran out of initial space");
			return NULL;
		}

		uint8* buffer = sInitialPointer;
		sInitialPointer += size;
		return buffer;
	}

	// TODO: Support CACHE_DONT_SLEEP!
	return block_alloc(size);
}


void
slab_internal_free(void* _buffer)
{
	uint8* buffer = (uint8*)_buffer;

	if (buffer >= sInitialBegin && buffer < sInitialLimit)
		return;

	block_free(buffer);
}


static void
object_cache_low_memory(void* _self, uint32 resources, int32 level)
{
	if (level == B_NO_LOW_RESOURCE)
		return;

	ObjectCache* cache = (ObjectCache*)_self;

	// we are calling the reclaimer without the object cache lock
	// to give the owner a chance to return objects to the slabs.
	// As we only register the low memory handler after we complete
	// the init of the object cache, unless there are some funky
	// memory re-order business going on, we should be ok.

	if (cache->reclaimer)
		cache->reclaimer(cache->cookie, level);

	MutexLocker _(cache->lock);
	size_t minimumAllowed;

	switch (level) {
	case B_LOW_RESOURCE_NOTE:
		minimumAllowed = cache->pressure / 2 + 1;
		break;

	case B_LOW_RESOURCE_WARNING:
		cache->pressure /= 2;
		minimumAllowed = 0;
		break;

	default:
		cache->pressure = 0;
		minimumAllowed = 0;
		break;
	}

	// If the object cache has minimum object reserve, make sure that we don't
	// free too many slabs.
	if (cache->min_object_reserve > 0 && cache->empty_count > 0) {
		size_t objectsPerSlab = cache->empty.Head()->size;
		size_t freeObjects = cache->total_objects - cache->used_count;

		if (cache->min_object_reserve + objectsPerSlab >= freeObjects)
			return;

		size_t slabsToFree = (freeObjects - cache->min_object_reserve)
			/ objectsPerSlab;

		if (cache->empty_count > minimumAllowed + slabsToFree)
			minimumAllowed = cache->empty_count - slabsToFree;
	}

	if (cache->empty_count <= minimumAllowed)
		return;

	TRACE_CACHE(cache, "cache: memory pressure, will release down to %lu.",
		minimumAllowed);

	while (cache->empty_count > minimumAllowed) {
		cache->ReturnSlab(cache->empty.RemoveHead());
		cache->empty_count--;
	}
}


static status_t
object_cache_resizer(void*)
{
	while (true) {
		MutexLocker locker(sResizeRequestsLock);

		// wait for the next request
		while (sResizeRequests.IsEmpty()) {
			ConditionVariableEntry entry;
			sResizeRequestsCondition.Add(&entry);
			locker.Unlock();
			entry.Wait();
			locker.Lock();
		}

		ResizeRequest* request = sResizeRequests.RemoveHead();

		locker.Unlock();

		// resize the cache, if necessary

		ObjectCache* cache = request->cache;

		MutexLocker cacheLocker(cache->lock);

		size_t freeObjects = cache->total_objects - cache->used_count;

		while (freeObjects < cache->min_object_reserve) {
			status_t error = object_cache_reserve_internal(cache,
				cache->min_object_reserve - freeObjects, 0, true);
			if (error != B_OK) {
				dprintf("object cache resizer: Failed to resize object cache "
					"%p!\n", cache);
				break;
			}

			freeObjects = cache->total_objects - cache->used_count;
		}

		request->pending = false;

		if (request->delete_when_done)
			delete request;
	}

	// never can get here
	return B_OK;
}


static void
increase_object_reserve(ObjectCache* cache)
{
	if (cache->resize_request->pending)
		return;

	cache->resize_request->pending = true;

	MutexLocker locker(sResizeRequestsLock);
	sResizeRequests.Add(cache->resize_request);
	sResizeRequestsCondition.NotifyAll();
}


object_cache*
create_object_cache(const char* name, size_t object_size, size_t alignment,
	void* cookie, object_cache_constructor constructor,
	object_cache_destructor destructor)
{
	return create_object_cache_etc(name, object_size, alignment, 0, 0, cookie,
		constructor, destructor, NULL);
}


object_cache*
create_object_cache_etc(const char* name, size_t objectSize, size_t alignment,
	size_t maximum, uint32 flags, void* cookie,
	object_cache_constructor constructor, object_cache_destructor destructor,
	object_cache_reclaimer reclaimer)
{
	ObjectCache* cache;

	if (objectSize == 0) {
		cache = NULL;
	} else if (objectSize <= 256) {
		cache = SmallObjectCache::Create(name, objectSize, alignment, maximum,
			flags, cookie, constructor, destructor, reclaimer);
	} else {
		cache = HashedObjectCache::Create(name, objectSize, alignment,
			maximum, flags, cookie, constructor, destructor, reclaimer);
	}

	if (cache != NULL) {
		MutexLocker _(sObjectCacheListLock);
		sObjectCaches.Add(cache);
	}

	T(Create(name, objectSize, alignment, maximum, flags, cookie, cache));
	return cache;
}


void
delete_object_cache(object_cache* cache)
{
	T(Delete(cache));

	{
		MutexLocker _(sObjectCacheListLock);
		sObjectCaches.Remove(cache);
	}

	if (!(cache->flags & CACHE_NO_DEPOT))
		object_depot_destroy(&cache->depot);

	mutex_lock(&cache->lock);

	unregister_low_resource_handler(object_cache_low_memory, cache);

	if (!cache->full.IsEmpty())
		panic("cache destroy: still has full slabs");

	if (!cache->partial.IsEmpty())
		panic("cache destroy: still has partial slabs");

	while (!cache->empty.IsEmpty())
		cache->ReturnSlab(cache->empty.RemoveHead());

	mutex_destroy(&cache->lock);
	cache->Delete();
}


status_t
object_cache_set_minimum_reserve(object_cache* cache, size_t objectCount)
{
	MutexLocker _(cache->lock);

	if (cache->min_object_reserve == objectCount)
		return B_OK;

	if (cache->min_object_reserve == 0) {
		cache->resize_request = new(std::nothrow) ResizeRequest(cache);
		if (cache->resize_request == NULL)
			return B_NO_MEMORY;
	} else if (cache->min_object_reserve == 0) {
		if (cache->resize_request->pending)
			cache->resize_request->delete_when_done = true;
		else
			delete cache->resize_request;

		cache->resize_request = NULL;
	}

	cache->min_object_reserve = objectCount;

	increase_object_reserve(cache);

	return B_OK;
}


void*
object_cache_alloc(object_cache* cache, uint32 flags)
{
	if (!(cache->flags & CACHE_NO_DEPOT)) {
		void* object = object_depot_obtain(&cache->depot);
		if (object) {
			T(Alloc(cache, flags, object));
			return object;
		}
	}

	MutexLocker _(cache->lock);
	slab* source;

	if (cache->partial.IsEmpty()) {
		if (cache->empty.IsEmpty()) {
			if (object_cache_reserve_internal(cache, 1, flags, false) < B_OK) {
				T(Alloc(cache, flags, NULL));
				return NULL;
			}

			cache->pressure++;
		}

		source = cache->empty.RemoveHead();
		cache->empty_count--;
		cache->partial.Add(source);
	} else {
		source = cache->partial.Head();
	}

	ParanoiaChecker _2(source);

	object_link* link = _pop(source->free);
	source->count--;
	cache->used_count++;

	if (cache->total_objects - cache->used_count < cache->min_object_reserve)
		increase_object_reserve(cache);

	REMOVE_PARANOIA_CHECK(PARANOIA_SUSPICIOUS, source, &link->next,
		sizeof(void*));

	TRACE_CACHE(cache, "allocate %p (%p) from %p, %lu remaining.",
		link_to_object(link, cache->object_size), link, source, source->count);

	if (source->count == 0) {
		cache->partial.Remove(source);
		cache->full.Add(source);
	}

	void* object = link_to_object(link, cache->object_size);
	T(Alloc(cache, flags, object));
	return object;
}


void
object_cache_free(object_cache* cache, void* object)
{
	if (object == NULL)
		return;

	T(Free(cache, object));

	if (!(cache->flags & CACHE_NO_DEPOT)) {
		if (object_depot_store(&cache->depot, object))
			return;
	}

	MutexLocker _(cache->lock);
	cache->ReturnObjectToSlab(cache->ObjectSlab(object), object);
}


static status_t
object_cache_reserve_internal(ObjectCache* cache, size_t objectCount,
	uint32 flags, bool unlockWhileAllocating)
{
	size_t numBytes = objectCount*  cache->object_size;
	size_t slabCount = ((numBytes - 1) / cache->slab_size) + 1;
		// TODO: This also counts the unusable space of each slab, which can
		// sum up.

	while (slabCount > 0) {
		slab* newSlab = cache->CreateSlab(flags, unlockWhileAllocating);
		if (newSlab == NULL)
			return B_NO_MEMORY;

		cache->empty.Add(newSlab);
		cache->empty_count++;
		slabCount--;
	}

	return B_OK;
}


status_t
object_cache_reserve(object_cache* cache, size_t objectCount, uint32 flags)
{
	if (objectCount == 0)
		return B_OK;

	T(Reserve(cache, objectCount, flags));

	MutexLocker _(cache->lock);
	return object_cache_reserve_internal(cache, objectCount, flags, false);
}


void
object_cache_get_usage(object_cache* cache, size_t* _allocatedMemory)
{
	MutexLocker _(cache->lock);
	*_allocatedMemory = cache->usage;
}


void
slab_init(kernel_args* args, addr_t initialBase, size_t initialSize)
{
	dprintf("slab: init base %p + 0x%lx\n", (void*)initialBase, initialSize);

	sInitialBegin = (uint8*)initialBase;
	sInitialLimit = sInitialBegin + initialSize;
	sInitialPointer = sInitialBegin;

	ObjectCache::SetKernelArgs(args);

	new (&sObjectCaches) ObjectCacheList();

	block_allocator_init_boot();

	add_debugger_command("slabs", dump_slabs, "list all object caches");
	add_debugger_command("cache_info", dump_cache_info,
		"dump information about a specific cache");
}


void
slab_init_post_sem()
{
	ObjectCacheList::Iterator it = sObjectCaches.GetIterator();

	while (it.HasNext()) {
		ObjectCache* cache = it.Next();
		cache->InitPostArea();
		register_low_resource_handler(object_cache_low_memory, cache,
			B_KERNEL_RESOURCE_PAGES | B_KERNEL_RESOURCE_MEMORY
				| B_KERNEL_RESOURCE_ADDRESS_SPACE, 5);

	}

	block_allocator_init_rest();
}


void
slab_init_post_thread()
{
	new(&sResizeRequests) ResizeRequestQueue;
	sResizeRequestsCondition.Init(&sResizeRequests, "object cache resizer");

	thread_id objectCacheResizer = spawn_kernel_thread(object_cache_resizer,
		"object cache resizer", B_URGENT_PRIORITY, NULL);
	if (objectCacheResizer < 0) {
		panic("slab_init_post_thread(): failed to spawn object cache resizer "
			"thread\n");
		return;
	}

	send_signal_etc(objectCacheResizer, SIGCONT, B_DO_NOT_RESCHEDULE);
}
