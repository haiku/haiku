/*
 * Copyright 2008-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <vm/VMCache.h>

#include <stddef.h>
#include <stdlib.h>

#include <algorithm>

#include <arch/cpu.h>
#include <condition_variable.h>
#include <heap.h>
#include <interrupts.h>
#include <kernel.h>
#include <slab/Slab.h>
#include <smp.h>
#include <thread.h>
#include <tracing.h>
#include <util/AutoLock.h>
#include <vfs.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/vm_types.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>

// needed for the factory only
#include "VMAnonymousCache.h"
#include "VMAnonymousNoSwapCache.h"
#include "VMDeviceCache.h"
#include "VMNullCache.h"
#include "../cache/vnode_store.h"


//#define TRACE_VM_CACHE
#ifdef TRACE_VM_CACHE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#if DEBUG_CACHE_LIST
VMCache* gDebugCacheList;
#endif
static rw_lock sCacheListLock = RW_LOCK_INITIALIZER("global VMCache list");
	// The lock is also needed when the debug feature is disabled.

ObjectCache* gCacheRefObjectCache;
#if ENABLE_SWAP_SUPPORT
ObjectCache* gAnonymousCacheObjectCache;
#endif
ObjectCache* gAnonymousNoSwapCacheObjectCache;
ObjectCache* gVnodeCacheObjectCache;
ObjectCache* gDeviceCacheObjectCache;
ObjectCache* gNullCacheObjectCache;


struct VMCache::PageEventWaiter {
	Thread*				thread;
	PageEventWaiter*	next;
	vm_page*			page;
	uint32				events;
};


#if VM_CACHE_TRACING

namespace VMCacheTracing {

class VMCacheTraceEntry : public AbstractTraceEntry {
	public:
		VMCacheTraceEntry(VMCache* cache)
			:
			fCache(cache)
		{
#if VM_CACHE_TRACING_STACK_TRACE
			fStackTrace = capture_tracing_stack_trace(
				VM_CACHE_TRACING_STACK_TRACE, 0, true);
				// Don't capture userland stack trace to avoid potential
				// deadlocks.
#endif
		}

#if VM_CACHE_TRACING_STACK_TRACE
		virtual void DumpStackTrace(TraceOutput& out)
		{
			out.PrintStackTrace(fStackTrace);
		}
#endif

		VMCache* Cache() const
		{
			return fCache;
		}

	protected:
		VMCache*	fCache;
#if VM_CACHE_TRACING_STACK_TRACE
		tracing_stack_trace* fStackTrace;
#endif
};


class Create : public VMCacheTraceEntry {
	public:
		Create(VMCache* cache)
			:
			VMCacheTraceEntry(cache)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache create: -> cache: %p", fCache);
		}
};


class Delete : public VMCacheTraceEntry {
	public:
		Delete(VMCache* cache)
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
		SetMinimalCommitment(VMCache* cache, off_t commitment)
			:
			VMCacheTraceEntry(cache),
			fOldCommitment(cache->committed_size),
			fCommitment(commitment)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache set min commitment: cache: %p, "
				"commitment: %" B_PRIdOFF " -> %" B_PRIdOFF, fCache,
				fOldCommitment, fCommitment);
		}

	private:
		off_t	fOldCommitment;
		off_t	fCommitment;
};


class Resize : public VMCacheTraceEntry {
	public:
		Resize(VMCache* cache, off_t size)
			:
			VMCacheTraceEntry(cache),
			fOldSize(cache->virtual_end),
			fSize(size)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache resize: cache: %p, size: %" B_PRIdOFF " -> %"
				B_PRIdOFF, fCache, fOldSize, fSize);
		}

	private:
		off_t	fOldSize;
		off_t	fSize;
};


class Rebase : public VMCacheTraceEntry {
	public:
		Rebase(VMCache* cache, off_t base)
			:
			VMCacheTraceEntry(cache),
			fOldBase(cache->virtual_base),
			fBase(base)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache rebase: cache: %p, base: %lld -> %lld", fCache,
				fOldBase, fBase);
		}

	private:
		off_t	fOldBase;
		off_t	fBase;
};


class AddConsumer : public VMCacheTraceEntry {
	public:
		AddConsumer(VMCache* cache, VMCache* consumer)
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

		VMCache* Consumer() const
		{
			return fConsumer;
		}

	private:
		VMCache*	fConsumer;
};


class RemoveConsumer : public VMCacheTraceEntry {
	public:
		RemoveConsumer(VMCache* cache, VMCache* consumer)
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
		VMCache*	fConsumer;
};


class Merge : public VMCacheTraceEntry {
	public:
		Merge(VMCache* cache, VMCache* consumer)
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
		VMCache*	fConsumer;
};


class InsertArea : public VMCacheTraceEntry {
	public:
		InsertArea(VMCache* cache, VMArea* area)
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

		VMArea*	Area() const
		{
			return fArea;
		}

	private:
		VMArea*	fArea;
};


class RemoveArea : public VMCacheTraceEntry {
	public:
		RemoveArea(VMCache* cache, VMArea* area)
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
		VMArea*	fArea;
};

}	// namespace VMCacheTracing

#	define T(x) new(std::nothrow) VMCacheTracing::x;

#	if VM_CACHE_TRACING >= 2

namespace VMCacheTracing {

class InsertPage : public VMCacheTraceEntry {
	public:
		InsertPage(VMCache* cache, vm_page* page, off_t offset)
			:
			VMCacheTraceEntry(cache),
			fPage(page),
			fOffset(offset)
		{
			Initialized();
		}

		virtual void AddDump(TraceOutput& out)
		{
			out.Print("vm cache insert page: cache: %p, page: %p, offset: %"
				B_PRIdOFF, fCache, fPage, fOffset);
		}

	private:
		vm_page*	fPage;
		off_t		fOffset;
};


class RemovePage : public VMCacheTraceEntry {
	public:
		RemovePage(VMCache* cache, vm_page* page)
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


//	#pragma mark - debugger commands


#if VM_CACHE_TRACING


static void*
cache_stack_find_area_cache(const TraceEntryIterator& baseIterator, void* area)
{
	using namespace VMCacheTracing;

	// find the previous "insert area" entry for the given area
	TraceEntryIterator iterator = baseIterator;
	TraceEntry* entry = iterator.Current();
	while (entry != NULL) {
		if (InsertArea* insertAreaEntry = dynamic_cast<InsertArea*>(entry)) {
			if (insertAreaEntry->Area() == area)
				return insertAreaEntry->Cache();
		}

		entry = iterator.Previous();
	}

	return NULL;
}


static void*
cache_stack_find_consumer(const TraceEntryIterator& baseIterator, void* cache)
{
	using namespace VMCacheTracing;

	// find the previous "add consumer" or "create" entry for the given cache
	TraceEntryIterator iterator = baseIterator;
	TraceEntry* entry = iterator.Current();
	while (entry != NULL) {
		if (Create* createEntry = dynamic_cast<Create*>(entry)) {
			if (createEntry->Cache() == cache)
				return NULL;
		} else if (AddConsumer* addEntry = dynamic_cast<AddConsumer*>(entry)) {
			if (addEntry->Consumer() == cache)
				return addEntry->Cache();
		}

		entry = iterator.Previous();
	}

	return NULL;
}


static int
command_cache_stack(int argc, char** argv)
{
	if (argc < 3 || argc > 4) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	bool isArea = false;

	int argi = 1;
	if (argc == 4) {
		if (strcmp(argv[argi], "area") != 0) {
			print_debugger_command_usage(argv[0]);
			return 0;
		}

		argi++;
		isArea = true;
	}

	uint64 addressValue;
	uint64 debugEntryIndex;
	if (!evaluate_debug_expression(argv[argi++], &addressValue, false)
		|| !evaluate_debug_expression(argv[argi++], &debugEntryIndex, false)) {
		return 0;
	}

	TraceEntryIterator baseIterator;
	if (baseIterator.MoveTo((int32)debugEntryIndex) == NULL) {
		kprintf("Invalid tracing entry index %" B_PRIu64 "\n", debugEntryIndex);
		return 0;
	}

	void* address = (void*)(addr_t)addressValue;

	kprintf("cache stack for %s %p at %" B_PRIu64 ":\n",
		isArea ? "area" : "cache", address, debugEntryIndex);
	if (isArea) {
		address = cache_stack_find_area_cache(baseIterator, address);
		if (address == NULL) {
			kprintf("  cache not found\n");
			return 0;
		}
	}

	while (address != NULL) {
		kprintf("  %p\n", address);
		address = cache_stack_find_consumer(baseIterator, address);
	}

	return 0;
}


#endif	// VM_CACHE_TRACING


//	#pragma mark -


status_t
vm_cache_init(kernel_args* args)
{
	// Create object caches for the structures we allocate here.
	gCacheRefObjectCache = create_object_cache("cache refs", sizeof(VMCacheRef),
		0);
#if ENABLE_SWAP_SUPPORT
	gAnonymousCacheObjectCache = create_object_cache("anon caches",
		sizeof(VMAnonymousCache), 0);
#endif
	gAnonymousNoSwapCacheObjectCache = create_object_cache(
		"anon no-swap caches", sizeof(VMAnonymousNoSwapCache), 0);
	gVnodeCacheObjectCache = create_object_cache("vnode caches",
		sizeof(VMVnodeCache), 0);
	gDeviceCacheObjectCache = create_object_cache("device caches",
		sizeof(VMDeviceCache), 0);
	gNullCacheObjectCache = create_object_cache("null caches",
		sizeof(VMNullCache), 0);

	if (gCacheRefObjectCache == NULL
#if ENABLE_SWAP_SUPPORT
		|| gAnonymousCacheObjectCache == NULL
#endif
		|| gAnonymousNoSwapCacheObjectCache == NULL
		|| gVnodeCacheObjectCache == NULL
		|| gDeviceCacheObjectCache == NULL
		|| gNullCacheObjectCache == NULL) {
		panic("vm_cache_init(): Failed to create object caches!");
		return B_NO_MEMORY;
	}

	return B_OK;
}


void
vm_cache_init_post_heap()
{
#if VM_CACHE_TRACING
	add_debugger_command_etc("cache_stack", &command_cache_stack,
		"List the ancestors (sources) of a VMCache at the time given by "
			"tracing entry index",
		"[ \"area\" ] <address> <tracing entry index>\n"
		"All ancestors (sources) of a given VMCache at the time given by the\n"
		"tracing entry index are listed. If \"area\" is given the supplied\n"
		"address is an area instead of a cache address. The listing will\n"
		"start with the area's cache at that point.\n",
		0);
#endif	// VM_CACHE_TRACING
}


VMCache*
vm_cache_acquire_locked_page_cache(vm_page* page, bool dontWait)
{
	rw_lock_read_lock(&sCacheListLock);

	while (true) {
		VMCacheRef* cacheRef = page->CacheRef();
		if (cacheRef == NULL) {
			rw_lock_read_unlock(&sCacheListLock);
			return NULL;
		}

		VMCache* cache = cacheRef->cache;
		if (dontWait) {
			if (!cache->TryLock()) {
				rw_lock_read_unlock(&sCacheListLock);
				return NULL;
			}
		} else {
			if (!cache->SwitchFromReadLock(&sCacheListLock)) {
				// cache has been deleted
				rw_lock_read_lock(&sCacheListLock);
				continue;
			}
			rw_lock_read_lock(&sCacheListLock);
		}

		if (cache == page->Cache()) {
			rw_lock_read_unlock(&sCacheListLock);
			cache->AcquireRefLocked();
			return cache;
		}

		// the cache changed in the meantime
		cache->Unlock();
	}
}


// #pragma mark - VMCache


VMCacheRef::VMCacheRef(VMCache* cache)
	:
	cache(cache)
{
}


// #pragma mark - VMCache


bool
VMCache::_IsMergeable() const
{
	return areas.IsEmpty() && temporary
		&& !consumers.IsEmpty() && consumers.Head() == consumers.Tail();
}


VMCache::VMCache()
	:
	fCacheRef(NULL)
{
}


VMCache::~VMCache()
{
	ASSERT(fRefCount == 0 && page_count == 0);

	object_cache_delete(gCacheRefObjectCache, fCacheRef);
}


status_t
VMCache::Init(const char* name, uint32 cacheType, uint32 allocationFlags)
{
	mutex_init(&fLock, name);

	fRefCount = 1;
	source = NULL;
	virtual_base = 0;
	virtual_end = 0;
	committed_size = 0;
	temporary = 0;
	page_count = 0;
	fWiredPagesCount = 0;
	fFaultCount = 0;
	fCopiedPagesCount = 0;
	// SIEVE: Initialize SIEVE algorithm members
	sieve_page_list_head = NULL;
	sieve_page_list_tail = NULL;
	sieve_hand = NULL;
	type = cacheType;
	fPageEventWaiters = NULL;

#if DEBUG_CACHE_LIST
	debug_previous = NULL;
	debug_next = NULL;
		// initialize in case the following fails
#endif

	fCacheRef = new(gCacheRefObjectCache, allocationFlags) VMCacheRef(this);
	if (fCacheRef == NULL)
		return B_NO_MEMORY;

#if DEBUG_CACHE_LIST
	rw_lock_write_lock(&sCacheListLock);

	if (gDebugCacheList != NULL)
		gDebugCacheList->debug_previous = this;
	debug_next = gDebugCacheList;
	gDebugCacheList = this;

	rw_lock_write_unlock(&sCacheListLock);
#endif

	return B_OK;
}


void
VMCache::Delete()
{
	if (!areas.IsEmpty())
		panic("cache %p to be deleted still has areas", this);
	if (!consumers.IsEmpty())
		panic("cache %p to be deleted still has consumers", this);

	T(Delete(this));

	// free all of the pages in the cache
	vm_page_reservation reservation = {};
	while (vm_page* page = pages.Root()) {
		if (!page->mappings.IsEmpty() || page->WiredCount() != 0) {
			panic("remove page %p from cache %p: page still has mappings!\n"
				"@!page %p; cache %p", page, this, page, this);
		}

		// remove it
		pages.Remove(page);
		page->SetCacheRef(NULL);
		page_count--;

		TRACE(("vm_cache_release_ref: freeing page 0x%lx\n",
			page->physical_page_number));
		DEBUG_PAGE_ACCESS_START(page);
		vm_page_free_etc(this, page, &reservation);
	}
	vm_page_unreserve_pages(&reservation);

	// remove the ref to the source
	if (source)
		source->_RemoveConsumer(this);

	// We lock and unlock the sCacheListLock, even if the DEBUG_CACHE_LIST is
	// not enabled. This synchronization point is needed for
	// vm_cache_acquire_locked_page_cache().
	rw_lock_write_lock(&sCacheListLock);

#if DEBUG_CACHE_LIST
	if (debug_previous)
		debug_previous->debug_next = debug_next;
	if (debug_next)
		debug_next->debug_previous = debug_previous;
	if (this == gDebugCacheList)
		gDebugCacheList = debug_next;
#endif

	mutex_destroy(&fLock);

	rw_lock_write_unlock(&sCacheListLock);

	DeleteObject();
}


void
VMCache::Unlock(bool consumerLocked)
{
	while (fRefCount == 1 && _IsMergeable()) {
		VMCache* consumer = consumers.Head();
		if (consumerLocked) {
			_MergeWithOnlyConsumer();
		} else if (consumer->TryLock()) {
			_MergeWithOnlyConsumer();
			consumer->Unlock();
		} else {
			// Someone else has locked the consumer ATM. Unlock this cache and
			// wait for the consumer lock. Increment the cache's ref count
			// temporarily, so that no one else will try what we are doing or
			// delete the cache.
			fRefCount++;
			bool consumerLockedTemp = consumer->SwitchLock(&fLock);
			Lock();
			fRefCount--;

			if (consumerLockedTemp) {
				if (fRefCount == 1 && _IsMergeable()
						&& consumer == consumers.Head()) {
					// nothing has changed in the meantime -- merge
					_MergeWithOnlyConsumer();
				}

				consumer->Unlock();
			}
		}
	}

	if (fRefCount == 0) {
		// delete this cache
		Delete();
	} else
		mutex_unlock(&fLock);
}


vm_page*
VMCache::LookupPage(off_t offset)
{
	AssertLocked();

	vm_page* page = pages.Lookup((page_num_t)(offset >> PAGE_SHIFT));

#if KDEBUG
	if (page != NULL && page->Cache() != this)
		panic("page %p not in cache %p\n", page, this);
#endif

	return page;
}


void
VMCache::InsertPage(vm_page* page, off_t offset)
{
	TRACE(("VMCache::InsertPage(): cache %p, page %p, offset %" B_PRIdOFF "\n",
		this, page, offset));
	T2(InsertPage(this, page, offset));

	AssertLocked();
	ASSERT(offset >= virtual_base && offset < virtual_end);

	if (page->CacheRef() != NULL) {
		panic("insert page %p into cache %p: page cache is set to %p\n",
			page, this, page->Cache());
	}

	page->cache_offset = (page_num_t)(offset >> PAGE_SHIFT);
	page_count++;
	page->SetCacheRef(fCacheRef);

	// SIEVE: Add to our FIFO list and initialize visited status for SIEVE.
	page->sieve_visited = false; // New pages start as not visited.
	_SieveAddPageToHead(page);   // Add to the head of SIEVE's doubly linked list.

#if KDEBUG
	vm_page* otherPage = pages.Lookup(page->cache_offset);
	if (otherPage != NULL) {
		panic("VMCache::InsertPage(): there's already page %p with cache "
			"offset %" B_PRIuPHYSADDR " in cache %p; inserting page %p",
			otherPage, page->cache_offset, this, page);
	}
#endif	// KDEBUG

	pages.Insert(page);

	if (page->WiredCount() > 0)
		IncrementWiredPagesCount();
}


/*!	Removes the vm_page from this cache. Of course, the page must
	really be in this cache or evil things will happen.
	The cache lock must be held.
*/
void
VMCache::RemovePage(vm_page* page)
{
	TRACE(("VMCache::RemovePage(): cache %p, page %p\n", this, page));
	AssertLocked();

	if (page->Cache() != this) {
		panic("remove page %p from cache %p: page cache is set to %p\n", page,
			this, page->Cache());
	}

	T2(RemovePage(this, page));

	// SIEVE: Remove from our FIFO list. This must be done before clearing
	// cache_ref or other destructive operations on the page, as _SieveRemovePage
	// might need to access page->cache_offset for debugging, or other page fields.
	_SieveRemovePage(page);

	pages.Remove(page);
	page_count--;
	page->SetCacheRef(NULL);

	if (page->WiredCount() > 0)
		DecrementWiredPagesCount();
}


/*!	Moves the given page from its current cache inserts it into this cache
	at the given offset.
	Both caches must be locked.
*/
void
VMCache::MovePage(vm_page* page, off_t offset)
{
	VMCache* oldCache = page->Cache();

	AssertLocked();
	oldCache->AssertLocked();
	ASSERT(offset >= virtual_base && offset < virtual_end);

	// remove from old cache
	oldCache->pages.Remove(page);
	oldCache->page_count--;
	T2(RemovePage(oldCache, page));

	// change the offset
	page->cache_offset = offset >> PAGE_SHIFT;

	// insert here
	pages.Insert(page);
	page_count++;
	page->SetCacheRef(fCacheRef);

	if (page->WiredCount() > 0) {
		IncrementWiredPagesCount();
		oldCache->DecrementWiredPagesCount();
	}

	T2(InsertPage(this, page, page->cache_offset << PAGE_SHIFT));
}

/*!	Moves the given page from its current cache inserts it into this cache.
	Both caches must be locked.
*/
void
VMCache::MovePage(vm_page* page)
{
	MovePage(page, page->cache_offset << PAGE_SHIFT);
}


/*!	Moves all pages from the given cache to this one.
	Both caches must be locked. This cache must be empty.
*/
void
VMCache::MoveAllPages(VMCache* fromCache)
{
	AssertLocked();
	fromCache->AssertLocked();
	ASSERT(page_count == 0);

	std::swap(fromCache->pages, pages);
	page_count = fromCache->page_count;
	fromCache->page_count = 0;
	fWiredPagesCount = fromCache->fWiredPagesCount;
	fromCache->fWiredPagesCount = 0;

	// swap the VMCacheRefs
	rw_lock_write_lock(&sCacheListLock);
	std::swap(fCacheRef, fromCache->fCacheRef);
	fCacheRef->cache = this;
	fromCache->fCacheRef->cache = fromCache;
	rw_lock_write_unlock(&sCacheListLock);

#if VM_CACHE_TRACING >= 2
	for (VMCachePagesTree::Iterator it = pages.GetIterator();
			vm_page* page = it.Next();) {
		T2(RemovePage(fromCache, page));
		T2(InsertPage(this, page, page->cache_offset << PAGE_SHIFT));
	}
#endif
}


/*!	Waits until one or more events happened for a given page which belongs to
	this cache.
	The cache must be locked. It will be unlocked by the method. \a relock
	specifies whether the method shall re-lock the cache before returning.
	\param page The page for which to wait.
	\param events The mask of events the caller is interested in.
	\param relock If \c true, the cache will be locked when returning,
		otherwise it won't be locked.
*/
void
VMCache::WaitForPageEvents(vm_page* page, uint32 events, bool relock)
{
	PageEventWaiter waiter;
	waiter.thread = thread_get_current_thread();
	waiter.next = fPageEventWaiters;
	waiter.page = page;
	waiter.events = events;

	fPageEventWaiters = &waiter;

	thread_prepare_to_block(waiter.thread, 0, THREAD_BLOCK_TYPE_OTHER_OBJECT, page);

	Unlock();
	thread_block();

	if (relock)
		Lock();
}


/*!	Makes this cache the source of the \a consumer cache,
	and adds the \a consumer to its list.
	This also grabs a reference to the source cache.
	Assumes you have the cache and the consumer's lock held.
*/
void
VMCache::AddConsumer(VMCache* consumer)
{
	TRACE(("add consumer vm cache %p to cache %p\n", consumer, this));
	T(AddConsumer(this, consumer));

	AssertLocked();
	consumer->AssertLocked();
	ASSERT(consumer->source == NULL);

	consumer->source = this;
	consumers.Add(consumer);

	AcquireRefLocked();
	AcquireStoreRef();
}


/*!	Adds the \a area to this cache.
	Assumes you have the locked the cache.
*/
status_t
VMCache::InsertAreaLocked(VMArea* area)
{
	TRACE(("VMCache::InsertAreaLocked(cache %p, area %p)\n", this, area));
	T(InsertArea(this, area));

	AssertLocked();

	areas.Insert(area, false);

	AcquireStoreRef();

	return B_OK;
}


status_t
VMCache::RemoveArea(VMArea* area)
{
	TRACE(("VMCache::RemoveArea(cache %p, area %p)\n", this, area));

	T(RemoveArea(this, area));

	// We release the store reference first, since otherwise we would reverse
	// the locking order or even deadlock ourselves (... -> free_vnode() -> ...
	// -> bfs_remove_vnode() -> ... -> file_cache_set_size() -> mutex_lock()).
	// Also cf. _RemoveConsumer().
	ReleaseStoreRef();

	AutoLocker<VMCache> locker(this);

	areas.Remove(area);

	return B_OK;
}


/*!	Transfers the areas from \a fromCache to this cache. This cache must not
	have areas yet. Both caches must be locked.
*/
void
VMCache::TransferAreas(VMCache* fromCache)
{
	AssertLocked();
	fromCache->AssertLocked();
	ASSERT(areas.IsEmpty());

	areas.TakeFrom(&fromCache->areas);

	for (VMArea* area = areas.First(); area != NULL; area = areas.GetNext(area)) {
		area->cache = this;
		AcquireRefLocked();
		fromCache->ReleaseRefLocked();

		T(RemoveArea(fromCache, area));
		T(InsertArea(this, area));
	}
}


uint32
VMCache::CountWritableAreas(VMArea* ignoreArea) const
{
	uint32 count = 0;

	for (VMArea* area = areas.First(); area != NULL; area = areas.GetNext(area)) {
		if (area != ignoreArea
			&& (area->protection & (B_WRITE_AREA | B_KERNEL_WRITE_AREA)) != 0) {
			count++;
		}
	}

	return count;
}


status_t
VMCache::WriteModified()
{
	TRACE(("VMCache::WriteModified(cache = %p)\n", this));

	if (temporary)
		return B_OK;

	Lock();
	status_t status = vm_page_write_modified_pages(this);
	Unlock();

	return status;
}


/*!	Commits the memory to the store if the \a commitment is larger than
	what's committed already.
	Assumes you have the cache's lock held.
*/
status_t
VMCache::SetMinimalCommitment(off_t commitment, int priority)
{
	TRACE(("VMCache::SetMinimalCommitment(cache %p, commitment %" B_PRIdOFF
		")\n", this, commitment));
	T(SetMinimalCommitment(this, commitment));

	status_t status = B_OK;

	// If we don't have enough committed space to cover through to the new end
	// of the area...
	if (committed_size < commitment) {
#if KDEBUG
		const off_t size = PAGE_ALIGN(virtual_end - virtual_base);
		ASSERT_PRINT(commitment <= size, "cache %p, commitment %" B_PRIdOFF ", size %" B_PRIdOFF,
			this, commitment, size);
#endif

		// try to commit more memory
		status = Commit(commitment, priority);
	}

	return status;
}


bool
VMCache::_FreePageRange(VMCachePagesTree::Iterator it,
	page_num_t* toPage = NULL, page_num_t* freedPages = NULL)
{
	for (vm_page* page = it.Next();
		page != NULL && (toPage == NULL || page->cache_offset < *toPage);
		page = it.Next()) {

		if (page->busy) {
			if (page->busy_writing) {
				// We cannot wait for the page to become available
				// as we might cause a deadlock this way
				page->busy_writing = false;
					// this will notify the writer to free the page
				if (freedPages != NULL)
					(*freedPages)++;
				continue;
			}

			// wait for page to become unbusy
			WaitForPageEvents(page, PAGE_EVENT_NOT_BUSY, true);
			return true;
		}

		// remove the page and put it into the free queue
		DEBUG_PAGE_ACCESS_START(page);
		vm_remove_all_page_mappings(page);
		ASSERT(page->WiredCount() == 0);
			// TODO: Find a real solution! If the page is wired
			// temporarily (e.g. by lock_memory()), we actually must not
			// unmap it!
		RemovePage(page);
			// Note: When iterating through a IteratableSplayTree
			// removing the current node is safe.

		vm_page_free(this, page);
		if (freedPages != NULL)
			(*freedPages)++;
	}

	return false;
}


/*!	This function updates the size field of the cache.
	If needed, it will free up all pages that don't belong to the cache anymore.
	The cache lock must be held when you call it.
	Since removed pages don't belong to the cache any longer, they are not
	written back before they will be removed.

	Note, this function may temporarily release the cache lock in case it
	has to wait for busy pages.
*/
status_t
VMCache::Resize(off_t newSize, int priority)
{
	TRACE(("VMCache::Resize(cache %p, newSize %" B_PRIdOFF ") old size %"
		B_PRIdOFF "\n", this, newSize, this->virtual_end));
	T(Resize(this, newSize));

	AssertLocked();

	page_num_t oldPageCount = (page_num_t)((virtual_end + B_PAGE_SIZE - 1)
		>> PAGE_SHIFT);
	page_num_t newPageCount = (page_num_t)((newSize + B_PAGE_SIZE - 1)
		>> PAGE_SHIFT);

	if (newPageCount < oldPageCount) {
		// Remove all pages in the cache outside of the new virtual size.
		// Iterate carefully as RemovePage() modifies the collection.
		VMCachePagesTree::Iterator it = pages.GetIterator(newPageCount, true, true);
		vm_page* page = it.Next();
		while (page != NULL) {
			vm_page* nextPage = it.Next(); // Get next before removing current

			// These pages are being removed due to truncation, not memory pressure.
			// We must ensure they are not busy or wired before forcibly removing.
			// This is an existing concern in Resize; SIEVE doesn't change this core need.
			if (!page->busy && page->WiredCount() == 0) {
				// TODO: What if page is modified? Resize should ensure data is synced
				// or changes are irrelevant if the file is truncated.
				// For now, assume modified pages here are also removed.
				vm_remove_all_page_mappings(page); // Ensure unmapped
				RemovePage(page); // This now calls _SieveRemovePage()
				vm_page_free(this, page);
			} else {
				// Page is busy or wired. Cannot remove it immediately.
				// This might leave the cache temporarily larger than newSize implies.
				// The underlying file system or VFS layer is responsible for
				// ensuring I/O is complete and pages are un-wired before size changes propagate.
				// For this simulation, we'll just log if KDEBUG is on.
#if KDEBUG
				debug_printf("VMCache::Resize: Cannot remove page %p (offset %" B_PRIuPHYSADDR ") due to busy/wired state.\n",
					page, page->cache_offset);
#endif
			}
			page = nextPage;
		}
	}
	if (newSize < virtual_end && newPageCount > 0) {
		// We may have a partial page at the end of the cache that must be cleared.
		uint32 partialBytes = newSize % B_PAGE_SIZE;
		if (partialBytes != 0) {
			vm_page* page = LookupPage(newSize - partialBytes);
			if (page != NULL) {
				vm_memset_physical(page->physical_page_number * B_PAGE_SIZE
					+ partialBytes, 0, B_PAGE_SIZE - partialBytes);
			}
		}
	}

	if (priority >= 0) {
		status_t status = Commit(PAGE_ALIGN(newSize - virtual_base), priority);
		if (status != B_OK)
			return status;
	}

	virtual_end = newSize;
	return B_OK;
}

/*!	This function updates the virtual_base field of the cache.
	If needed, it will free up all pages that don't belong to the cache anymore.
	The cache lock must be held when you call it.
	Since removed pages don't belong to the cache any longer, they are not
	written back before they will be removed.

	Note, this function may temporarily release the cache lock in case it
	has to wait for busy pages.
*/
status_t
VMCache::Rebase(off_t newBase, int priority)
{
	TRACE(("VMCache::Rebase(cache %p, newBase %lld) old base %lld\n",
		this, newBase, this->virtual_base));
	T(Rebase(this, newBase));

	AssertLocked();

	page_num_t basePage = (page_num_t)(newBase >> PAGE_SHIFT);

	if (newBase > virtual_base) {
		// Remove all pages in the cache outside of the new virtual base.
		VMCachePagesTree::Iterator it = pages.GetIterator();
		vm_page* page = it.Next();
		while (page != NULL && page->cache_offset < basePage) {
			vm_page* nextPage = it.Next(); // Get next before removing current

			if (!page->busy && page->WiredCount() == 0) {
				// TODO: Handle modified pages similar to Resize.
				vm_remove_all_page_mappings(page);
				RemovePage(page); // This now calls _SieveRemovePage()
				vm_page_free(this, page);
			} else {
#if KDEBUG
				debug_printf("VMCache::Rebase: Cannot remove page %p (offset %" B_PRIuPHYSADDR ") due to busy/wired state.\n",
					page, page->cache_offset);
#endif
			}
			page = nextPage;
		}
	}

	if (priority >= 0) {
		status_t status = Commit(PAGE_ALIGN(virtual_end - newBase), priority);
		if (status != B_OK)
			return status;
	}

	virtual_base = newBase;
	return B_OK;
}


/*!	Moves pages in the given range from the source cache into this cache. Both
	caches must be locked.
*/
status_t
VMCache::Adopt(VMCache* source, off_t offset, off_t size, off_t newOffset)
{
	page_num_t startPage = offset >> PAGE_SHIFT;
	page_num_t endPage = (offset + size + B_PAGE_SIZE - 1) >> PAGE_SHIFT;
	off_t offsetChange = newOffset - offset;

	VMCachePagesTree::Iterator it = source->pages.GetIterator(startPage, true,
		true);
	for (vm_page* page = it.Next();
				page != NULL && page->cache_offset < endPage;
				page = it.Next()) {
		MovePage(page, (page->cache_offset << PAGE_SHIFT) + offsetChange);
	}

	return B_OK;
}


/*! Discards pages in the given range. */
ssize_t
VMCache::Discard(off_t offset, off_t size)
{
	page_num_t discardedPageCount = 0;
	page_num_t startPage = offset >> PAGE_SHIFT;
	page_num_t endPage = (offset + size + B_PAGE_SIZE - 1) >> PAGE_SHIFT;

	VMCachePagesTree::Iterator it = pages.GetIterator(startPage, true, true);
	vm_page* page = it.Next();
	while (page != NULL && page->cache_offset < endPage) {
		vm_page* nextPage = it.Next(); // Get next before removing current

		// Discard implies pages should be freed if possible.
		// Similar busy/wired/modified checks as Resize/Rebase apply.
		// For discard, modified pages are usually just thrown away, not written back.
		if (!page->busy && page->WiredCount() == 0) {
			if (page->modified) {
				// For discard, we usually don't write back modified pages.
				// They are simply being discarded. Mark as not modified.
				page->modified = false;
				// TODO: Consider if vm_page_set_state to CACHED is needed if it was MODIFIED
			}
			vm_remove_all_page_mappings(page);
			RemovePage(page); // This now calls _SieveRemovePage()
			vm_page_free(this, page);
			discardedPageCount++;
		} else {
#if KDEBUG
			debug_printf("VMCache::Discard: Cannot discard page %p (offset %" B_PRIuPHYSADDR ") due to busy/wired state.\n",
				page, page->cache_offset);
#endif
		}
		page = nextPage;
	}

	return (discardedPageCount * B_PAGE_SIZE);
}


/*!	You have to call this function with the VMCache lock held. */
status_t
VMCache::FlushAndRemoveAllPages()
{
	ASSERT_LOCKED_MUTEX(&fLock);

	while (page_count > 0) {
		// write back modified pages
		status_t status = vm_page_write_modified_pages(this);
		if (status != B_OK)
			return status;

		// remove pages
		for (VMCachePagesTree::Iterator it = pages.GetIterator();
				vm_page* page = it.Next();) {
			if (page->busy) {
				// wait for page to become unbusy
				WaitForPageEvents(page, PAGE_EVENT_NOT_BUSY, true);

				// restart from the start of the list
				it = pages.GetIterator();
				continue;
			}

			// skip modified pages -- they will be written back in the next
			// iteration
			if (page->State() == PAGE_STATE_MODIFIED)
				continue;

			// We can't remove mapped pages.
			if (page->IsMapped())
				return B_BUSY;

			DEBUG_PAGE_ACCESS_START(page);
			RemovePage(page);
			vm_page_free(this, page);
				// Note: When iterating through a IteratableSplayTree
				// removing the current node is safe.
		}
	}

	return B_OK;
}


bool
VMCache::CanOvercommit()
{
	return false;
}


status_t
VMCache::Commit(off_t size, int priority)
{
	ASSERT_UNREACHABLE();
	return B_NOT_SUPPORTED;
}


/*!	Returns whether the cache's underlying backing store could deliver the
	page at the given offset.

	Basically it returns whether a Read() at \a offset would at least read a
	partial page (assuming that no unexpected errors occur or the situation
	changes in the meantime).
*/
bool
VMCache::StoreHasPage(off_t offset)
{
	// In accordance with Fault() the default implementation doesn't have a
	// backing store and doesn't allow faults.
	return false;
}


status_t
VMCache::Read(off_t offset, const generic_io_vec *vecs, size_t count,
	uint32 flags, generic_size_t *_numBytes)
{
	return B_ERROR;
}


status_t
VMCache::Write(off_t offset, const generic_io_vec *vecs, size_t count,
	uint32 flags, generic_size_t *_numBytes)
{
	return B_ERROR;
}


status_t
VMCache::WriteAsync(off_t offset, const generic_io_vec* vecs, size_t count,
	generic_size_t numBytes, uint32 flags, AsyncIOCallback* callback)
{
	// Not supported, fall back to the synchronous hook.
	generic_size_t transferred = numBytes;
	status_t error = Write(offset, vecs, count, flags, &transferred);

	if (callback != NULL)
		callback->IOFinished(error, transferred != numBytes, transferred);

	return error;
}


/*!	\brief Returns whether the cache can write the page at the given offset.

	The cache must be locked when this function is invoked.

	@param offset The page offset.
	@return \c true, if the page can be written, \c false otherwise.
*/
bool
VMCache::CanWritePage(off_t offset)
{
	return false;
}


status_t
VMCache::Fault(struct VMAddressSpace *aspace, off_t offset)
{
	return B_BAD_ADDRESS;
}


void
VMCache::Merge(VMCache* source)
{
	const page_num_t firstOffset = ROUNDDOWN(virtual_base, B_PAGE_SIZE) >> PAGE_SHIFT,
		endOffset = (page_num_t)((virtual_end + B_PAGE_SIZE - 1) >> PAGE_SHIFT);

	VMCachePagesTree::Iterator it = source->pages.GetIterator();
	while (vm_page* page = it.Next()) {
		if (page->cache_offset < firstOffset || page->cache_offset >= endOffset)
			continue;

		// Note: Removing the current node while iterating through a
		// IteratableSplayTree is safe.
		vm_page* consumerPage = LookupPage(
			(off_t)page->cache_offset << PAGE_SHIFT);
		if (consumerPage == NULL) {
			// the page is not yet in the consumer cache - move it upwards
			MovePage(page);
		}
	}
}


status_t
VMCache::AcquireUnreferencedStoreRef()
{
	return B_ERROR;
}


void
VMCache::AcquireStoreRef()
{
}


void
VMCache::ReleaseStoreRef()
{
}


/*!	Kernel debugger version of StoreHasPage().
	Does not do any locking.
*/
bool
VMCache::DebugStoreHasPage(off_t offset)
{
	// default that works for all subclasses that don't lock anyway
	return StoreHasPage(offset);
}


/*!	Kernel debugger version of LookupPage().
	Does not do any locking.
*/
vm_page*
VMCache::DebugLookupPage(off_t offset)
{
	return pages.Lookup((page_num_t)(offset >> PAGE_SHIFT));
}


void
VMCache::Dump(bool showPages) const
{
	kprintf("CACHE %p:\n", this);
	kprintf("  ref_count:    %" B_PRId32 "\n", RefCount());
	kprintf("  source:       %p\n", source);
	kprintf("  type:         %s\n", vm_cache_type_to_string(type));
	kprintf("  virtual_base: 0x%" B_PRIx64 "\n", virtual_base);
	kprintf("  virtual_end:  0x%" B_PRIx64 "\n", virtual_end);
	kprintf("  temporary:    %" B_PRIu32 "\n", uint32(temporary));
	kprintf("  lock:         %p\n", &fLock);
#if KDEBUG
	kprintf("  lock.holder:  %" B_PRId32 "\n", fLock.holder);
#endif
	kprintf("  areas:\n");

	for (VMArea* area = areas.First(); area != NULL; area = areas.GetNext(area)) {
		kprintf("    area 0x%" B_PRIx32 ", %s\n", area->id, area->name);
		kprintf("\tbase_addr:  0x%lx, size: 0x%lx\n", area->Base(),
			area->Size());
		kprintf("\tprotection: 0x%" B_PRIx32 "\n", area->protection);
		kprintf("\towner:      0x%" B_PRIx32 "\n", area->address_space->ID());
	}

	kprintf("  consumers:\n");
	for (ConsumerList::ConstIterator it = consumers.GetIterator();
		 	VMCache* consumer = it.Next();) {
		kprintf("\t%p\n", consumer);
	}

	kprintf("  pages:\n");
	if (showPages) {
		for (VMCachePagesTree::ConstIterator it = pages.GetIterator();
				vm_page* page = it.Next();) {
			if (!vm_page_is_dummy(page)) {
				kprintf("\t%p ppn %#" B_PRIxPHYSADDR " offset %#" B_PRIxPHYSADDR
					" state %u (%s) wired_count %u\n", page,
					page->physical_page_number, page->cache_offset,
					page->State(), page_state_to_string(page->State()),
					page->WiredCount());
			} else {
				kprintf("\t%p DUMMY PAGE state %u (%s)\n",
					page, page->State(), page_state_to_string(page->State()));
			}
		}
	} else
		kprintf("\t%" B_PRIu32 " in cache\n", page_count);
}


/*!	Wakes up threads waiting for page events.
	\param page The page for which events occurred.
	\param events The mask of events that occurred.
*/
void
VMCache::_NotifyPageEvents(vm_page* page, uint32 events)
{
	PageEventWaiter** it = &fPageEventWaiters;
	while (PageEventWaiter* waiter = *it) {
		if (waiter->page == page && (waiter->events & events) != 0) {
			// remove from list and unblock
			*it = waiter->next;
			thread_unblock(waiter->thread, B_OK);
		} else
			it = &waiter->next;
	}
}


/*!	Merges the given cache with its only consumer.
	The caller must hold both the cache's and the consumer's lock. The method
	does release neither lock.
*/
void
VMCache::_MergeWithOnlyConsumer()
{
	VMCache* consumer = consumers.RemoveHead();

	TRACE(("merge vm cache %p (ref == %" B_PRId32 ") with vm cache %p\n",
		this, this->fRefCount, consumer));

	T(Merge(this, consumer));

	// merge the cache
	consumer->Merge(this);

	// The remaining consumer has got a new source.
	if (source != NULL) {
		VMCache* newSource = source;

		newSource->Lock();

		newSource->consumers.Remove(this);
		newSource->consumers.Add(consumer);
		consumer->source = newSource;
		source = NULL;

		newSource->Unlock();
	} else
		consumer->source = NULL;

	// Release the reference the cache's consumer owned. The consumer takes
	// over the cache's ref to its source (if any) instead.
	ReleaseRefLocked();
}


/*!	Removes the \a consumer from this cache.
	It will also release the reference to the cache owned by the consumer.
	Assumes you have the consumer's cache lock held. This cache must not be
	locked.
*/
void
VMCache::_RemoveConsumer(VMCache* consumer)
{
	TRACE(("remove consumer vm cache %p from cache %p\n", consumer, this));
	T(RemoveConsumer(this, consumer));

	consumer->AssertLocked();

	// Remove the consumer from the cache, but keep its reference until the end.
	Lock();
	consumers.Remove(consumer);
	consumer->source = NULL;
	Unlock();

	// Release the store ref without holding the cache lock, as calling into
	// the VFS while holding the cache lock would reverse the usual locking order.
	ReleaseStoreRef();

	// Now release the consumer's reference.
	ReleaseRef();
}


// #pragma mark - SIEVE Helper Methods


/*! Adds a page to the head of the SIEVE FIFO list (newest entry).
	The cache lock must be held.
	The page's sieve_visited flag should be set by the caller.
*/
void
VMCache::_SieveAddPageToHead(vm_page* page)
{
	AssertLocked();
	ASSERT(page->sieve_next == NULL && page->sieve_prev == NULL); // Page should not already be in a list

	page->sieve_next = sieve_page_list_head;
	page->sieve_prev = NULL;
	if (sieve_page_list_head != NULL)
		sieve_page_list_head->sieve_prev = page;
	sieve_page_list_head = page;

	if (sieve_page_list_tail == NULL) // If list was empty, new page is also the tail
		sieve_page_list_tail = page;

	// Initialize sieve_hand if the list was empty and now has its first page.
	// The SIEVE hand scans from oldest towards newest. If the list was empty,
	// the new page is both newest and oldest, so hand points to it (the tail).
	if (sieve_hand == NULL && sieve_page_list_tail != NULL) {
		sieve_hand = sieve_page_list_tail;
	}
}


/*! Removes a page from the SIEVE FIFO list.
	Adjusts sieve_hand if it points to the removed page.
	The cache lock must be held.
*/
void
VMCache::_SieveRemovePage(vm_page* page)
{
	AssertLocked();

	// If page is not in any list (e.g. sieve_next/prev are NULL and it's not head/tail)
	// this indicates a potential logic error or a double removal.
	if (page->sieve_next == NULL && page->sieve_prev == NULL &&
	    sieve_page_list_head != page && sieve_page_list_tail != page) {
#if KDEBUG
		// This might be noisy if pages are legitimately removed before full SIEVE integration
		// in all paths, but good for debugging initial implementation.
		debug_printf("VMCache::_SieveRemovePage: Page %p (offset %" B_PRIuPHYSADDR ") may not be in SIEVE list or was already removed.\n",
			page, page->cache_offset);
#endif
		// Ensure pointers are definitely NULL if it was an orphaned page and return.
		page->sieve_next = NULL;
		page->sieve_prev = NULL;
		return;
	}

	// Adjust sieve_hand if it points to the page being removed.
	// The SIEVE hand points to an object O. If O is evicted, the hand is advanced
	// to the next object in the list (towards older objects / tail).
	if (sieve_hand == page) {
		sieve_hand = page->sieve_prev; // Move hand towards older pages (tail)
		if (sieve_hand == NULL) {
			// If the removed page was the tail (oldest) and hand pointed to it,
			// the new hand should become the new tail.
			// This will be correctly set after list pointers are updated if new tail exists.
			// Or if list becomes empty, hand will be set to NULL below.
			// For now, if page->sieve_prev is NULL, hand becomes NULL temporarily.
		}
	}

	// Unlink the page
	if (page == sieve_page_list_head)
		sieve_page_list_head = page->sieve_next;
	if (page->sieve_prev != NULL)
		page->sieve_prev->sieve_next = page->sieve_next;

	if (page == sieve_page_list_tail)
		sieve_page_list_tail = page->sieve_prev;
	if (page->sieve_next != NULL)
		page->sieve_next->sieve_prev = page->sieve_prev;

	// Post-unlinking adjustments for sieve_hand
	if (sieve_page_list_head == NULL) { // List became empty
		ASSERT(sieve_page_list_tail == NULL);
		sieve_hand = NULL;
	} else if (sieve_hand == NULL && sieve_page_list_tail != NULL) {
		// If hand became NULL (e.g., was pointing to the tail which was removed,
		// and it was the only element or page->sieve_prev was NULL),
		// re-initialize it to the current tail.
		sieve_hand = sieve_page_list_tail;
	}


	page->sieve_next = NULL;
	page->sieve_prev = NULL;
}


/*! Finds a victim page for eviction using the SIEVE algorithm.
	The cache lock must be held.
	Scans from the current 'sieve_hand' towards older entries (tail of list).
	If a visited page is found, its visited bit is cleared, and the hand moves to it.
	If an unvisited page is found, it's returned as the victim.
	Returns the victim page, or NULL if no suitable (clean, not busy, not wired)
	victim is found after a full scan. The returned page is *not* removed from
	any cache structures by this function; that's the caller's responsibility.
*/
vm_page*
VMCache::_SieveFindVictimPage()
{
	AssertLocked();

	if (sieve_hand == NULL) { // Implies list is empty
		ASSERT(sieve_page_list_head == NULL && sieve_page_list_tail == NULL);
		return NULL;
	}

	vm_page* scanPointer = sieve_hand;

	// Loop at most twice through the list. Once for initial scan and demotions,
	// a second time to find a now unvisited page if all were initially visited.
	for (int pass = 0; pass < 2; pass++) {
		vm_page* current = scanPointer; // Start scan from current hand/scanPointer position
		do {
			// Candidate pages must be clean, not busy, and not wired.
			if (!current->busy && current->WiredCount() == 0
				&& !current->modified) {

				if (current->sieve_visited) {
					current->sieve_visited = false; // Demote
					sieve_hand = current;           // Move hand to this demoted page
				} else {
					// Found an unvisited, viable page. This is the victim.
					// The hand does not move from where it was before this find.
					return current;
				}
			}

			// Advance scanPointer towards the tail (older pages)
			current = current->sieve_prev;
			if (current == NULL) { // Wrapped around past the tail
				current = sieve_page_list_tail; // Reset scan to tail
				if (current == NULL) // List became empty during scan
					return NULL;
			}
		// Continue until we've scanned back to where this pass's scan started (sieve_hand for pass 0,
		// or the initial scanPointer if hand moved for subsequent passes)
		} while (current != scanPointer);

		// If we are here, it means a full pass was made from 'scanPointer'.
		// If pass 0, and we are here, all pages from original sieve_hand to tail then head to original sieve_hand
		// were either not viable or were visited (and now demoted). The sieve_hand is now at the
		// position of the last page whose visited bit was cleared.
		// The second pass (pass == 1) will scan again. If a page (now unvisited) is found, it's the victim.
		// If the second pass completes and we are here, no victim was found.
		if (pass == 0) {
			scanPointer = sieve_hand; // Start the second pass from the potentially new hand
		}
	}

	return NULL; // No suitable victim found after two full passes
}


// #pragma mark - SIEVE Helper Methods


/*! Adds a page to the head of the SIEVE FIFO list.
	The cache lock must be held.
*/
void
VMCache::_SieveAddPageToHead(vm_page* page)
{
	AssertLocked();
	ASSERT(page->sieve_next == NULL && page->sieve_prev == NULL); // Page should not already be in a list

	page->sieve_next = sieve_page_list_head;
	page->sieve_prev = NULL;
	if (sieve_page_list_head != NULL)
		sieve_page_list_head->sieve_prev = page;
	sieve_page_list_head = page;

	if (sieve_page_list_tail == NULL) // If list was empty, new page is also the tail
		sieve_page_list_tail = page;

	// Initialize sieve_hand if the list was empty and now has its first page.
	// According to SIEVE paper, hand is just a pointer, often to the last evicted object's position.
	// For a new list, pointing it to the tail (oldest, which is also the newest here) is a reasonable start.
	if (sieve_hand == NULL && sieve_page_list_tail != NULL) {
		sieve_hand = sieve_page_list_tail;
	}
}


/*! Removes a page from the SIEVE FIFO list.
	Adjusts sieve_hand if it points to the removed page.
	The cache lock must be held.
*/
void
VMCache::_SieveRemovePage(vm_page* page)
{
	AssertLocked();

	// If page is not in any list, this is a no-op for pointers, but indicates potential issue.
	if (page->sieve_next == NULL && page->sieve_prev == NULL &&
	    sieve_page_list_head != page && sieve_page_list_tail != page) {
#if KDEBUG
		debug_printf("VMCache::_SieveRemovePage: Page %p (offset %" B_PRIuPHYSADDR ") not in SIEVE list or double removal.\n",
			page, page->cache_offset);
#endif
		// Ensure pointers are definitely NULL if it was an orphaned page
		page->sieve_next = NULL;
		page->sieve_prev = NULL;
		return;
	}

	// Adjust sieve_hand if it points to the page being removed
	if (sieve_hand == page) {
		// Move hand to the previous (logically older) element in scan order (towards tail).
		// If removing the tail and hand is at tail, hand moves to new tail.
		// If removing head and hand is at head, hand moves to new head (which is page->sieve_next).
		// The SIEVE paper implies hand moves to next position *after* eviction.
		// If current hand is victim, hand should effectively point to where next scan would start.
		sieve_hand = page->sieve_prev; // Move towards older item (tail)
		if (sieve_hand == NULL) { // If removed page was the tail, hand moves to new tail
			sieve_hand = sieve_page_list_tail; // This will be updated below
		}
	}

	if (page == sieve_page_list_head)
		sieve_page_list_head = page->sieve_next;
	if (page->sieve_prev != NULL)
		page->sieve_prev->sieve_next = page->sieve_next;

	if (page == sieve_page_list_tail)
		sieve_page_list_tail = page->sieve_prev;
	if (page->sieve_next != NULL)
		page->sieve_next->sieve_prev = page->sieve_prev;

	if (sieve_page_list_head == NULL) { // List became empty
		ASSERT(sieve_page_list_tail == NULL);
		sieve_hand = NULL;
	} else if (sieve_hand == NULL && sieve_page_list_tail != NULL) {
		// If hand became NULL (e.g. was pointing to the only page that got removed)
		// re-initialize it to the tail, as this is where scan usually starts or wraps.
		sieve_hand = sieve_page_list_tail;
	}


	page->sieve_next = NULL;
	page->sieve_prev = NULL;
}


/*! Finds a victim page for eviction using the SIEVE algorithm.
	The cache lock must be held.
	Returns the victim page, or NULL if no suitable victim is found.
	The returned page is still part of all cache structures; the caller
	is responsible for its actual removal and freeing.
*/
vm_page*
VMCache::_SieveFindVictimPage()
{
	AssertLocked();

	if (sieve_hand == NULL) { // Implies list is empty
		ASSERT(sieve_page_list_head == NULL && sieve_page_list_tail == NULL);
		return NULL;
	}

	vm_page* scanPointer = sieve_hand;
	vm_page* const originalHandPosition = sieve_hand;
	bool handAdvancedInThisPass = false; // Tracks if hand moved in the current "conceptual" pass

	do {
		// Candidate pages must be clean, not busy, and not wired.
		if (!scanPointer->busy && scanPointer->WiredCount() == 0
			&& !scanPointer->modified) {

			if (scanPointer->sieve_visited) {
				scanPointer->sieve_visited = false; // Demote
				sieve_hand = scanPointer;           // Move hand to this demoted page
				handAdvancedInThisPass = true;
			} else {
				// Found an unvisited, viable page. This is the victim.
				return scanPointer;
			}
		}

		// Advance scanPointer towards the tail (older pages)
		scanPointer = scanPointer->sieve_prev;
		if (scanPointer == NULL) { // Wrapped around past the tail (which means we were at sieve_page_list_tail)
			scanPointer = sieve_page_list_tail; // Effectively, stay at tail or reset to tail for a "new" pass sense

			// Check if we've made a full conceptual pass starting from originalHandPosition
			// A full pass is tricky to define perfectly here without counting iterations or more state.
			// The core idea is: if we scan all objects and all were visited (and thus demoted),
			// the hand will end up at the last demoted object. A subsequent scan should then find it unvisited.
			// If originalHandPosition is reached again AND the hand didn't move (no demotions in this pass),
			// it implies no viable unvisited page was found among the scannable pages from originalHandPosition.
			if (scanPointer == originalHandPosition && !handAdvancedInThisPass) {
				// If the hand itself (which might be the originalHandPosition or a newly demoted page)
				// is now a candidate, return it.
				if (!sieve_hand->busy && sieve_hand->WiredCount() == 0
					&& !sieve_hand->modified && !sieve_hand->sieve_visited) {
					return sieve_hand;
				}
				return NULL; // No victim after a full scan without further demotions.
			}
			// If hand did move, the loop condition (scanPointer != originalHandPosition || handAdvancedInThisPass)
			// might need adjustment or we rely on eventually finding a non-visited page or the hand itself.
			// Resetting handAdvancedInThisPass if we consider this a "new" full scan from the (potentially new) hand.
			// For simplicity, the loop condition will eventually make scanPointer == sieve_hand.
		}
	} while (scanPointer != sieve_hand); // Loop until we scan back to the current hand.

	// After the loop, the current sieve_hand is the only remaining candidate.
	// This covers the case where all pages were visited and demoted, and the hand
	// made a full circle.
	if (!sieve_hand->busy && sieve_hand->WiredCount() == 0
		&& !sieve_hand->modified && !sieve_hand->sieve_visited) {
		return sieve_hand;
	}

	return NULL; // No suitable victim found
}


// #pragma mark - VMCacheFactory
	// TODO: Move to own source file!


/*static*/ status_t
VMCacheFactory::CreateAnonymousCache(VMCache*& _cache, bool canOvercommit,
	int32 numPrecommittedPages, int32 numGuardPages, bool swappable,
	int priority)
{
	uint32 allocationFlags = HEAP_DONT_WAIT_FOR_MEMORY
		| HEAP_DONT_LOCK_KERNEL_SPACE;
	if (priority >= VM_PRIORITY_VIP)
		allocationFlags |= HEAP_PRIORITY_VIP;

#if ENABLE_SWAP_SUPPORT
	if (swappable) {
		VMAnonymousCache* cache
			= new(gAnonymousCacheObjectCache, allocationFlags) VMAnonymousCache;
		if (cache == NULL)
			return B_NO_MEMORY;

		status_t error = cache->Init(canOvercommit, numPrecommittedPages,
			numGuardPages, allocationFlags);
		if (error != B_OK) {
			cache->Delete();
			return error;
		}

		T(Create(cache));

		_cache = cache;
		return B_OK;
	}
#endif

	VMAnonymousNoSwapCache* cache
		= new(gAnonymousNoSwapCacheObjectCache, allocationFlags)
			VMAnonymousNoSwapCache;
	if (cache == NULL)
		return B_NO_MEMORY;

	status_t error = cache->Init(canOvercommit, numPrecommittedPages,
		numGuardPages, allocationFlags);
	if (error != B_OK) {
		cache->Delete();
		return error;
	}

	T(Create(cache));

	_cache = cache;
	return B_OK;
}


/*static*/ status_t
VMCacheFactory::CreateVnodeCache(VMCache*& _cache, struct vnode* vnode)
{
	const uint32 allocationFlags = HEAP_DONT_WAIT_FOR_MEMORY
		| HEAP_DONT_LOCK_KERNEL_SPACE;
		// Note: Vnode cache creation is never VIP.

	VMVnodeCache* cache
		= new(gVnodeCacheObjectCache, allocationFlags) VMVnodeCache;
	if (cache == NULL)
		return B_NO_MEMORY;

	status_t error = cache->Init(vnode, allocationFlags);
	if (error != B_OK) {
		cache->Delete();
		return error;
	}

	T(Create(cache));

	_cache = cache;
	return B_OK;
}


/*static*/ status_t
VMCacheFactory::CreateDeviceCache(VMCache*& _cache, addr_t baseAddress)
{
	const uint32 allocationFlags = HEAP_DONT_WAIT_FOR_MEMORY
		| HEAP_DONT_LOCK_KERNEL_SPACE;
		// Note: Device cache creation is never VIP.

	VMDeviceCache* cache
		= new(gDeviceCacheObjectCache, allocationFlags) VMDeviceCache;
	if (cache == NULL)
		return B_NO_MEMORY;

	status_t error = cache->Init(baseAddress, allocationFlags);
	if (error != B_OK) {
		cache->Delete();
		return error;
	}

	T(Create(cache));

	_cache = cache;
	return B_OK;
}


/*static*/ status_t
VMCacheFactory::CreateNullCache(int priority, VMCache*& _cache)
{
	uint32 allocationFlags = HEAP_DONT_WAIT_FOR_MEMORY
		| HEAP_DONT_LOCK_KERNEL_SPACE;
	if (priority >= VM_PRIORITY_VIP)
		allocationFlags |= HEAP_PRIORITY_VIP;

	VMNullCache* cache
		= new(gNullCacheObjectCache, allocationFlags) VMNullCache;
	if (cache == NULL)
		return B_NO_MEMORY;

	status_t error = cache->Init(allocationFlags);
	if (error != B_OK) {
		cache->Delete();
		return error;
	}

	T(Create(cache));

	_cache = cache;
	return B_OK;
}
