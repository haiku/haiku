/*
 * Copyright 2008-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2003-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_VM_CACHE_H
#define _KERNEL_VM_VM_CACHE_H


#include <debug.h>
#include <kernel.h>
#include <util/DoublyLinkedList.h>
#include <vm/vm.h>
#include <vm/vm_types.h>

#include "kernel_debug_config.h"


struct kernel_args;
class ObjectCache;


enum {
	CACHE_TYPE_RAM = 0,
	CACHE_TYPE_VNODE,
	CACHE_TYPE_DEVICE,
	CACHE_TYPE_NULL
};

enum {
	PAGE_EVENT_NOT_BUSY	= 0x01		// page not busy anymore
};


extern ObjectCache* gCacheRefObjectCache;
extern ObjectCache* gAnonymousCacheObjectCache;
extern ObjectCache* gAnonymousNoSwapCacheObjectCache;
extern ObjectCache* gVnodeCacheObjectCache;
extern ObjectCache* gDeviceCacheObjectCache;
extern ObjectCache* gNullCacheObjectCache;


struct VMCachePagesTreeDefinition {
	typedef page_num_t KeyType;
	typedef	vm_page NodeType;

	static page_num_t GetKey(const NodeType* node)
	{
		return node->cache_offset;
	}

	static SplayTreeLink<NodeType>* GetLink(NodeType* node)
	{
		return &node->cache_link;
	}

	static int Compare(page_num_t key, const NodeType* node)
	{
		return key == node->cache_offset ? 0
			: (key < node->cache_offset ? -1 : 1);
	}

	static NodeType** GetListLink(NodeType* node)
	{
		return &node->cache_next;
	}
};

typedef IteratableSplayTree<VMCachePagesTreeDefinition> VMCachePagesTree;


struct VMCache : public DoublyLinkedListLinkImpl<VMCache> {
public:
	typedef DoublyLinkedList<VMCache> ConsumerList;

public:
								VMCache();
	virtual						~VMCache();

			status_t			Init(uint32 cacheType, uint32 allocationFlags);

	virtual	void				Delete();

	inline	bool				Lock();
	inline	bool				TryLock();
	inline	bool				SwitchLock(mutex* from);
	inline	bool				SwitchFromReadLock(rw_lock* from);
			void				Unlock(bool consumerLocked = false);
	inline	void				AssertLocked();

	inline	void				AcquireRefLocked();
	inline	void				AcquireRef();
	inline	void				ReleaseRefLocked();
	inline	void				ReleaseRef();
	inline	void				ReleaseRefAndUnlock(
									bool consumerLocked = false);

	inline	VMCacheRef*			CacheRef() const	{ return fCacheRef; }

			void				WaitForPageEvents(vm_page* page, uint32 events,
									bool relock);
			void				NotifyPageEvents(vm_page* page, uint32 events)
									{ if (fPageEventWaiters != NULL)
										_NotifyPageEvents(page, events); }
	inline	void				MarkPageUnbusy(vm_page* page);

			vm_page*			LookupPage(off_t offset);
			void				InsertPage(vm_page* page, off_t offset);
			void				RemovePage(vm_page* page);
			void				MovePage(vm_page* page);
			void				MoveAllPages(VMCache* fromCache);

	inline	page_num_t			WiredPagesCount() const;
	inline	void				IncrementWiredPagesCount();
	inline	void				DecrementWiredPagesCount();

			void				AddConsumer(VMCache* consumer);

			status_t			InsertAreaLocked(VMArea* area);
			status_t			RemoveArea(VMArea* area);
			void				TransferAreas(VMCache* fromCache);
			uint32				CountWritableAreas(VMArea* ignoreArea) const;

			status_t			WriteModified();
			status_t			SetMinimalCommitment(off_t commitment,
									int priority);
	virtual	status_t			Resize(off_t newSize, int priority);

			status_t			FlushAndRemoveAllPages();

			void*				UserData()	{ return fUserData; }
			void				SetUserData(void* data)	{ fUserData = data; }
									// Settable by the lock owner and valid as
									// long as the lock is owned.

			// for debugging only
			int32				RefCount() const
									{ return fRefCount; }

	// backing store operations
	virtual	status_t			Commit(off_t size, int priority);
	virtual	bool				HasPage(off_t offset);

	virtual	status_t			Read(off_t offset, const generic_io_vec *vecs,
									size_t count,uint32 flags,
									generic_size_t *_numBytes);
	virtual	status_t			Write(off_t offset, const generic_io_vec *vecs,
									size_t count, uint32 flags,
									generic_size_t *_numBytes);
	virtual	status_t			WriteAsync(off_t offset,
									const generic_io_vec* vecs, size_t count,
									generic_size_t numBytes, uint32 flags,
									AsyncIOCallback* callback);
	virtual	bool				CanWritePage(off_t offset);

	virtual	int32				MaxPagesPerWrite() const
									{ return -1; } // no restriction
	virtual	int32				MaxPagesPerAsyncWrite() const
									{ return -1; } // no restriction

	virtual	status_t			Fault(struct VMAddressSpace *aspace,
									off_t offset);

	virtual	void				Merge(VMCache* source);

	virtual	status_t			AcquireUnreferencedStoreRef();
	virtual	void				AcquireStoreRef();
	virtual	void				ReleaseStoreRef();

	virtual	bool				DebugHasPage(off_t offset);
			vm_page*			DebugLookupPage(off_t offset);

	virtual	void				Dump(bool showPages) const;

protected:
	virtual	void				DeleteObject() = 0;

public:
			VMArea*				areas;
			ConsumerList		consumers;
				// list of caches that use this cache as a source
			VMCachePagesTree	pages;
			VMCache*			source;
			off_t				virtual_base;
			off_t				virtual_end;
			off_t				committed_size;
				// TODO: Remove!
			uint32				page_count;
			uint32				temporary : 1;
			uint32				type : 6;

#if DEBUG_CACHE_LIST
			VMCache*			debug_previous;
			VMCache*			debug_next;
#endif

private:
			struct PageEventWaiter;
			friend struct VMCacheRef;

private:
			void				_NotifyPageEvents(vm_page* page, uint32 events);

	inline	bool				_IsMergeable() const;

			void				_MergeWithOnlyConsumer();
			void				_RemoveConsumer(VMCache* consumer);

private:
			int32				fRefCount;
			mutex				fLock;
			PageEventWaiter*	fPageEventWaiters;
			void*				fUserData;
			VMCacheRef*			fCacheRef;
			page_num_t			fWiredPagesCount;
};


#if DEBUG_CACHE_LIST
extern VMCache* gDebugCacheList;
#endif


class VMCacheFactory {
public:
	static	status_t		CreateAnonymousCache(VMCache*& cache,
								bool canOvercommit, int32 numPrecommittedPages,
								int32 numGuardPages, bool swappable,
								int priority);
	static	status_t		CreateVnodeCache(VMCache*& cache,
								struct vnode* vnode);
	static	status_t		CreateDeviceCache(VMCache*& cache,
								addr_t baseAddress);
	static	status_t		CreateNullCache(int priority, VMCache*& cache);
};



bool
VMCache::Lock()
{
	return mutex_lock(&fLock) == B_OK;
}


bool
VMCache::TryLock()
{
	return mutex_trylock(&fLock) == B_OK;
}


bool
VMCache::SwitchLock(mutex* from)
{
	return mutex_switch_lock(from, &fLock) == B_OK;
}


bool
VMCache::SwitchFromReadLock(rw_lock* from)
{
	return mutex_switch_from_read_lock(from, &fLock) == B_OK;
}


void
VMCache::AssertLocked()
{
	ASSERT_LOCKED_MUTEX(&fLock);
}


void
VMCache::AcquireRefLocked()
{
	ASSERT_LOCKED_MUTEX(&fLock);

	fRefCount++;
}


void
VMCache::AcquireRef()
{
	Lock();
	fRefCount++;
	Unlock();
}


void
VMCache::ReleaseRefLocked()
{
	ASSERT_LOCKED_MUTEX(&fLock);

	fRefCount--;
}


void
VMCache::ReleaseRef()
{
	Lock();
	fRefCount--;
	Unlock();
}


void
VMCache::ReleaseRefAndUnlock(bool consumerLocked)
{
	ReleaseRefLocked();
	Unlock(consumerLocked);
}


void
VMCache::MarkPageUnbusy(vm_page* page)
{
	ASSERT(page->busy);
	page->busy = false;
	NotifyPageEvents(page, PAGE_EVENT_NOT_BUSY);
}


page_num_t
VMCache::WiredPagesCount() const
{
	return fWiredPagesCount;
}


void
VMCache::IncrementWiredPagesCount()
{
	ASSERT(fWiredPagesCount < page_count);

	fWiredPagesCount++;
}


void
VMCache::DecrementWiredPagesCount()
{
	ASSERT(fWiredPagesCount > 0);

	fWiredPagesCount--;
}


// vm_page methods implemented here to avoid VMCache.h inclusion in vm_types.h

inline void
vm_page::IncrementWiredCount()
{
	if (fWiredCount++ == 0)
		cache_ref->cache->IncrementWiredPagesCount();
}


inline void
vm_page::DecrementWiredCount()
{
	if (--fWiredCount == 0)
		cache_ref->cache->DecrementWiredPagesCount();
}


#ifdef __cplusplus
extern "C" {
#endif

status_t vm_cache_init(struct kernel_args *args);
void vm_cache_init_post_heap();
struct VMCache *vm_cache_acquire_locked_page_cache(struct vm_page *page,
	bool dontWait);

#ifdef __cplusplus
}
#endif


#endif	/* _KERNEL_VM_VM_CACHE_H */
