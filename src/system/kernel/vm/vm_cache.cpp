/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include <vm_cache.h>

#include <stddef.h>
#include <stdlib.h>

#include <arch/cpu.h>
#include <condition_variable.h>
#include <debug.h>
#include <heap.h>
#include <int.h>
#include <kernel.h>
#include <lock.h>
#include <smp.h>
#include <tracing.h>
#include <util/khash.h>
#include <util/AutoLock.h>
#include <vm.h>
#include <vm_page.h>
#include <vm_priv.h>
#include <vm_types.h>


//#define TRACE_VM_CACHE
#ifdef TRACE_VM_CACHE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#if DEBUG_CACHE_LIST
vm_cache* gDebugCacheList;
#endif
static mutex sCacheListLock = MUTEX_INITIALIZER("global vm_cache list");
	// The lock is also needed when the debug feature is disabled.


struct page_lookup_key {
	uint32	offset;
	vm_cache* cache;
};


#if VM_CACHE_TRACING

namespace VMCacheTracing {

class VMCacheTraceEntry : public AbstractTraceEntry {
	public:
		VMCacheTraceEntry(vm_cache* cache)
			:
			fCache(cache)
		{
		}

	protected:
		vm_cache*	fCache;
};


class Create : public VMCacheTraceEntry {
	public:
		Create(vm_cache* cache, vm_store* store)
			:
			VMCacheTraceEntry(cache),
			fStore(store)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache create: store: %p -> cache: %p", fStore,
				fCache);
		}

	private:
		vm_store*	fStore;
};


class Delete : public VMCacheTraceEntry {
	public:
		Delete(vm_cache* cache)
			:
			VMCacheTraceEntry(cache)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache delete: cache: %p", fCache);
		}
};


class SetMinimalCommitment : public VMCacheTraceEntry {
	public:
		SetMinimalCommitment(vm_cache* cache, off_t commitment)
			:
			VMCacheTraceEntry(cache),
			fOldCommitment(cache->store->committed_size),
			fCommitment(commitment)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache set min commitment: cache: %p, "
				"commitment: %lld -> %lld", fCache, fOldCommitment,
				fCommitment);
		}

	private:
		off_t	fOldCommitment;
		off_t	fCommitment;
};


class Resize : public VMCacheTraceEntry {
	public:
		Resize(vm_cache* cache, off_t size)
			:
			VMCacheTraceEntry(cache),
			fOldSize(cache->virtual_size),
			fSize(size)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache resize: cache: %p, size: %lld -> %lld", fCache,
				fOldSize, fSize);
		}

	private:
		off_t	fOldSize;
		off_t	fSize;
};


class AddConsumer : public VMCacheTraceEntry {
	public:
		AddConsumer(vm_cache* cache, vm_cache* consumer)
			:
			VMCacheTraceEntry(cache),
			fConsumer(consumer)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache add consumer: cache: %p, consumer: %p", fCache,
				fConsumer);
		}

	private:
		vm_cache*	fConsumer;
};


class RemoveConsumer : public VMCacheTraceEntry {
	public:
		RemoveConsumer(vm_cache* cache, vm_cache* consumer)
			:
			VMCacheTraceEntry(cache),
			fConsumer(consumer)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache remove consumer: cache: %p, consumer: %p",
				fCache, fConsumer);
		}

	private:
		vm_cache*	fConsumer;
};


class Merge : public VMCacheTraceEntry {
	public:
		Merge(vm_cache* cache, vm_cache* consumer)
			:
			VMCacheTraceEntry(cache),
			fConsumer(consumer)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache merge with consumer: cache: %p, consumer: %p",
				fCache, fConsumer);
		}

	private:
		vm_cache*	fConsumer;
};


class InsertArea : public VMCacheTraceEntry {
	public:
		InsertArea(vm_cache* cache, vm_area* area)
			:
			VMCacheTraceEntry(cache),
			fArea(area)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache insert area: cache: %p, area: %p", fCache,
				fArea);
		}

	private:
		vm_area*	fArea;
};


class RemoveArea : public VMCacheTraceEntry {
	public:
		RemoveArea(vm_cache* cache, vm_area* area)
			:
			VMCacheTraceEntry(cache),
			fArea(area)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache remove area: cache: %p, area: %p", fCache,
				fArea);
		}

	private:
		vm_area*	fArea;
};

}	// namespace VMCacheTracing

#	define T(x) new(std::nothrow) VMCacheTracing::x;

#	if VM_CACHE_TRACING >= 2

namespace VMCacheTracing {

class InsertPage : public VMCacheTraceEntry {
	public:
		InsertPage(vm_cache* cache, vm_page* page, off_t offset)
			:
			VMCacheTraceEntry(cache),
			fPage(page),
			fOffset(offset)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache insert page: cache: %p, page: %p, offset: %lld",
				fCache, fPage, fOffset);
		}

	private:
		vm_page*	fPage;
		off_t		fOffset;
};


class RemovePage : public VMCacheTraceEntry {
	public:
		RemovePage(vm_cache* cache, vm_page* page)
			:
			VMCacheTraceEntry(cache),
			fPage(page)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache remove page: cache: %p, page: %p", fCache,
				fPage);
		}

	private:
		vm_page*	fPage;
};

}	// namespace VMCacheTracing

#		define T2(x) new(std::nothrow) VMCacheTracing::x;
#	else
#		define T2(x) ;
#	endif
#else
#	define T(x) ;
#	define T2(x) ;
#endif


static int
page_compare_func(void* _p, const void* _key)
{
	vm_page* page = (vm_page*)_p;
	const struct page_lookup_key* key = (page_lookup_key*)_key;

	TRACE(("page_compare_func: page %p, key %p\n", page, key));

	if (page->cache == key->cache && page->cache_offset == key->offset)
		return 0;

	return -1;
}


static uint32
page_hash_func(void* _p, const void* _key, uint32 range)
{
	vm_page* page = (vm_page*)_p;
	const struct page_lookup_key* key = (page_lookup_key*)_key;

	#define HASH(offset, ref) ((offset) + ((uint32)(ref) >> 6) * 997)
		// sizeof(vm_cache) >= 64, hence (uint32)(ref) >> 6 is still unique

	if (page)
		return HASH(page->cache_offset, page->cache) % range;

	return HASH(key->offset, key->cache) % range;
}


/*!	Acquires a reference to a cache yet unreferenced by the caller. The
	caller must make sure, that the cache is not deleted, e.g. by holding the
	cache's source cache lock or by holding the global cache list lock.
	Returns \c true, if the reference could be acquired.
*/
static inline bool
acquire_unreferenced_cache_ref(vm_cache* cache)
{
	while (true) {
		int32 count = cache->ref_count;
		if (count == 0)
			return false;

		if (atomic_test_and_set(&cache->ref_count, count + 1, count) == count)
			return true;
	}
}


static void
delete_cache(vm_cache* cache)
{
	if (cache->areas != NULL)
		panic("cache %p to be deleted still has areas", cache);
	if (!list_is_empty(&cache->consumers))
		panic("cache %p to be deleted still has consumers", cache);

	T(Delete(cache));

	// delete the cache's backing store
	cache->store->ops->destroy(cache->store);

	// free all of the pages in the cache
	while (vm_page* page = cache->pages.Root()) {
		if (!page->mappings.IsEmpty() || page->wired_count != 0) {
			panic("remove page %p from cache %p: page still has mappings!\n",
				page, cache);
		}

		// remove it
		cache->pages.Remove(page);
		page->cache = NULL;
		// TODO: we also need to remove all of the page's mappings!

		TRACE(("vm_cache_release_ref: freeing page 0x%lx\n",
			oldPage->physical_page_number));
		vm_page_free(cache, page);
	}

	// remove the ref to the source
	if (cache->source)
		vm_cache_remove_consumer(cache->source, cache);

	// We lock and unlock the sCacheListLock, even if the DEBUG_CACHE_LIST is
	// not enabled. This synchronization point is needed for
	// vm_cache_acquire_page_cache_ref().
	mutex_lock(&sCacheListLock);

#if DEBUG_CACHE_LIST
	if (cache->debug_previous)
		cache->debug_previous->debug_next = cache->debug_next;
	if (cache->debug_next)
		cache->debug_next->debug_previous = cache->debug_previous;
	if (cache == gDebugCacheList)
		gDebugCacheList = cache->debug_next;
#endif

	mutex_unlock(&sCacheListLock);

	mutex_destroy(&cache->lock);
	free(cache);
}


static bool
is_cache_mergeable(vm_cache* cache)
{
	return (cache->areas == NULL && cache->temporary
		&& !list_is_empty(&cache->consumers)
		&& cache->consumers.link.next == cache->consumers.link.prev);
}


/*!	Merges the given cache with its only consumer.
	The caller must own a reference (other than the one from the consumer) and
	hold the cache's lock.
*/
static void
merge_cache_with_only_consumer(vm_cache* cache)
{
	vm_cache* consumer = (vm_cache*)list_get_first_item(&cache->consumers);

	if (!acquire_unreferenced_cache_ref(consumer))
		return;

	// In case we managed to grab a reference to the consumerRef,
	// this doesn't guarantee that we get the cache we wanted
	// to, so we need to check if this cache is really the last
	// consumer of the cache we want to merge it with.

	ConditionVariable busyCondition;

	// But since we need to keep the locking order upper->lower cache, we
	// need to unlock our cache now
	busyCondition.Publish(cache, "cache");
	cache->busy = true;

	while (true) {
		mutex_unlock(&cache->lock);

		mutex_lock(&consumer->lock);
		mutex_lock(&cache->lock);

		bool isMergeable = is_cache_mergeable(cache);
		if (isMergeable && consumer == list_get_first_item(&cache->consumers))
			break;

		// something changed, get rid of the consumer lock and ref
		mutex_unlock(&consumer->lock);
		vm_cache_release_ref(consumer);

		if (!isMergeable) {
			dprintf("merge_cache_with_only_consumer(): cache %p is no longer; "
				"mergeable\n", cache);
			cache->busy = false;
			busyCondition.Unpublish();
			return;
		}

		// the cache is still mergeable, just the consumer changed
		consumer = (vm_cache*)list_get_first_item(&cache->consumers);
	
		if (!acquire_unreferenced_cache_ref(consumer))
			return;
	}

	consumer = (vm_cache*)list_remove_head_item(&cache->consumers);

	TRACE(("merge vm cache %p (ref == %ld) with vm cache %p\n",
		cache, cache->ref_count, consumer));

	T(Merge(cache, consumer));

	for (VMCachePagesTree::Iterator it = cache->pages.GetIterator();
			vm_page* page = it.Next();) {
		// Note: Removing the current node while iterating through a
		// IteratableSplayTree is safe.
		vm_page* consumerPage = vm_cache_lookup_page(consumer,
			(off_t)page->cache_offset << PAGE_SHIFT);
		if (consumerPage == NULL) {
			// the page is not yet in the consumer cache - move it upwards
			vm_cache_remove_page(cache, page);
			vm_cache_insert_page(consumer, page,
				(off_t)page->cache_offset << PAGE_SHIFT);
		} else if (consumerPage->state == PAGE_STATE_BUSY
			&& consumerPage->type == PAGE_TYPE_DUMMY) {
			// the page is currently busy taking a read fault - IOW,
			// vm_soft_fault() has mapped our page so we can just
			// move it up
			//dprintf("%ld: merged busy page %p, cache %p, offset %ld\n", find_thread(NULL), page, cacheRef->cache, page->cache_offset);
			vm_cache_remove_page(consumer, consumerPage);
			consumerPage->state = PAGE_STATE_INACTIVE;
			((vm_dummy_page*)consumerPage)->busy_condition.Unpublish();

			vm_cache_remove_page(cache, page);
			vm_cache_insert_page(consumer, page,
				(off_t)page->cache_offset << PAGE_SHIFT);
#ifdef DEBUG_PAGE_CACHE_TRANSITIONS
		} else {
			page->debug_flags = 0;
			if (consumerPage->state == PAGE_STATE_BUSY)
				page->debug_flags |= 0x1;
			if (consumerPage->type == PAGE_TYPE_DUMMY)
				page->debug_flags |= 0x2;
			page->collided_page = consumerPage;
			consumerPage->collided_page = page;
#endif	// DEBUG_PAGE_CACHE_TRANSITIONS
		}
	}

	// The remaining consumer has got a new source.
	if (cache->source != NULL) {
		vm_cache* newSource = cache->source;

		mutex_lock(&newSource->lock);

		list_remove_item(&newSource->consumers, cache);
		list_add_item(&newSource->consumers, consumer);
		consumer->source = newSource;
		cache->source = NULL;

		mutex_unlock(&newSource->lock);
	} else
		consumer->source = NULL;

	// Release the reference the cache's consumer owned. The consumer takes
	// over the cache's ref to its source (if any) instead.
if (cache->ref_count < 2)
panic("cacheRef %p ref count too low!\n", cache);
	vm_cache_release_ref(cache);

	// Unpublishing the condition variable will wake up all threads waiting for
	// the cache. It will still remain marked busy, though. In fact "busy" is
	// a misnomer, since it actually means "to be deleted". Any thread we woke
	// up will not touch the cache anymore, but release its reference and retry
	// the hierarchy (cf. fault_find_page()).
	busyCondition.Unpublish();

	mutex_unlock(&consumer->lock);
	vm_cache_release_ref(consumer);
}


//	#pragma mark -


status_t
vm_cache_init(kernel_args* args)
{
	return B_OK;
}


vm_cache*
vm_cache_create(vm_store* store)
{
	vm_cache* cache;

	if (store == NULL) {
		panic("vm_cache created with NULL store!");
		return NULL;
	}

	cache = (vm_cache*)malloc_nogrow(sizeof(vm_cache));
	if (cache == NULL)
		return NULL;

	mutex_init(&cache->lock, "vm_cache");
	list_init_etc(&cache->consumers, offsetof(vm_cache, consumer_link));
	new(&cache->pages) VMCachePagesTree;
	cache->areas = NULL;
	cache->ref_count = 1;
	cache->source = NULL;
	cache->virtual_base = 0;
	cache->virtual_size = 0;
	cache->temporary = 0;
	cache->scan_skip = 0;
	cache->page_count = 0;
	cache->busy = false;


#if DEBUG_CACHE_LIST
	mutex_lock(&sCacheListLock);

	if (gDebugCacheList)
		gDebugCacheList->debug_previous = cache;
	cache->debug_previous = NULL;
	cache->debug_next = gDebugCacheList;
	gDebugCacheList = cache;

	mutex_unlock(&sCacheListLock);
#endif

	// connect the store to its cache
	cache->store = store;
	store->cache = cache;

	T(Create(cache, store));

	return cache;
}


void
vm_cache_acquire_ref(vm_cache* cache)
{
	TRACE(("vm_cache_acquire_ref: cache %p, ref will be %ld\n",
		cache, cache->ref_count + 1));

	if (cache == NULL)
		panic("vm_cache_acquire_ref: passed NULL\n");

	atomic_add(&cache->ref_count, 1);
}


void
vm_cache_release_ref(vm_cache* cache)
{
	TRACE(("vm_cache_release_ref: cacheRef %p, ref will be %ld\n",
		cache, cache->ref_count - 1));

	if (cache == NULL)
		panic("vm_cache_release_ref: passed NULL\n");

	if (atomic_add(&cache->ref_count, -1) != 1) {
#if 0
{
	// count min references to see if everything is okay
	struct stack_frame {
		struct stack_frame*	previous;
		void*				return_address;
	};
	int32 min = 0;
	vm_area* a;
	vm_cache* c;
	bool locked = false;
	if (cacheRef->lock.holder != find_thread(NULL)) {
		mutex_lock(&cacheRef->lock);
		locked = true;
	}
	for (a = cacheRef->areas; a != NULL; a = a->cache_next)
		min++;
	for (c = NULL; (c = list_get_next_item(&cacheRef->cache->consumers, c)) != NULL; )
		min++;
	dprintf("! %ld release cache_ref %p, ref_count is now %ld (min %ld, called from %p)\n",
		find_thread(NULL), cacheRef, cacheRef->ref_count,
		min, ((struct stack_frame*)x86_read_ebp())->return_address);
	if (cacheRef->ref_count < min)
		panic("cache_ref %p has too little ref_count!!!!", cacheRef);
	if (locked)
		mutex_unlock(&cacheRef->lock);
}
#endif
		return;
	}

	// delete this cache

	delete_cache(cache);
}


vm_cache*
vm_cache_acquire_page_cache_ref(vm_page* page)
{
	MutexLocker locker(sCacheListLock);

	vm_cache* cache = page->cache;
		// Getting/setting the pointer must be atomic on all architectures.
	if (cache == NULL)
		return NULL;

	// get a reference
	if (!acquire_unreferenced_cache_ref(cache))
		return NULL;

	return cache;
}


vm_page*
vm_cache_lookup_page(vm_cache* cache, off_t offset)
{
	ASSERT_LOCKED_MUTEX(&cache->lock);

	vm_page* page = cache->pages.Lookup((page_num_t)(offset >> PAGE_SHIFT));

	if (page != NULL && cache != page->cache)
		panic("page %p not in cache %p\n", page, cache);

	return page;
}


void
vm_cache_insert_page(vm_cache* cache, vm_page* page, off_t offset)
{
	TRACE(("vm_cache_insert_page: cache %p, page %p, offset %Ld\n",
		cache, page, offset));
	ASSERT_LOCKED_MUTEX(&cache->lock);

	if (page->cache != NULL) {
		panic("insert page %p into cache %p: page cache is set to %p\n",
			page, cache, page->cache);
	}

	T2(InsertPage(cache, page, offset));

	page->cache_offset = (page_num_t)(offset >> PAGE_SHIFT);
	cache->page_count++;
	page->usage_count = 2;
	page->cache = cache;

#if KDEBUG
	vm_page* otherPage = cache->pages.Lookup(page->cache_offset);
	if (otherPage != NULL) {
		panic("vm_cache_insert_page(): there's already page %p with cache "
			"offset %lu in cache %p; inserting page %p", otherPage,
			page->cache_offset, cache, page);
	}
#endif	// KDEBUG

	cache->pages.Insert(page);
}


/*!	Removes the vm_page from this cache. Of course, the page must
	really be in this cache or evil things will happen.
	The cache lock must be held.
*/
void
vm_cache_remove_page(vm_cache* cache, vm_page* page)
{
	TRACE(("vm_cache_remove_page: cache %p, page %p\n", cache, page));
	ASSERT_LOCKED_MUTEX(&cache->lock);

	if (page->cache != cache) {
		panic("remove page %p from cache %p: page cache is set to %p\n", page,
			cache, page->cache);
	}

	T2(RemovePage(cache, page));

	cache->pages.Remove(page);
	page->cache = NULL;
	cache->page_count--;
}


status_t
vm_cache_write_modified(vm_cache* cache, bool fsReenter)
{
	TRACE(("vm_cache_write_modified(cache = %p)\n", cache));

	if (cache->temporary)
		return B_OK;

	mutex_lock(&cache->lock);
	status_t status = vm_page_write_modified_pages(cache, fsReenter);
	mutex_unlock(&cache->lock);

	return status;
}


/*!	Commits the memory to the store if the \a commitment is larger than
	what's committed already.
	Assumes you have the \a ref's lock held.
*/
status_t
vm_cache_set_minimal_commitment_locked(vm_cache* cache, off_t commitment)
{
	TRACE(("vm_cache_set_minimal_commitment_locked(cache %p, commitment %Ld)\n",
		cache, commitment));
	ASSERT_LOCKED_MUTEX(&cache->lock);

	T(SetMinimalCommitment(cache, commitment));

	vm_store* store = cache->store;
	status_t status = B_OK;

	// If we don't have enough committed space to cover through to the new end
	// of the area...
	if (store->committed_size < commitment) {
		// ToDo: should we check if the cache's virtual size is large
		//	enough for a commitment of that size?

		// try to commit more memory
		status = store->ops->commit(store, commitment);
	}

	return status;
}


/*!	This function updates the size field of the vm_cache structure.
	If needed, it will free up all pages that don't belong to the cache anymore.
	The cache lock must be held when you call it.
	Since removed pages don't belong to the cache any longer, they are not
	written back before they will be removed.

	Note, this function way temporarily release the cache lock in case it
	has to wait for busy pages.
*/
status_t
vm_cache_resize(vm_cache* cache, off_t newSize)
{
	TRACE(("vm_cache_resize(cache %p, newSize %Ld) old size %Ld\n",
		cache, newSize, cache->virtual_size));
	ASSERT_LOCKED_MUTEX(&cache->lock);

	T(Resize(cache, newSize));

	status_t status = cache->store->ops->commit(cache->store, newSize);
	if (status != B_OK)
		return status;

	uint32 oldPageCount = (uint32)((cache->virtual_size + B_PAGE_SIZE - 1)
		>> PAGE_SHIFT);
	uint32 newPageCount = (uint32)((newSize + B_PAGE_SIZE - 1) >> PAGE_SHIFT);

	if (newPageCount < oldPageCount) {
		// we need to remove all pages in the cache outside of the new virtual
		// size
		for (VMCachePagesTree::Iterator it
					= cache->pages.GetIterator(newPageCount, true, true);
				vm_page* page = it.Next();) {
			if (page->state == PAGE_STATE_BUSY) {
				if (page->busy_writing) {
					// We cannot wait for the page to become available
					// as we might cause a deadlock this way
					page->busy_writing = false;
						// this will notify the writer to free the page
				} else {
					// wait for page to become unbusy
					ConditionVariableEntry entry;
					entry.Add(page);
					mutex_unlock(&cache->lock);
					entry.Wait();
					mutex_lock(&cache->lock);

					// restart from the start of the list
					it = cache->pages.GetIterator(newPageCount, true, true);
				}
				continue;
			}

			// remove the page and put it into the free queue
			vm_cache_remove_page(cache, page);
			vm_page_free(cache, page);
				// Note: When iterating through a IteratableSplayTree
				// removing the current node is safe.
		}
	}

	cache->virtual_size = newSize;
	return B_OK;
}


/*!	Removes the \a consumer from the \a cache.
	It will also release the reference to the cacheRef owned by the consumer.
	Assumes you have the consumer's cache lock held.
*/
void
vm_cache_remove_consumer(vm_cache* cache, vm_cache* consumer)
{
	TRACE(("remove consumer vm cache %p from cache %p\n", consumer, cache));
	ASSERT_LOCKED_MUTEX(&consumer->lock);

	T(RemoveConsumer(cache, consumer));

	// Remove the store ref before locking the cache. Otherwise we'd call into
	// the VFS while holding the cache lock, which would reverse the usual
	// locking order.
	if (cache->store->ops->release_ref)
		cache->store->ops->release_ref(cache->store);

	// remove the consumer from the cache, but keep its reference until later
	mutex_lock(&cache->lock);
	list_remove_item(&cache->consumers, consumer);
	consumer->source = NULL;

	// merge cache, if possible
	if (is_cache_mergeable(cache))
		merge_cache_with_only_consumer(cache);

	mutex_unlock(&cache->lock);
	vm_cache_release_ref(cache);
}


/*!	Marks the \a cache as source of the \a consumer cache,
	and adds the \a consumer to its list.
	This also grabs a reference to the source cache.
	Assumes you have the cache and the consumer's lock held.
*/
void
vm_cache_add_consumer_locked(vm_cache* cache, vm_cache* consumer)
{
	TRACE(("add consumer vm cache %p to cache %p\n", consumer, cache));
	ASSERT_LOCKED_MUTEX(&cache->lock);
	ASSERT_LOCKED_MUTEX(&consumer->lock);

	T(AddConsumer(cache, consumer));

	consumer->source = cache;
	list_add_item(&cache->consumers, consumer);

	vm_cache_acquire_ref(cache);

	if (cache->store->ops->acquire_ref != NULL)
		cache->store->ops->acquire_ref(cache->store);
}


/*!	Adds the \a area to the \a cache.
	Assumes you have the locked the cache.
*/
status_t
vm_cache_insert_area_locked(vm_cache* cache, vm_area* area)
{
	TRACE(("vm_cache_insert_area_locked(cache %p, area %p)\n", cache, area));
	ASSERT_LOCKED_MUTEX(&cache->lock);

	T(InsertArea(cache, area));

	area->cache_next = cache->areas;
	if (area->cache_next)
		area->cache_next->cache_prev = area;
	area->cache_prev = NULL;
	cache->areas = area;

	if (cache->store->ops->acquire_ref != NULL)
		cache->store->ops->acquire_ref(cache->store);

	return B_OK;
}


status_t
vm_cache_remove_area(vm_cache* cache, vm_area* area)
{
	TRACE(("vm_cache_remove_area(cache %p, area %p)\n", cache, area));

	T(RemoveArea(cache, area));

	MutexLocker locker(cache->lock);

	if (area->cache_prev)
		area->cache_prev->cache_next = area->cache_next;
	if (area->cache_next)
		area->cache_next->cache_prev = area->cache_prev;
	if (cache->areas == area)
		cache->areas = area->cache_next;

	if (cache->store->ops->release_ref)
		cache->store->ops->release_ref(cache->store);

	// merge cache, if possible
	if (is_cache_mergeable(cache))
		merge_cache_with_only_consumer(cache);

	return B_OK;
}
