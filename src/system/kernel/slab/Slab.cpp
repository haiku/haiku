/*
 * Copyright 2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Copyright 2008-2010, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <slab/Slab.h>

#include <algorithm>
#include <new>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <condition_variable.h>
#include <elf.h>
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
#include "MemoryManager.h"
#include "slab_debug.h"
#include "slab_private.h"
#include "SmallObjectCache.h"


typedef DoublyLinkedList<ObjectCache> ObjectCacheList;

typedef DoublyLinkedList<ObjectCache,
	DoublyLinkedListMemberGetLink<ObjectCache, &ObjectCache::maintenance_link> >
		MaintenanceQueue;

static ObjectCacheList sObjectCaches;
static mutex sObjectCacheListLock = MUTEX_INITIALIZER("object cache list");

static mutex sMaintenanceLock
	= MUTEX_INITIALIZER("object cache resize requests");
static MaintenanceQueue sMaintenanceQueue;
static ConditionVariable sMaintenanceCondition;


#if SLAB_ALLOCATION_TRACKING_AVAILABLE

struct caller_info {
	addr_t		caller;
	size_t		count;
	size_t		size;
};

static const int32 kCallerInfoTableSize = 1024;
static caller_info sCallerInfoTable[kCallerInfoTableSize];
static int32 sCallerInfoCount = 0;


RANGE_MARKER_FUNCTION_PROTOTYPES(slab_allocator)
RANGE_MARKER_FUNCTION_PROTOTYPES(SlabHashedObjectCache)
RANGE_MARKER_FUNCTION_PROTOTYPES(SlabMemoryManager)
RANGE_MARKER_FUNCTION_PROTOTYPES(SlabObjectCache)
RANGE_MARKER_FUNCTION_PROTOTYPES(SlabObjectDepot)
RANGE_MARKER_FUNCTION_PROTOTYPES(Slab)
RANGE_MARKER_FUNCTION_PROTOTYPES(SlabSmallObjectCache)


static const addr_t kSlabCodeAddressRanges[] = {
	RANGE_MARKER_FUNCTION_ADDRESS_RANGE(slab_allocator),
	RANGE_MARKER_FUNCTION_ADDRESS_RANGE(SlabHashedObjectCache),
	RANGE_MARKER_FUNCTION_ADDRESS_RANGE(SlabMemoryManager),
	RANGE_MARKER_FUNCTION_ADDRESS_RANGE(SlabObjectCache),
	RANGE_MARKER_FUNCTION_ADDRESS_RANGE(SlabObjectDepot),
	RANGE_MARKER_FUNCTION_ADDRESS_RANGE(Slab),
	RANGE_MARKER_FUNCTION_ADDRESS_RANGE(SlabSmallObjectCache)
};

static const uint32 kSlabCodeAddressRangeCount
	= sizeof(kSlabCodeAddressRanges) / sizeof(kSlabCodeAddressRanges[0]) / 2;

#endif	// SLAB_ALLOCATION_TRACKING_AVAILABLE


RANGE_MARKER_FUNCTION_BEGIN(Slab)


#if SLAB_OBJECT_CACHE_TRACING


namespace SlabObjectCacheTracing {

class ObjectCacheTraceEntry
	: public TRACE_ENTRY_SELECTOR(SLAB_OBJECT_CACHE_TRACING_STACK_TRACE) {
	public:
		ObjectCacheTraceEntry(ObjectCache* cache)
			:
			TraceEntryBase(SLAB_OBJECT_CACHE_TRACING_STACK_TRACE, 0, true),
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


}	// namespace SlabObjectCacheTracing

#	define T(x)	new(std::nothrow) SlabObjectCacheTracing::x

#else
#	define T(x)
#endif	// SLAB_OBJECT_CACHE_TRACING


// #pragma mark -


static void
dump_slab(::slab* slab)
{
	kprintf("  %p  %p  %6" B_PRIuSIZE " %6" B_PRIuSIZE " %6" B_PRIuSIZE "  %p\n",
		slab, slab->pages, slab->size, slab->count, slab->offset, slab->free);
}


static int
dump_slabs(int argc, char* argv[])
{
	kprintf("%10s %22s %8s %8s %8s %6s %8s %8s %8s\n", "address", "name",
		"objsize", "align", "usage", "empty", "usedobj", "total", "flags");

	ObjectCacheList::Iterator it = sObjectCaches.GetIterator();

	while (it.HasNext()) {
		ObjectCache* cache = it.Next();

		kprintf("%p %22s %8lu %8" B_PRIuSIZE " %8lu %6lu %8lu %8lu %8lx\n",
			cache, cache->name, cache->object_size, cache->alignment,
			cache->usage, cache->empty_count, cache->used_count,
			cache->total_objects, cache->flags);
	}

	return 0;
}


static int
dump_cache_info(int argc, char* argv[])
{
	if (argc < 2) {
		kprintf("usage: slab_cache [address]\n");
		return 0;
	}

	ObjectCache* cache = (ObjectCache*)parse_expression(argv[1]);

	kprintf("name:              %s\n", cache->name);
	kprintf("lock:              %p\n", &cache->lock);
	kprintf("object_size:       %lu\n", cache->object_size);
	kprintf("alignment:         %" B_PRIuSIZE "\n", cache->alignment);
	kprintf("cache_color_cycle: %lu\n", cache->cache_color_cycle);
	kprintf("total_objects:     %lu\n", cache->total_objects);
	kprintf("used_count:        %lu\n", cache->used_count);
	kprintf("empty_count:       %lu\n", cache->empty_count);
	kprintf("pressure:          %lu\n", cache->pressure);
	kprintf("slab_size:         %lu\n", cache->slab_size);
	kprintf("usage:             %lu\n", cache->usage);
	kprintf("maximum:           %lu\n", cache->maximum);
	kprintf("flags:             0x%lx\n", cache->flags);
	kprintf("cookie:            %p\n", cache->cookie);
	kprintf("resize entry don't wait: %p\n", cache->resize_entry_dont_wait);
	kprintf("resize entry can wait:   %p\n", cache->resize_entry_can_wait);

	kprintf("  slab        chunk         size   used offset  free\n");

	SlabList::Iterator iterator = cache->empty.GetIterator();
	if (iterator.HasNext())
		kprintf("empty:\n");
	while (::slab* slab = iterator.Next())
		dump_slab(slab);

	iterator = cache->partial.GetIterator();
	if (iterator.HasNext())
		kprintf("partial:\n");
	while (::slab* slab = iterator.Next())
		dump_slab(slab);

	iterator = cache->full.GetIterator();
	if (iterator.HasNext())
		kprintf("full:\n");
	while (::slab* slab = iterator.Next())
		dump_slab(slab);

	if ((cache->flags & CACHE_NO_DEPOT) == 0) {
		kprintf("depot:\n");
		dump_object_depot(&cache->depot);
	}

	return 0;
}


#if SLAB_ALLOCATION_TRACKING_AVAILABLE

#if SLAB_OBJECT_CACHE_ALLOCATION_TRACKING
	// until memory manager tracking is analyzed

static caller_info*
get_caller_info(addr_t caller)
{
	// find the caller info
	for (int32 i = 0; i < sCallerInfoCount; i++) {
		if (caller == sCallerInfoTable[i].caller)
			return &sCallerInfoTable[i];
	}

	// not found, add a new entry, if there are free slots
	if (sCallerInfoCount >= kCallerInfoTableSize)
		return NULL;

	caller_info* info = &sCallerInfoTable[sCallerInfoCount++];
	info->caller = caller;
	info->count = 0;
	info->size = 0;

	return info;
}

#endif	// SLAB_OBJECT_CACHE_ALLOCATION_TRACKING


static int
caller_info_compare_size(const void* _a, const void* _b)
{
	const caller_info* a = (const caller_info*)_a;
	const caller_info* b = (const caller_info*)_b;
	return (int)(b->size - a->size);
}


static int
caller_info_compare_count(const void* _a, const void* _b)
{
	const caller_info* a = (const caller_info*)_a;
	const caller_info* b = (const caller_info*)_b;
	return (int)(b->count - a->count);
}


#if SLAB_OBJECT_CACHE_ALLOCATION_TRACKING

static bool
analyze_allocation_callers(ObjectCache* cache, const SlabList& slabList,
	size_t& _totalAllocationSize, size_t& _totalAllocationCount)
{
	for (SlabList::ConstIterator it = slabList.GetIterator();
			slab* slab = it.Next();) {
		for (uint32 i = 0; i < slab->size; i++) {
			AllocationTrackingInfo* info = &slab->tracking[i];
			if (!info->IsInitialized())
				continue;

			_totalAllocationSize += cache->object_size;
			_totalAllocationCount++;

			addr_t caller = 0;
			AbstractTraceEntryWithStackTrace* traceEntry = info->TraceEntry();

			if (traceEntry != NULL && info->IsTraceEntryValid()) {
				caller = tracing_find_caller_in_stack_trace(
					traceEntry->StackTrace(), kSlabCodeAddressRanges,
					kSlabCodeAddressRangeCount);
			}

			caller_info* callerInfo = get_caller_info(caller);
			if (callerInfo == NULL) {
				kprintf("out of space for caller infos\n");
				return false;
			}

			callerInfo->count++;
			callerInfo->size += cache->object_size;
		}
	}

	return true;
}


static bool
analyze_allocation_callers(ObjectCache* cache, size_t& _totalAllocationSize,
	size_t& _totalAllocationCount)
{
	return analyze_allocation_callers(cache, cache->full, _totalAllocationSize,
			_totalAllocationCount)
		&& analyze_allocation_callers(cache, cache->partial,
			_totalAllocationSize, _totalAllocationCount);
}

#endif	// SLAB_OBJECT_CACHE_ALLOCATION_TRACKING


static int
dump_allocations_per_caller(int argc, char **argv)
{
	bool sortBySize = true;
	ObjectCache* cache = NULL;

	for (int32 i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-c") == 0) {
			sortBySize = false;
		} else if (strcmp(argv[i], "-o") == 0) {
			uint64 cacheAddress;
			if (++i >= argc
				|| !evaluate_debug_expression(argv[i], &cacheAddress, true)) {
				print_debugger_command_usage(argv[0]);
				return 0;
			}

			cache = (ObjectCache*)(addr_t)cacheAddress;
		} else {
			print_debugger_command_usage(argv[0]);
			return 0;
		}
	}

	sCallerInfoCount = 0;

	size_t totalAllocationSize = 0;
	size_t totalAllocationCount = 0;
	if (cache != NULL) {
#if SLAB_OBJECT_CACHE_ALLOCATION_TRACKING
		analyze_allocation_callers(cache, totalAllocationSize,
			totalAllocationCount);
#else
		kprintf("Object cache allocation tracking not available. "
			"SLAB_OBJECT_CACHE_TRACING (%d) and "
			"SLAB_OBJECT_CACHE_TRACING_STACK_TRACE (%d) must be enabled.\n",
			SLAB_OBJECT_CACHE_TRACING, SLAB_OBJECT_CACHE_TRACING_STACK_TRACE);
		return 0;
#endif
	} else {
#if SLAB_OBJECT_CACHE_ALLOCATION_TRACKING
		ObjectCacheList::Iterator it = sObjectCaches.GetIterator();

		while (it.HasNext()) {
			analyze_allocation_callers(it.Next(), totalAllocationSize,
				totalAllocationCount);
		}
#endif
	}

	// sort the array
	qsort(sCallerInfoTable, sCallerInfoCount, sizeof(caller_info),
		sortBySize ? &caller_info_compare_size : &caller_info_compare_count);

	kprintf("%ld different callers, sorted by %s...\n\n", sCallerInfoCount,
		sortBySize ? "size" : "count");

	kprintf("     count        size      caller\n");
	kprintf("----------------------------------\n");
	for (int32 i = 0; i < sCallerInfoCount; i++) {
		caller_info& info = sCallerInfoTable[i];
		kprintf("%10" B_PRIuSIZE "  %10" B_PRIuSIZE "  %p", info.count,
			info.size, (void*)info.caller);

		const char *symbol;
		const char *imageName;
		bool exactMatch;
		addr_t baseAddress;

		if (elf_debug_lookup_symbol_address(info.caller, &baseAddress, &symbol,
				&imageName, &exactMatch) == B_OK) {
			kprintf("  %s + %#" B_PRIxADDR " (%s)%s\n", symbol,
				info.caller - baseAddress, imageName,
				exactMatch ? "" : " (nearest)");
		} else
			kprintf("\n");
	}

	kprintf("\ntotal allocations: %" B_PRIuSIZE ", %" B_PRIuSIZE " bytes\n",
		totalAllocationCount, totalAllocationSize);

	return 0;
}

#endif	// SLAB_ALLOCATION_TRACKING_AVAILABLE


void
add_alloc_tracing_entry(ObjectCache* cache, uint32 flags, void* object)
{
#if SLAB_OBJECT_CACHE_TRACING
#if SLAB_OBJECT_CACHE_ALLOCATION_TRACKING
	cache->TrackingInfoFor(object)->Init(T(Alloc(cache, flags, object)));
#else
	T(Alloc(cache, flags, object));
#endif
#endif
}

// #pragma mark -


void
request_memory_manager_maintenance()
{
	MutexLocker locker(sMaintenanceLock);
	sMaintenanceCondition.NotifyAll();
}


// #pragma mark -


static void
delete_object_cache_internal(object_cache* cache)
{
	if (!(cache->flags & CACHE_NO_DEPOT))
		object_depot_destroy(&cache->depot, 0);

	mutex_lock(&cache->lock);

	if (!cache->full.IsEmpty())
		panic("cache destroy: still has full slabs");

	if (!cache->partial.IsEmpty())
		panic("cache destroy: still has partial slabs");

	while (!cache->empty.IsEmpty())
		cache->ReturnSlab(cache->empty.RemoveHead(), 0);

	mutex_destroy(&cache->lock);
	cache->Delete();
}


static void
increase_object_reserve(ObjectCache* cache)
{
	MutexLocker locker(sMaintenanceLock);

	cache->maintenance_resize = true;

	if (!cache->maintenance_pending) {
		cache->maintenance_pending = true;
		sMaintenanceQueue.Add(cache);
		sMaintenanceCondition.NotifyAll();
	}
}


/*!	Makes sure that \a objectCount objects can be allocated.
*/
static status_t
object_cache_reserve_internal(ObjectCache* cache, size_t objectCount,
	uint32 flags)
{
	// If someone else is already adding slabs, we wait for that to be finished
	// first.
	thread_id thread = find_thread(NULL);
	while (true) {
		if (objectCount <= cache->total_objects - cache->used_count)
			return B_OK;

		ObjectCacheResizeEntry* resizeEntry = NULL;
		if (cache->resize_entry_dont_wait != NULL) {
			resizeEntry = cache->resize_entry_dont_wait;
			if (thread == resizeEntry->thread)
				return B_WOULD_BLOCK;
			// Note: We could still have reentered the function, i.e.
			// resize_entry_can_wait would be ours. That doesn't matter much,
			// though, since after the don't-wait thread has done its job
			// everyone will be happy.
		} else if (cache->resize_entry_can_wait != NULL) {
			resizeEntry = cache->resize_entry_can_wait;
			if (thread == resizeEntry->thread)
				return B_WOULD_BLOCK;

			if ((flags & CACHE_DONT_WAIT_FOR_MEMORY) != 0)
				break;
		} else
			break;

		ConditionVariableEntry entry;
		resizeEntry->condition.Add(&entry);

		cache->Unlock();
		entry.Wait();
		cache->Lock();
	}

	// prepare the resize entry others can wait on
	ObjectCacheResizeEntry*& resizeEntry
		= (flags & CACHE_DONT_WAIT_FOR_MEMORY) != 0
			? cache->resize_entry_dont_wait : cache->resize_entry_can_wait;

	ObjectCacheResizeEntry myResizeEntry;
	resizeEntry = &myResizeEntry;
	resizeEntry->condition.Init(cache, "wait for slabs");
	resizeEntry->thread = thread;

	// add new slabs until there are as many free ones as requested
	while (objectCount > cache->total_objects - cache->used_count) {
		slab* newSlab = cache->CreateSlab(flags);
		if (newSlab == NULL) {
			resizeEntry->condition.NotifyAll();
			resizeEntry = NULL;
			return B_NO_MEMORY;
		}

		cache->usage += cache->slab_size;
		cache->total_objects += newSlab->size;

		cache->empty.Add(newSlab);
		cache->empty_count++;
	}

	resizeEntry->condition.NotifyAll();
	resizeEntry = NULL;

	return B_OK;
}


static void
object_cache_low_memory(void* dummy, uint32 resources, int32 level)
{
	if (level == B_NO_LOW_RESOURCE)
		return;

	MutexLocker cacheListLocker(sObjectCacheListLock);

	// Append the first cache to the end of the queue. We assume that it is
	// one of the caches that will never be deleted and thus we use it as a
	// marker.
	ObjectCache* firstCache = sObjectCaches.RemoveHead();
	sObjectCaches.Add(firstCache);
	cacheListLocker.Unlock();

	ObjectCache* cache;
	do {
		cacheListLocker.Lock();

		cache = sObjectCaches.RemoveHead();
		sObjectCaches.Add(cache);

		MutexLocker maintenanceLocker(sMaintenanceLock);
		if (cache->maintenance_pending || cache->maintenance_in_progress) {
			// We don't want to mess with caches in maintenance.
			continue;
		}

		cache->maintenance_pending = true;
		cache->maintenance_in_progress = true;

		maintenanceLocker.Unlock();
		cacheListLocker.Unlock();

		// We are calling the reclaimer without the object cache lock
		// to give the owner a chance to return objects to the slabs.

		if (cache->reclaimer)
			cache->reclaimer(cache->cookie, level);

		if ((cache->flags & CACHE_NO_DEPOT) == 0)
			object_depot_make_empty(&cache->depot, 0);

		MutexLocker cacheLocker(cache->lock);
		size_t minimumAllowed;

		switch (level) {
			case B_LOW_RESOURCE_NOTE:
				minimumAllowed = cache->pressure / 2 + 1;
				cache->pressure -= cache->pressure / 8;
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

		while (cache->empty_count > minimumAllowed) {
			// make sure we respect the cache's minimum object reserve
			size_t objectsPerSlab = cache->empty.Head()->size;
			size_t freeObjects = cache->total_objects - cache->used_count;
			if (freeObjects < cache->min_object_reserve + objectsPerSlab)
				break;

			cache->ReturnSlab(cache->empty.RemoveHead(), 0);
			cache->empty_count--;
		}

		cacheLocker.Unlock();

		// Check whether in the meantime someone has really requested
		// maintenance for the cache.
		maintenanceLocker.Lock();

		if (cache->maintenance_delete) {
			delete_object_cache_internal(cache);
			continue;
		}

		cache->maintenance_in_progress = false;

		if (cache->maintenance_resize)
			sMaintenanceQueue.Add(cache);
		else
			cache->maintenance_pending = false;
	} while (cache != firstCache);
}


static status_t
object_cache_maintainer(void*)
{
	while (true) {
		MutexLocker locker(sMaintenanceLock);

		// wait for the next request
		while (sMaintenanceQueue.IsEmpty()) {
			// perform memory manager maintenance, if needed
			if (MemoryManager::MaintenanceNeeded()) {
				locker.Unlock();
				MemoryManager::PerformMaintenance();
				locker.Lock();
				continue;
			}

			ConditionVariableEntry entry;
			sMaintenanceCondition.Add(&entry);
			locker.Unlock();
			entry.Wait();
			locker.Lock();
		}

		ObjectCache* cache = sMaintenanceQueue.RemoveHead();

		while (true) {
			bool resizeRequested = cache->maintenance_resize;
			bool deleteRequested = cache->maintenance_delete;

			if (!resizeRequested && !deleteRequested) {
				cache->maintenance_pending = false;
				cache->maintenance_in_progress = false;
				break;
			}

			cache->maintenance_resize = false;
			cache->maintenance_in_progress = true;

			locker.Unlock();

			if (deleteRequested) {
				delete_object_cache_internal(cache);
				break;
			}

			// resize the cache, if necessary

			MutexLocker cacheLocker(cache->lock);

			if (resizeRequested) {
				status_t error = object_cache_reserve_internal(cache,
					cache->min_object_reserve, 0);
				if (error != B_OK) {
					dprintf("object cache resizer: Failed to resize object "
						"cache %p!\n", cache);
					break;
				}
			}

			locker.Lock();
		}
	}

	// never can get here
	return B_OK;
}


// #pragma mark - public API


object_cache*
create_object_cache(const char* name, size_t object_size, size_t alignment,
	void* cookie, object_cache_constructor constructor,
	object_cache_destructor destructor)
{
	return create_object_cache_etc(name, object_size, alignment, 0, 0, 0, 0,
		cookie, constructor, destructor, NULL);
}


object_cache*
create_object_cache_etc(const char* name, size_t objectSize, size_t alignment,
	size_t maximum, size_t magazineCapacity, size_t maxMagazineCount,
	uint32 flags, void* cookie, object_cache_constructor constructor,
	object_cache_destructor destructor, object_cache_reclaimer reclaimer)
{
	ObjectCache* cache;

	if (objectSize == 0) {
		cache = NULL;
	} else if (objectSize <= 256) {
		cache = SmallObjectCache::Create(name, objectSize, alignment, maximum,
			magazineCapacity, maxMagazineCount, flags, cookie, constructor,
			destructor, reclaimer);
	} else {
		cache = HashedObjectCache::Create(name, objectSize, alignment, maximum,
			magazineCapacity, maxMagazineCount, flags, cookie, constructor,
			destructor, reclaimer);
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

	MutexLocker cacheLocker(cache->lock);

	{
		MutexLocker maintenanceLocker(sMaintenanceLock);
		if (cache->maintenance_in_progress) {
			// The maintainer thread is working with the cache. Just mark it
			// to be deleted.
			cache->maintenance_delete = true;
			return;
		}

		// unschedule maintenance
		if (cache->maintenance_pending)
			sMaintenanceQueue.Remove(cache);
	}

	// at this point no-one should have a reference to the cache anymore
	cacheLocker.Unlock();

	delete_object_cache_internal(cache);
}


status_t
object_cache_set_minimum_reserve(object_cache* cache, size_t objectCount)
{
	MutexLocker _(cache->lock);

	if (cache->min_object_reserve == objectCount)
		return B_OK;

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
			add_alloc_tracing_entry(cache, flags, object);
			return fill_allocated_block(object, cache->object_size);
		}
	}

	MutexLocker _(cache->lock);
	slab* source = NULL;

	while (true) {
		source = cache->partial.Head();
		if (source != NULL)
			break;

		source = cache->empty.RemoveHead();
		if (source != NULL) {
			cache->empty_count--;
			cache->partial.Add(source);
			break;
		}

		if (object_cache_reserve_internal(cache, 1, flags) != B_OK) {
			T(Alloc(cache, flags, NULL));
			return NULL;
		}

		cache->pressure++;
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
	add_alloc_tracing_entry(cache, flags, object);
	return fill_allocated_block(object, cache->object_size);
}


void
object_cache_free(object_cache* cache, void* object, uint32 flags)
{
	if (object == NULL)
		return;

	T(Free(cache, object));

#if PARANOID_KERNEL_FREE
	// TODO: allow forcing the check even if we don't find deadbeef
	if (*(uint32*)object == 0xdeadbeef) {
		if (!cache->AssertObjectNotFreed(object))
			return;

		if ((cache->flags & CACHE_NO_DEPOT) == 0) {
			if (object_depot_contains_object(&cache->depot, object)) {
				panic("object_cache: object %p is already freed", object);
				return;
			}
		}
	}

	fill_freed_block(object, cache->object_size);
#endif

#if SLAB_OBJECT_CACHE_ALLOCATION_TRACKING
	cache->TrackingInfoFor(object)->Clear();
#endif

	if ((cache->flags & CACHE_NO_DEPOT) == 0) {
		object_depot_store(&cache->depot, object, flags);
		return;
	}

	MutexLocker _(cache->lock);
	cache->ReturnObjectToSlab(cache->ObjectSlab(object), object, flags);
}


status_t
object_cache_reserve(object_cache* cache, size_t objectCount, uint32 flags)
{
	if (objectCount == 0)
		return B_OK;

	T(Reserve(cache, objectCount, flags));

	MutexLocker _(cache->lock);
	return object_cache_reserve_internal(cache, objectCount, flags);
}


void
object_cache_get_usage(object_cache* cache, size_t* _allocatedMemory)
{
	MutexLocker _(cache->lock);
	*_allocatedMemory = cache->usage;
}


void
slab_init(kernel_args* args)
{
	MemoryManager::Init(args);

	new (&sObjectCaches) ObjectCacheList();

	block_allocator_init_boot();
}


void
slab_init_post_area()
{
	MemoryManager::InitPostArea();

	add_debugger_command("slabs", dump_slabs, "list all object caches");
	add_debugger_command("slab_cache", dump_cache_info,
		"dump information about a specific object cache");
	add_debugger_command("slab_depot", dump_object_depot,
		"dump contents of an object depot");
	add_debugger_command("slab_magazine", dump_depot_magazine,
		"dump contents of a depot magazine");
#if SLAB_ALLOCATION_TRACKING_AVAILABLE
	add_debugger_command_etc("allocations_per_caller",
		&dump_allocations_per_caller,
		"Dump current heap allocations summed up per caller",
		"[ \"-c\" ] [ -o <object cache> ]\n"
		"The current allocations will by summed up by caller (their count and\n"
		"size) printed in decreasing order by size or, if \"-c\" is\n"
		"specified, by allocation count. If given <object cache> specifies\n"
		"the address of the object cache for which to print the allocations.\n",
		0);
#endif	// SLAB_ALLOCATION_TRACKING_AVAILABLE
}


void
slab_init_post_sem()
{
	register_low_resource_handler(object_cache_low_memory, NULL,
		B_KERNEL_RESOURCE_PAGES | B_KERNEL_RESOURCE_MEMORY
			| B_KERNEL_RESOURCE_ADDRESS_SPACE, 5);

	block_allocator_init_rest();
}


void
slab_init_post_thread()
{
	new(&sMaintenanceQueue) MaintenanceQueue;
	sMaintenanceCondition.Init(&sMaintenanceQueue, "object cache maintainer");

	thread_id objectCacheResizer = spawn_kernel_thread(object_cache_maintainer,
		"object cache resizer", B_URGENT_PRIORITY, NULL);
	if (objectCacheResizer < 0) {
		panic("slab_init_post_thread(): failed to spawn object cache resizer "
			"thread\n");
		return;
	}

	resume_thread(objectCacheResizer);
}


RANGE_MARKER_FUNCTION_END(Slab)
