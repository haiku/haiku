/*
 * Copyright 2004-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Axel Dörfler <axeld@pinc-software.de>
 * 		Ingo Weinhold <bonefish@cs.tu-berlin.de>
 */

#include "haiku_fs_cache.h"

#include <new>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <Debug.h>

#include <kernel/util/DoublyLinkedList.h>

#include "haiku_hash.h"
#include "haiku_lock.h"
#include "HaikuKernelVolume.h"
#include "kernel_emu.h"

#undef TRACE
//#define TRACE_FILE_CACHE
#ifdef TRACE_FILE_CACHE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

using std::nothrow;


// This is a hacked version of the kernel file cache implementation. The main
// part of the implementation didn't change that much -- some code not needed
// in userland has been removed, file_cache_ref, vm_cache_ref and vm_cache
// have been unified, and the complete interface to the VM (vm_*()) and the
// VFS (vfs_*()) has been re-implemented to suit our needs.
//
// The PagePool class is a data structure used for managing the pages (vm_page)
// allocated and assigned to caches (file_cache_ref). It has a list for free
// pages, i.e. those that are not assigned to any cache and can be reused at
// once. A second list contains those pages that belong to a cache, but are
// not in use by any of the functions. These pages have a reference count of
// 0. They will be stolen from the owning cache when a usable page is needed,
// there are no free pages anymore, and the limit of pages that shall be used
// at maximum has already been reached.
//
// The only purpose of the page reference count (vm_page::ref_count) is to
// indicate whether the page is in use (i.e. known to one or more of the cache
// functions currently being executed). vm_page_get_page(),
// vm_page_allocate_page(), and vm_cache_lookup_page acquire a reference to
// the page, vm_page_put_page() releases a reference.

// vm_page::state indicates in what state
// a page currently is. PAGE_STATE_FREE is only encountered for pages not
// belonging to a cache (being in page pool's free pages list, or just having
// been removed or not yet added). PAGE_STATE_ACTIVE/MODIFIED indicate that
// the page contains valid file data, in the latter case not yet written to
// disk. PAGE_STATE_BUSY means that the page is currently being manipulated by
// an operation, usually meaning that page data are being read from/written to
// disk. The required behavior when encountering a busy page, is to put the
// page, release the page pool and cache locks, wait a short time, and
// retry again later.
//
// Changing the page state requires having a reference to the page,
// holding the lock of the owning cache (if any) and the lock of the page pool
// (in case the page is not newly allocated).
//
// A page is in up to three lists. The free or unused list in the page pool
// (guarded by the page pool lock), the pages list of the cache the page
// belongs to, and the modified pages list of the cache (both cache lists
// are guarded by both the page pool and the cache lock). The modified pages
// list only contains PAGE_STATE_MODIFIED or PAGE_STATE_BUSY pages.


// maximum number of iovecs per request
#define MAX_IO_VECS			64	// 256 kB
#define MAX_FILE_IO_VECS	32
#define MAX_TEMP_IO_VECS	8

#define CACHED_FILE_EXTENTS	2
	// must be smaller than MAX_FILE_IO_VECS
	// ToDo: find out how much of these are typically used

using UserlandFS::KernelEmu::dprintf;
using UserlandFS::KernelEmu::panic;
#define user_memcpy(a, b, c) memcpy(a, b, c)

#define PAGE_ALIGN(x) (((x) + (B_PAGE_SIZE - 1)) & ~(B_PAGE_SIZE - 1))

namespace UserlandFS {
namespace HaikuKernelEmu {

enum {
	PAGE_STATE_ACTIVE = 0,
	PAGE_STATE_BUSY,
	PAGE_STATE_MODIFIED,
	PAGE_STATE_FREE
};

enum {
	PHYSICAL_PAGE_NO_WAIT = 0,
	PHYSICAL_PAGE_CAN_WAIT,
};

struct vm_page;
struct file_cache_ref;

typedef DoublyLinkedListLink<vm_page> page_link;

// vm page
struct vm_page {
	vm_page*			next;
	page_link			link_cache;
	page_link			link_cache_modified;
	page_link			link_pool;
	file_cache_ref*		cache;
	off_t				offset;
	void*				data;
	uint8				state;
	uint32				ref_count;

	vm_page()
		: next(NULL),
		  cache(NULL),
		  offset(0),
		  data(malloc(B_PAGE_SIZE)),
		  state(PAGE_STATE_FREE),
		  ref_count(1)
	{
	}

	~vm_page()
	{
		free(data);
	}

	addr_t				Address() const	{ return (addr_t)data; }

	static int Compare(void *_cacheEntry, const void *_key);
	static uint32 Hash(void *_cacheEntry, const void *_key, uint32 range);
};

struct file_extent {
	off_t			offset;
	file_io_vec		disk;
};

struct file_map {
	file_map();
	~file_map();

	file_extent *operator[](uint32 index);
	file_extent *ExtentAt(uint32 index);
	status_t Add(file_io_vec *vecs, size_t vecCount, off_t &lastOffset);
	void Free();

	union {
		file_extent	direct[CACHED_FILE_EXTENTS];
		file_extent	*array;
	};
	size_t			count;
};


static vm_page *vm_page_allocate_page(int state);
static void vm_page_get_page(vm_page* page);
static status_t vm_page_put_page(vm_page* page);
static status_t vm_page_set_state(vm_page *page, int state);

static void vm_cache_insert_page(file_cache_ref *cacheRef, vm_page *page,
	off_t offset);
static void vm_cache_remove_page(file_cache_ref *cacheRef, vm_page *page);
static vm_page *vm_cache_lookup_page(file_cache_ref *cacheRef, off_t page);
static status_t vm_cache_resize(file_cache_ref *cacheRef, off_t newSize);
static status_t vm_cache_write_modified(file_cache_ref *ref, bool fsReenter);

static status_t vfs_read_pages(int fd, off_t pos, const iovec *vecs,
	size_t count, size_t *_numBytes, bool fsReenter);
static status_t vfs_write_pages(int fd, off_t pos, const iovec *vecs,
	size_t count, size_t *_numBytes, bool fsReenter);
static status_t vfs_get_file_map(file_cache_ref *cache, off_t offset,
	size_t size, struct file_io_vec *vecs, size_t *_count);

static HaikuKernelVolume* file_cache_get_volume(mount_id mountID);

static status_t pages_io(file_cache_ref *ref, off_t offset, const iovec *vecs,
	size_t count, size_t *_numBytes, bool doWrite);


typedef DoublyLinkedList<vm_page,
	DoublyLinkedListMemberGetLink<vm_page,
		&vm_page::link_cache_modified> > cache_modified_page_list;

typedef DoublyLinkedList<vm_page,
	DoublyLinkedListMemberGetLink<vm_page,
		&vm_page::link_cache> > cache_page_list;

struct file_cache_ref {
	mutex						lock;
	mount_id					mountID;
	vnode_id					nodeID;
	fs_vnode					nodeHandle;
	int							deviceFD;
	off_t						virtual_size;

	cache_page_list				pages;
	cache_modified_page_list	modifiedPages;

	file_map					map;
};

struct page_hash_key {
	page_hash_key() {}
	page_hash_key(int fd, vnode_id id, off_t offset)
		: deviceFD(fd),
		  nodeID(id),
		  offset(offset)
	{
	}

	int				deviceFD;
	vnode_id		nodeID;
	off_t			offset;
};


struct volume_list_entry;
typedef DoublyLinkedListLink<volume_list_entry> volume_list_link;

struct volume_list_entry {
	volume_list_link	link;
	HaikuKernelVolume*	volume;
};

typedef DoublyLinkedList<volume_list_entry,
	DoublyLinkedListMemberGetLink<volume_list_entry,
		&volume_list_entry::link> > volume_list;

typedef DoublyLinkedList<vm_page,
	DoublyLinkedListMemberGetLink<vm_page,
		&vm_page::link_pool> > pool_page_list;

struct PagePool {

	PagePool()
		: pageHash(NULL),
		  unusedPages(),
		  freePages(),
		  allocatedPages(0)
	{
	}

	~PagePool()
	{
	}

	status_t Init()
	{
		status_t error = recursive_lock_init(&lock, "page pool");
		if (error != B_OK) {
			panic("PagePool: Failed to init lock\n");
			return error;
		}

		pageHash = hash_init(256, 0, &vm_page::Compare, &vm_page::Hash);
		if (pageHash == NULL) {
			panic("PagePool: Failed to create page hash\n");
			return B_NO_MEMORY;
		}

		return B_OK;
	}

	bool Lock() { return (recursive_lock_lock(&lock) == B_OK); }
	void Unlock() { recursive_lock_unlock(&lock); }

	recursive_lock	lock;
	hash_table*		pageHash;
	pool_page_list	unusedPages;
	pool_page_list	freePages;
	uint32			allocatedPages;
};

struct PagePutter {
	PagePutter(vm_page* page)
		: fPage(page)
	{
	}
	
	~PagePutter()
	{
		if (fPage)
			vm_page_put_page(fPage);
	}

private:
	vm_page*	fPage;
};

static PagePool sPagePool;
static const uint32 kMaxAllocatedPages = 1024;
static volume_list sVolumeList;
static mutex sVolumeListLock;


int
vm_page::Compare(void *_cacheEntry, const void *_key)
{
	vm_page *page = (vm_page*)_cacheEntry;
	const page_hash_key *key = (const page_hash_key *)_key;

	// device FD
	if (page->cache->deviceFD != key->deviceFD)
		return page->cache->deviceFD - key->deviceFD;

	// node ID
	if (page->cache->nodeID < key->nodeID)
		return -1;
	if (page->cache->nodeID > key->nodeID)
		return 1;

	// offset
	if (page->offset < key->offset)
		return -1;
	if (page->offset > key->offset)
		return 1;

	return 0;
}

uint32
vm_page::Hash(void *_cacheEntry, const void *_key, uint32 range)
{
	vm_page *page = (vm_page*)_cacheEntry;
	const page_hash_key *key = (const page_hash_key *)_key;

	int fd = (page ? page->cache->deviceFD : key->deviceFD);
	vnode_id id = (page ? page->cache->nodeID : key->nodeID);
	off_t offset = (page ? page->offset : key->offset);

	uint32 value = fd;
	value = value * 17 + id;
	value = value * 17 + offset;

	return value % range;
}


vm_page *
vm_page_allocate_page(int state)
{
	AutoLocker<PagePool> poolLocker(sPagePool);

	// is a queued free page available?
	vm_page* page = sPagePool.freePages.RemoveHead();
	if (page) {
		page->ref_count++;
		return page;
	}

	// no free page

#if 0
// TODO: Nice idea in principle, but causes a serious locking problem.
// vm_page_allocate_page() is always invoked with some cached locked at the
// same time. Thus locking a cache here to steal a page can cause a deadlock.
	// If the limit for allocated pages has been reached, we try to steal an
	// unused page.
	if (sPagePool.allocatedPages >= kMaxAllocatedPages
		&& !sPagePool.unusedPages.IsEmpty()) {
		// we grab the first non-busy page
		for (pool_page_list::Iterator it(&sPagePool.unusedPages);
			vm_page* currentPage = it.Next();) {
			if (currentPage->state != PAGE_STATE_BUSY) {
				it.Remove();
				page = currentPage;
				page->ref_count++;
				break;
			}
		}

		// If we've found a suitable page, we need to mark it busy, write it
		// if it was modified, and finally remove it from its cache.
		if (page != NULL) {
			bool modified = (page->state == PAGE_STATE_MODIFIED);

			// mark the page busy
			page->state = PAGE_STATE_BUSY;

			// We don't need the pool lock anymore.
			poolLocker.Unlock();

			file_cache_ref* cache = page->cache;

			// If the page is modified, we write it to the disk.
			if (modified) {
				// find out, how much to write, and remove the page from the
				// cache's modified pages list
				MutexLocker cacheLocker(cache->lock);
				size_t bytes = min_c(B_PAGE_SIZE,
					cache->virtual_size - page->offset);
				cache->modifiedPages.Remove(page);
				cacheLocker.Unlock();

				// if we need to write anything, do it now
				if (bytes > 0) {
					iovec vecs[1];
					vecs[0].iov_base = page->data;
					vecs[0].iov_len = bytes;
					status_t error = pages_io(cache, page->offset, vecs, 1,
						&bytes, true);
					if (error != B_OK) {
						// failed to write the page: bad, but we can't do
						// much about it
						dprintf("vm_page_allocate_page(): Failed to write "
							"page %p (cache %p, offset: %lld).\n", page,
							cache, page->offset);
					}
				}
			}

			// remove the page from the cache
			MutexLocker cacheLocker(cache->lock);
			vm_cache_remove_page(cache, page);

			// now it's ours
			page->state = PAGE_STATE_FREE;

			return page;
		}
	}
#endif	// 0

	// no page yet -- allocate a new one

	page = new(nothrow) vm_page;
	if (!page || !page->data) {
		delete page;
		return NULL;
	}

	sPagePool.allocatedPages++;

	return page;
}


static void
vm_page_get_page(vm_page* page)
{
	if (page) {
		AutoLocker<PagePool> _(sPagePool);

		// increase ref count
		page->ref_count++;

		// if the page was unused before, remove it from the unused pages list
		if (page->ref_count == 1)
			sPagePool.unusedPages.Remove(page);
	}
}


static status_t
vm_page_put_page(vm_page* page)
{
	if (!page)
		return B_BAD_VALUE;

	AutoLocker<PagePool> _(sPagePool);

	if (page->ref_count <= 0) {
		panic("vm_page_put_page(): page %p already unreferenced!\n", page);
		return B_BAD_VALUE;
	}

	// decrease ref count
	page->ref_count--;

	if (page->ref_count > 0)
		return B_OK;

	// the page is unreference now: add it to the unused or free pages list

	if (page->state == PAGE_STATE_FREE) {
		// page is free
		// if we've maxed out the allowed allocated page, free this one,
		// otherwise add it to the free list
		if (sPagePool.allocatedPages > kMaxAllocatedPages) {
			delete page;
			sPagePool.allocatedPages--;
		} else
			sPagePool.freePages.Add(page);
	} else {
		// page is is not free; add to unused pages list
		sPagePool.unusedPages.Add(page);
	}

	return B_OK;
}


status_t
vm_page_set_state(vm_page *page, int state)
{
	AutoLocker<PagePool> _(sPagePool);

	if (page->ref_count <= 0) {
		panic("vm_page_set_state(): page %p is already unreferenced!\n",
			page);
		return B_BAD_VALUE;
	}

	if (state == page->state)
		return B_OK;

	// If it was modified before, remove the page from the cache's modified
	// pages list.
	if (page->state == PAGE_STATE_MODIFIED && page->cache)
		page->cache->modifiedPages.Remove(page);

	switch (state) {
		case PAGE_STATE_ACTIVE:
		case PAGE_STATE_BUSY:
		case PAGE_STATE_FREE:
			page->state = state;
			break;

		case PAGE_STATE_MODIFIED:
		{
			page->state = state;

			// add page to the modified list of the cache
			if (!page->cache) {
				panic("vm_page_set_state(): setting page state of page %p "
					"to PAGE_STATE_MODIFIED, but page is not in a cache\n",
					page);
				return B_BAD_VALUE;
			}

			page->cache->modifiedPages.Add(page);

			break;
		}

		default:
			panic("vm_page_set_state(): invalid page state: %d\n", state);
			return B_BAD_VALUE;
	}

	return B_OK;
}


// cache must be locked
//
void
vm_cache_insert_page(file_cache_ref *cache, vm_page *page, off_t offset)
{
	AutoLocker<PagePool> _(sPagePool);

	if (page->cache != NULL) {
		panic("vm_cache_insert_page(%p, %p): page already in cache %p\n",
			cache, page, page->cache);
		return;
	}

	page->cache = cache;
	page->offset = offset;

	// insert page into hash
	status_t error = hash_insert(sPagePool.pageHash, page);
	if (error != B_OK) {
		panic("vm_cache_insert_page(): Failed to insert page %p into hash!\n",
			page);
		page->cache = NULL;
		page->offset = 0;
		return;
	}

	// add page to cache page list
	cache->pages.Add(page);
}


// cache must be locked
//
void
vm_cache_remove_page(file_cache_ref *cache, vm_page *page)
{
	if (cache != page->cache) {
		panic("vm_cache_remove_page(%p, %p): page is in cache %p\n",
			cache, page, page->cache);
		return;
	}

	AutoLocker<PagePool> _(sPagePool);

	if (page->state == PAGE_STATE_MODIFIED)
		cache->modifiedPages.Remove(page);

	cache->pages.Remove(page);

	hash_remove(sPagePool.pageHash, page);

	page->cache = NULL;
	page->offset = 0;
}


vm_page *
vm_cache_lookup_page(file_cache_ref *cache, off_t offset)
{
	if (!cache)
		return NULL;

	AutoLocker<PagePool> _(sPagePool);

	page_hash_key key(cache->deviceFD, cache->nodeID, offset);

	vm_page* page = (vm_page*)hash_lookup(sPagePool.pageHash, &key);

	if (page)
		vm_page_get_page(page);

	return page;
}


// cache must be locked
//
status_t
vm_cache_resize(file_cache_ref *cache, off_t newSize)
{
	off_t oldAlignedSize = PAGE_ALIGN(cache->virtual_size);
	off_t newAlignedSize = PAGE_ALIGN(newSize);

	cache->virtual_size = newSize;

	// if the aligned cache size increased or remained the same, we're done
	if (newAlignedSize >= oldAlignedSize)
		return B_OK;

	// the file shrinks, so we need to get rid of excess pages

	// Hold the page pool lock virtually all the time from now on.
	AutoLocker<PagePool> poolLocker(sPagePool);

	// For sake of efficiency we sort the cache's list of pages so that all
	// pages that need to be removed are at the beginning of the list.
	vm_page* page = cache->pages.Head();
	if (newAlignedSize > 0) {
		while (page) {
			vm_page* nextPage = cache->pages.GetNext(page);

			if (page->offset >= newAlignedSize) {
				// move to the beginning of the list
				cache->pages.Remove(page);
				cache->pages.Add(page, false);
			}

			page = nextPage;
		}
	}

	// now we remove and free the excess pages one by one
	while (true) {
		// get the first page in the list to remove
		// (since we sorted the list, this is usually very cheap)
		for (cache_page_list::Iterator it(&cache->pages); (page = it.Next());) {
			if (page->offset >= newAlignedSize)
				break;
		}

		// no more pages? -- then we're done
		if (!page)
			return B_OK;

		if (page->state == PAGE_STATE_BUSY) {
			// the page is busy -- wait a while and try again
			poolLocker.Unlock();
			mutex_unlock(&cache->lock);
			sleep(20000);
			mutex_lock(&cache->lock);
			poolLocker.Lock();
		} else {
			// the page isn't busy -- get rid of it
			vm_page_get_page(page);
			vm_cache_remove_page(cache, page);
			vm_page_set_state(page, PAGE_STATE_FREE);
			vm_page_put_page(page);
		}
	}

	return B_OK;
}


status_t
vm_cache_write_modified(file_cache_ref *cache, bool fsReenter)
{
	// TODO: Write more than one page at a time. To avoid deadlocks, when a
	// busy page is encountered the previously collected pages need to be
	// written. Otherwise as many pages as our on-stack array can contain
	// can be processed at once.
	MutexLocker locker(cache->lock);
	while (true) {	
		// get the next modified page and mark it busy
		vm_page* page = NULL;

		while (true) {
			// get the first modified page
			AutoLocker<PagePool> poolLocker(sPagePool);
			page = cache->modifiedPages.Head();

			if (!page)
				return B_OK;

			// if not marked busy, remove it and mark it busy
			if (page->state != PAGE_STATE_BUSY) {
				cache->modifiedPages.Remove(page);
				vm_page_get_page(page);
				page->state = PAGE_STATE_BUSY;
				break;
			}

			// page is busy -- wait a while
			vm_page_put_page(page);
			poolLocker.Unlock();
			locker.Unlock();
			sleep(20000);
			locker.Lock();
		}

		locker.Unlock();

		// write the page
		size_t bytes = min_c(B_PAGE_SIZE, cache->virtual_size - page->offset);
		iovec vecs[1];
		vecs[0].iov_base = page->data;
		vecs[0].iov_len = bytes;
		status_t error = pages_io(cache, page->offset, vecs, 1, &bytes, true);
		if (error != B_OK)
			return error;

		locker.Lock();

		vm_page_set_state(page, PAGE_STATE_ACTIVE);
		vm_page_put_page(page);
	}

	return B_OK;
}


status_t
vfs_read_pages(int fd, off_t pos, const iovec *vecs, size_t count,
	size_t *_numBytes, bool fsReenter)
{
	// check how much the iovecs allow us to read
	size_t toRead = 0;
	for (size_t i = 0; i < count; i++)
		toRead += vecs[i].iov_len;

	iovec* newVecs = NULL;
	if (*_numBytes < toRead) {
		// We're supposed to read less than specified by the vecs. Since
		// readv_pos() doesn't support this, we need to clone the vecs.
		newVecs = new(nothrow) iovec[count];
		if (!newVecs)
			return B_NO_MEMORY;

		size_t newCount = 0;
		for (size_t i = 0; i < count && toRead > 0; i++) {
			size_t vecLen = min_c(vecs[i].iov_len, toRead);
			newVecs[i].iov_base = vecs[i].iov_base;
			newVecs[i].iov_len = vecLen;
			toRead -= vecLen;
			newCount++;
		}

		vecs = newVecs;
		count = newCount;
	}

	ssize_t bytesRead = readv_pos(fd, pos, vecs, count);
	delete[] newVecs;
	if (bytesRead < 0)
		return bytesRead;

	*_numBytes = bytesRead;
	return B_OK;
}


status_t
vfs_write_pages(int fd, off_t pos, const iovec *vecs, size_t count,
	size_t *_numBytes, bool fsReenter)
{
	// check how much the iovecs allow us to write
	size_t toWrite = 0;
	for (size_t i = 0; i < count; i++)
		toWrite += vecs[i].iov_len;

	iovec* newVecs = NULL;
	if (*_numBytes < toWrite) {
		// We're supposed to write less than specified by the vecs. Since
		// writev_pos() doesn't support this, we need to clone the vecs.
		newVecs = new(nothrow) iovec[count];
		if (!newVecs)
			return B_NO_MEMORY;

		size_t newCount = 0;
		for (size_t i = 0; i < count && toWrite > 0; i++) {
			size_t vecLen = min_c(vecs[i].iov_len, toWrite);
			newVecs[i].iov_base = vecs[i].iov_base;
			newVecs[i].iov_len = vecLen;
			toWrite -= vecLen;
			newCount++;
		}

		vecs = newVecs;
		count = newCount;
	}

	ssize_t bytesWritten = writev_pos(fd, pos, vecs, count);
	delete[] newVecs;
	if (bytesWritten < 0)
		return bytesWritten;

	*_numBytes = bytesWritten;
	return B_OK;
}


// cache must be locked
//
status_t
vfs_get_file_map(file_cache_ref *cache, off_t offset, size_t size,
	struct file_io_vec *vecs, size_t *_count)
{
	// get the volume for the cache
	HaikuKernelVolume* volume = file_cache_get_volume(cache->mountID);
	if (!volume) {
		panic("vfs_get_file_map(): no volume for ID %ld\n", cache->mountID);
		return B_ERROR;
	}

	// if node handle is not cached, get it
	fs_vnode nodeHandle = cache->nodeHandle;
	if (!nodeHandle) {
		// TODO: Unlock cache while getting the handle?
		status_t error = UserlandFS::KernelEmu::get_vnode(cache->mountID,
			cache->nodeID, &nodeHandle);
		if (error != B_OK)
			return error;
		UserlandFS::KernelEmu::put_vnode(cache->mountID, cache->nodeID);

		// cache the handle
		cache->nodeHandle = nodeHandle;
	}

	return volume->GetFileMap(nodeHandle, offset, size, vecs, _count);
}


status_t
file_cache_init()
{
	status_t error = sPagePool.Init();
	if (error != B_OK)
		return error;

	error = mutex_init(&sVolumeListLock, "volume list");
	if (error != B_OK) {
		panic("file_cache_init: Failed to init volume list lock\n");
		return error;
	}

	return B_OK;
}


status_t
file_cache_register_volume(HaikuKernelVolume* volume)
{
	volume_list_entry* entry = new(nothrow) volume_list_entry;
	if (!entry)
		return B_NO_MEMORY;

	entry->volume = volume;

	MutexLocker _(sVolumeListLock);

	sVolumeList.Add(entry);

	return B_OK;
}


status_t
file_cache_unregister_volume(HaikuKernelVolume* volume)
{
	MutexLocker _(sVolumeListLock);

	// find the entry
	for (volume_list::Iterator it(&sVolumeList);
		 volume_list_entry* entry = it.Next();) {
		if (entry->volume == volume) {
			// found: remove and delete it
			it.Remove();
			delete entry;
			return B_OK;
		}
	}

	return B_BAD_VALUE;
}


static HaikuKernelVolume*
file_cache_get_volume(mount_id mountID)
{
	MutexLocker _(sVolumeListLock);

	// find the entry
	for (volume_list::Iterator it(&sVolumeList);
		 volume_list_entry* entry = it.Next();) {
		if (entry->volume->GetID() == mountID)
			return entry->volume;
	}

	return NULL;
}



// #pragma mark -


file_map::file_map()
{
	array = NULL;
	count = 0;
}


file_map::~file_map()
{
	Free();
}


file_extent *
file_map::operator[](uint32 index)
{
	return ExtentAt(index);
}


file_extent *
file_map::ExtentAt(uint32 index)
{
	if (index >= count)
		return NULL;

	if (count > CACHED_FILE_EXTENTS)
		return &array[index];

	return &direct[index];
}


status_t
file_map::Add(file_io_vec *vecs, size_t vecCount, off_t &lastOffset)
{
	TRACE(("file_map::Add(vecCount = %ld)\n", vecCount));

	off_t offset = 0;

	if (vecCount <= CACHED_FILE_EXTENTS && count == 0) {
		// just use the reserved area in the file_cache_ref structure
	} else {
		// TODO: once we can invalidate only parts of the file map,
		//	we might need to copy the previously cached file extends
		//	from the direct range
		file_extent *newMap = (file_extent *)realloc(array,
			(count + vecCount) * sizeof(file_extent));
		if (newMap == NULL)
			return B_NO_MEMORY;

		array = newMap;

		if (count != 0) {
			file_extent *extent = ExtentAt(count - 1);
			offset = extent->offset + extent->disk.length;
		}
	}

	int32 start = count;
	count += vecCount;

	for (uint32 i = 0; i < vecCount; i++) {
		file_extent *extent = ExtentAt(start + i);

		extent->offset = offset;
		extent->disk = vecs[i];

		offset += extent->disk.length;
	}

#ifdef TRACE_FILE_CACHE
	for (uint32 i = 0; i < count; i++) {
		file_extent *extent = ExtentAt(i);
		dprintf("[%ld] extend offset %Ld, disk offset %Ld, length %Ld\n",
			i, extent->offset, extent->disk.offset, extent->disk.length);
	}
#endif

	lastOffset = offset;
	return B_OK;
}


void
file_map::Free()
{
	if (count > CACHED_FILE_EXTENTS)
		free(array);

	array = NULL;
	count = 0;
}


//	#pragma mark -


static void
add_to_iovec(iovec *vecs, int32 &index, int32 max, addr_t address, size_t size)
{
	if (index > 0 && (addr_t)vecs[index - 1].iov_base + vecs[index - 1].iov_len == address) {
		// the iovec can be combined with the previous one
		vecs[index - 1].iov_len += size;
		return;
	}

	if (index == max)
		panic("no more space for iovecs!");

	// we need to start a new iovec
	vecs[index].iov_base = (void *)address;
	vecs[index].iov_len = size;
	index++;
}


static file_extent *
find_file_extent(file_cache_ref *ref, off_t offset, uint32 *_index)
{
	// TODO: do binary search

	for (uint32 index = 0; index < ref->map.count; index++) {
		file_extent *extent = ref->map[index];

		if (extent->offset <= offset
			&& extent->offset + extent->disk.length > offset) {
			if (_index)
				*_index = index;
			return extent;
		}
	}

	return NULL;
}


static status_t
get_file_map(file_cache_ref *ref, off_t offset, size_t size,
	file_io_vec *vecs, size_t *_count)
{
	size_t maxVecs = *_count;
	status_t status = B_OK;

	if (ref->map.count == 0) {
		// we don't yet have the map of this file, so let's grab it
		// (ordered by offset, so that we can do a binary search on them)

		mutex_lock(&ref->lock);

		// the file map could have been requested in the mean time
		if (ref->map.count == 0) {
			size_t vecCount = maxVecs;
			off_t mapOffset = 0;

			while (true) {
				status = vfs_get_file_map(ref, mapOffset, ~0UL, vecs, &vecCount);
				if (status < B_OK && status != B_BUFFER_OVERFLOW) {
					mutex_unlock(&ref->lock);
					return status;
				}

				status_t addStatus = ref->map.Add(vecs, vecCount, mapOffset);
				if (addStatus != B_OK) {
					// only clobber the status in case of failure
					status = addStatus;
				}

				if (status != B_BUFFER_OVERFLOW)
					break;

				// when we are here, the map has been stored in the array, and
				// the array size was still too small to cover the whole file
				vecCount = maxVecs;
			}
		}

		mutex_unlock(&ref->lock);
	}

	if (status != B_OK) {
		// We must invalidate the (part of the) map we already
		// have, as we cannot know if it's complete or not
		ref->map.Free();
		return status;
	}

	// We now have cached the map of this file, we now need to
	// translate it for the requested access.

	uint32 index;
	file_extent *fileExtent = find_file_extent(ref, offset, &index);
	if (fileExtent == NULL) {
		// access outside file bounds? But that's not our problem
		*_count = 0;
		return B_OK;
	}

	offset -= fileExtent->offset;
	vecs[0].offset = fileExtent->disk.offset + offset;
	vecs[0].length = fileExtent->disk.length - offset;

	if (vecs[0].length >= size || index >= ref->map.count - 1) {
		*_count = 1;
		return B_OK;
	}

	// copy the rest of the vecs

	size -= vecs[0].length;

	for (index = 1; index < ref->map.count;) {
		fileExtent++;

		vecs[index] = fileExtent->disk;
		index++;

		if (index >= maxVecs) {
			*_count = index;
			return B_BUFFER_OVERFLOW;
		}

		if (size <= fileExtent->disk.length)
			break;

		size -= fileExtent->disk.length;
	}

	*_count = index;
	return B_OK;
}


/*!
	Does the dirty work of translating the request into actual disk offsets
	and reads to or writes from the supplied iovecs as specified by \a doWrite.
*/
static status_t
pages_io(file_cache_ref *ref, off_t offset, const iovec *vecs, size_t count,
	size_t *_numBytes, bool doWrite)
{
	TRACE(("pages_io: ref = %p, offset = %Ld, size = %lu, vecCount = %lu, %s\n", ref, offset,
		*_numBytes, count, doWrite ? "write" : "read"));

	// translate the iovecs into direct device accesses
	file_io_vec fileVecs[MAX_FILE_IO_VECS];
	size_t fileVecCount = MAX_FILE_IO_VECS;
	size_t numBytes = *_numBytes;

	status_t status = get_file_map(ref, offset, numBytes, fileVecs,
		&fileVecCount);
	if (status < B_OK) {
		TRACE(("get_file_map(offset = %Ld, numBytes = %lu) failed\n", offset,
			numBytes));
		return status;
	}

	// TODO: handle array overflow gracefully!

#ifdef TRACE_FILE_CACHE
	dprintf("got %lu file vecs for %Ld:%lu:\n", fileVecCount, offset, numBytes);
	for (size_t i = 0; i < fileVecCount; i++) {
		dprintf("  [%lu] offset = %Ld, size = %Ld\n",
			i, fileVecs[i].offset, fileVecs[i].length);
	}
#endif

	if (fileVecCount == 0) {
		// There are no file vecs at this offset, so we're obviously trying
		// to access the file outside of its bounds
		TRACE(("pages_io: access outside of vnode %p at offset %Ld\n",
			ref->vnode, offset));
		return B_BAD_VALUE;
	}

	uint32 fileVecIndex;
	size_t size;

	if (!doWrite) {
		// now directly read the data from the device
		// the first file_io_vec can be read directly

		size = fileVecs[0].length;
		if (size > numBytes)
			size = numBytes;

		status = vfs_read_pages(ref->deviceFD, fileVecs[0].offset, vecs,
			count, &size, false);
		if (status < B_OK)
			return status;

		// TODO: this is a work-around for buggy device drivers!
		//	When our own drivers honour the length, we can:
		//	a) also use this direct I/O for writes (otherwise, it would
		//	   overwrite precious data)
		//	b) panic if the term below is true (at least for writes)
		if (size > fileVecs[0].length) {
			//dprintf("warning: device driver %p doesn't respect total length in read_pages() call!\n", ref->device);
			size = fileVecs[0].length;
		}

		ASSERT(size <= fileVecs[0].length);

		// If the file portion was contiguous, we're already done now
		if (size == numBytes)
			return B_OK;

		// if we reached the end of the file, we can return as well
		if (size != fileVecs[0].length) {
			*_numBytes = size;
			return B_OK;
		}

		fileVecIndex = 1;
	} else {
		fileVecIndex = 0;
		size = 0;
	}

	// Too bad, let's process the rest of the file_io_vecs

	size_t totalSize = size;

	// first, find out where we have to continue in our iovecs
	uint32 i = 0;
	for (; i < count; i++) {
		if (size < vecs[i].iov_len)
			break;

		size -= vecs[i].iov_len;
	}

	size_t vecOffset = size;
	size_t bytesLeft = numBytes - size;

	for (; fileVecIndex < fileVecCount; fileVecIndex++) {
		file_io_vec &fileVec = fileVecs[fileVecIndex];
		off_t fileOffset = fileVec.offset;
		off_t fileLeft = fileVec.length;

		fileLeft = min_c(fileVec.length, bytesLeft);

		TRACE(("FILE VEC [%lu] length %Ld\n", fileVecIndex, fileLeft));

		// process the complete fileVec
		while (fileLeft > 0) {
			iovec tempVecs[MAX_TEMP_IO_VECS];
			uint32 tempCount = 0;

			// size tracks how much of what is left of the current fileVec
			// (fileLeft) has been assigned to tempVecs 
			size = 0;

			// assign what is left of the current fileVec to the tempVecs
			while (size < fileLeft && i < count
					&& tempCount < MAX_TEMP_IO_VECS) {
				// try to satisfy one iovec per iteration (or as much as
				// possible)
				TRACE(("fill vec %ld, offset = %lu, size = %lu\n",
					i, vecOffset, size));

				// bytes left of the current iovec
				size_t vecLeft = vecs[i].iov_len - vecOffset;
				if (vecLeft == 0) {
					i++;
					vecOffset = 0;
					continue;
				}

				// actually available bytes
				size_t tempVecSize = min_c(vecLeft, fileLeft - size);

				tempVecs[tempCount].iov_base
					= (void *)((addr_t)vecs[i].iov_base + vecOffset);
				tempVecs[tempCount].iov_len = tempVecSize;
				tempCount++;

				size += tempVecSize;
				vecOffset += tempVecSize;
			}

			size_t bytes = size;
			if (doWrite) {
				status = vfs_write_pages(ref->deviceFD, fileOffset,
					tempVecs, tempCount, &bytes, false);
			} else {
				status = vfs_read_pages(ref->deviceFD, fileOffset,
					tempVecs, tempCount, &bytes, false);
			}
			if (status < B_OK)
				return status;

			totalSize += bytes;
			bytesLeft -= size;
			fileOffset += size;
			fileLeft -= size;
			//dprintf("-> file left = %Lu\n", fileLeft);

			if (size != bytes || i >= count) {
				// there are no more bytes or iovecs, let's bail out
				*_numBytes = totalSize;
				return B_OK;
			}
		}
	}

	*_numBytes = totalSize;
	return B_OK;
}


/*!
	This function is called by read_into_cache() (and from there only) - it
	can only handle a certain amount of bytes, and read_into_cache() makes
	sure that it matches that criterion.
*/
static inline status_t
read_chunk_into_cache(file_cache_ref *ref, off_t offset, size_t size,
	int32 pageOffset, addr_t buffer, size_t bufferSize)
{
	TRACE(("read_chunk(offset = %Ld, size = %lu, pageOffset = %ld, buffer = %#lx, bufferSize = %lu\n",
		offset, size, pageOffset, buffer, bufferSize));

	iovec vecs[MAX_IO_VECS];
	int32 vecCount = 0;

	vm_page *pages[MAX_IO_VECS];
	int32 pageIndex = 0;

	// allocate pages for the cache and mark them busy
	for (size_t pos = 0; pos < size; pos += B_PAGE_SIZE) {
		vm_page *page = pages[pageIndex++] = vm_page_allocate_page(PAGE_STATE_FREE);
		if (page == NULL)
			panic("no more pages!");

		page->state = PAGE_STATE_BUSY;

		vm_cache_insert_page(ref, page, offset + pos);

		addr_t virtualAddress = page->Address();

		add_to_iovec(vecs, vecCount, MAX_IO_VECS, virtualAddress, B_PAGE_SIZE);
		// ToDo: check if the array is large enough!
	}

	mutex_unlock(&ref->lock);

	// read file into reserved pages
	status_t status = pages_io(ref, offset, vecs, vecCount, &size, false);
	if (status < B_OK) {
		// reading failed, free allocated pages

		dprintf("file_cache: read pages failed: %s\n", strerror(status));

		mutex_lock(&ref->lock);

		for (int32 i = 0; i < pageIndex; i++) {
			vm_cache_remove_page(ref, pages[i]);
			vm_page_set_state(pages[i], PAGE_STATE_FREE);
			vm_page_put_page(pages[i]);
		}

		return status;
	}

	// copy the pages and unmap them again

	for (int32 i = 0; i < vecCount; i++) {
		addr_t base = (addr_t)vecs[i].iov_base;
		size_t size = vecs[i].iov_len;

		// copy to user buffer if necessary
		if (bufferSize != 0) {
			size_t bytes = min_c(bufferSize, size - pageOffset);

			user_memcpy((void *)buffer, (void *)(base + pageOffset), bytes);
			buffer += bytes;
			bufferSize -= bytes;
			pageOffset = 0;
		}
	}

	mutex_lock(&ref->lock);

	// make the pages accessible in the cache
	for (int32 i = pageIndex; i-- > 0;) {
		vm_page_set_state(pages[i], PAGE_STATE_ACTIVE);
		vm_page_put_page(pages[i]);
	}

	return B_OK;
}


/*!
	This function reads \a size bytes directly from the file into the cache.
	If \a bufferSize does not equal zero, \a bufferSize bytes from the data
	read in are also copied to the provided \a buffer.
	This function always allocates all pages; it is the responsibility of the
	calling function to only ask for yet uncached ranges.
	The cache_ref lock must be hold when calling this function.
*/
static status_t
read_into_cache(file_cache_ref *ref, off_t offset, size_t size, addr_t buffer, size_t bufferSize)
{
	TRACE(("read_from_cache: ref = %p, offset = %Ld, size = %lu, buffer = %p, bufferSize = %lu\n",
		ref, offset, size, (void *)buffer, bufferSize));

	// do we have to read in anything at all?
	if (size == 0)
		return B_OK;

	// make sure "offset" is page aligned - but also remember the page offset
	int32 pageOffset = offset & (B_PAGE_SIZE - 1);
	size = PAGE_ALIGN(size + pageOffset);
	offset -= pageOffset;

	while (true) {
		size_t chunkSize = size;
		if (chunkSize > (MAX_IO_VECS * B_PAGE_SIZE))
			chunkSize = MAX_IO_VECS * B_PAGE_SIZE;

		status_t status = read_chunk_into_cache(ref, offset, chunkSize, pageOffset,
								buffer, bufferSize);
		if (status != B_OK)
			return status;

		if ((size -= chunkSize) == 0)
			return B_OK;

		if (chunkSize >= bufferSize) {
			bufferSize = 0;
			buffer = NULL;
		} else {
			bufferSize -= chunkSize - pageOffset;
			buffer += chunkSize - pageOffset;
		}

		offset += chunkSize;
		pageOffset = 0;
	}

	return B_OK;
}


/**	Like read_chunk_into_cache() but writes data into the cache */

static inline status_t
write_chunk_to_cache(file_cache_ref *ref, off_t offset, size_t size,
	int32 pageOffset, addr_t buffer, size_t bufferSize)
{
	iovec vecs[MAX_IO_VECS];
	int32 vecCount = 0;
	vm_page *pages[MAX_IO_VECS];
	int32 pageIndex = 0;
	status_t status = B_OK;

	// ToDo: this should be settable somewhere
	bool writeThrough = false;

	// allocate pages for the cache and mark them busy
	for (size_t pos = 0; pos < size; pos += B_PAGE_SIZE) {
		// ToDo: if space is becoming tight, and this cache is already grown
		//	big - shouldn't we better steal the pages directly in that case?
		//	(a working set like approach for the file cache)
		vm_page *page = pages[pageIndex++] = vm_page_allocate_page(PAGE_STATE_FREE);
		page->state = PAGE_STATE_BUSY;

		vm_cache_insert_page(ref, page, offset + pos);

		addr_t virtualAddress = page->Address();

		add_to_iovec(vecs, vecCount, MAX_IO_VECS, virtualAddress, B_PAGE_SIZE);
		// ToDo: check if the array is large enough!
	}

	mutex_unlock(&ref->lock);

	// copy contents (and read in partially written pages first)

	if (pageOffset != 0) {
		// This is only a partial write, so we have to read the rest of the page
		// from the file to have consistent data in the cache
		iovec readVec = { vecs[0].iov_base, B_PAGE_SIZE };
		size_t bytesRead = B_PAGE_SIZE;

		status = pages_io(ref, offset, &readVec, 1, &bytesRead, false);
		// ToDo: handle errors for real!
		if (status < B_OK)
			panic("1. pages_io() failed: %s!\n", strerror(status));
	}

	addr_t lastPageOffset = (pageOffset + bufferSize) & (B_PAGE_SIZE - 1);
	if (lastPageOffset != 0) {
		// get the last page in the I/O vectors
		addr_t last = (addr_t)vecs[vecCount - 1].iov_base
			+ vecs[vecCount - 1].iov_len - B_PAGE_SIZE;

		if (offset + pageOffset + bufferSize == ref->virtual_size) {
			// the space in the page after this write action needs to be cleaned
			memset((void *)(last + lastPageOffset), 0, B_PAGE_SIZE - lastPageOffset);
		} else if (vecCount > 1) {
			// the end of this write does not happen on a page boundary, so we
			// need to fetch the last page before we can update it
			iovec readVec = { (void *)last, B_PAGE_SIZE };
			size_t bytesRead = B_PAGE_SIZE;

			status = pages_io(ref, offset + size - B_PAGE_SIZE, &readVec, 1,
				&bytesRead, false);
			// ToDo: handle errors for real!
			if (status < B_OK)
				panic("pages_io() failed: %s!\n", strerror(status));
		}
	}

	for (int32 i = 0; i < vecCount; i++) {
		addr_t base = (addr_t)vecs[i].iov_base;
		size_t bytes = min_c(bufferSize, size_t(vecs[i].iov_len - pageOffset));

		// copy data from user buffer
		user_memcpy((void *)(base + pageOffset), (void *)buffer, bytes);

		bufferSize -= bytes;
		if (bufferSize == 0)
			break;

		buffer += bytes;
		pageOffset = 0;
	}

	if (writeThrough) {
		// write cached pages back to the file if we were asked to do that
		status_t status = pages_io(ref, offset, vecs, vecCount, &size, true);
		if (status < B_OK) {
			// ToDo: remove allocated pages, ...?
			panic("file_cache: remove allocated pages! write pages failed: %s\n",
				strerror(status));
		}
	}

	mutex_lock(&ref->lock);

	// unmap the pages again

	// make the pages accessible in the cache
	for (int32 i = pageIndex; i-- > 0;) {
		if (writeThrough)
			vm_page_set_state(pages[i], PAGE_STATE_ACTIVE);
		else
			vm_page_set_state(pages[i], PAGE_STATE_MODIFIED);
		vm_page_put_page(pages[i]);
	}

	return status;
}


/**	Like read_into_cache() but writes data into the cache. To preserve data consistency,
 *	it might also read pages into the cache, though, if only a partial page gets written.
 *	The cache_ref lock must be hold when calling this function.
 */

static status_t
write_to_cache(file_cache_ref *ref, off_t offset, size_t size, addr_t buffer, size_t bufferSize)
{
	TRACE(("write_to_cache: ref = %p, offset = %Ld, size = %lu, buffer = %p, bufferSize = %lu\n",
		ref, offset, size, (void *)buffer, bufferSize));

	// make sure "offset" is page aligned - but also remember the page offset
	int32 pageOffset = offset & (B_PAGE_SIZE - 1);
	size = PAGE_ALIGN(size + pageOffset);
	offset -= pageOffset;

	while (true) {
		size_t chunkSize = size;
		if (chunkSize > (MAX_IO_VECS * B_PAGE_SIZE))
			chunkSize = MAX_IO_VECS * B_PAGE_SIZE;

		status_t status = write_chunk_to_cache(ref, offset, chunkSize, pageOffset, buffer, bufferSize);
		if (status != B_OK)
			return status;

		if ((size -= chunkSize) == 0)
			return B_OK;

		if (chunkSize >= bufferSize) {
			bufferSize = 0;
			buffer = NULL;
		} else {
			bufferSize -= chunkSize - pageOffset;
			buffer += chunkSize - pageOffset;
		}

		offset += chunkSize;
		pageOffset = 0;
	}

	return B_OK;
}


static status_t
satisfy_cache_io(file_cache_ref *ref, off_t offset, addr_t buffer, addr_t lastBuffer,
	bool doWrite)
{
	size_t requestSize = buffer - lastBuffer;

	if (doWrite)
		return write_to_cache(ref, offset, requestSize, lastBuffer, requestSize);

	return read_into_cache(ref, offset, requestSize, lastBuffer, requestSize);
}


static status_t
cache_io(void *_cacheRef, off_t offset, addr_t buffer, size_t *_size, bool doWrite)
{
	if (_cacheRef == NULL)
		panic("cache_io() called with NULL ref!\n");

	file_cache_ref *ref = (file_cache_ref *)_cacheRef;
	off_t fileSize = ref->virtual_size;

	TRACE(("cache_io(ref = %p, offset = %Ld, buffer = %p, size = %lu, %s)\n",
		ref, offset, (void *)buffer, *_size, doWrite ? "write" : "read"));

	// out of bounds access?
	if (offset >= fileSize || offset < 0) {
		*_size = 0;
		return B_OK;
	}

	int32 pageOffset = offset & (B_PAGE_SIZE - 1);
	size_t size = *_size;
	offset -= pageOffset;

	if (offset + pageOffset + size > fileSize) {
		// adapt size to be within the file's offsets
		size = fileSize - pageOffset - offset;
		*_size = size;
	}

	// "offset" and "lastOffset" are always aligned to B_PAGE_SIZE,
	// the "last*" variables always point to the end of the last
	// satisfied request part

	size_t bytesLeft = size, lastLeft = size;
	int32 lastPageOffset = pageOffset;
	addr_t lastBuffer = buffer;
	off_t lastOffset = offset;

	mutex_lock(&ref->lock);

	for (; bytesLeft > 0; offset += B_PAGE_SIZE) {
		// check if this page is already in memory
	restart:
		vm_page *page = vm_cache_lookup_page(ref, offset);
		PagePutter pagePutter(page);

		if (page != NULL) {
			// The page is busy - since we need to unlock the cache sometime
			// in the near future, we need to satisfy the request of the pages
			// we didn't get yet (to make sure no one else interferes in the
			// mean time).
			status_t status = B_OK;

			if (lastBuffer != buffer) {
				status = satisfy_cache_io(ref, lastOffset + lastPageOffset,
					buffer, lastBuffer, doWrite);
				if (status == B_OK) {
					lastBuffer = buffer;
					lastLeft = bytesLeft;
					lastOffset = offset;
					lastPageOffset = 0;
					pageOffset = 0;
				}
			}

			if (status != B_OK) {
				mutex_unlock(&ref->lock);
				return status;
			}

			if (page->state == PAGE_STATE_BUSY) {
				mutex_unlock(&ref->lock);
				// ToDo: don't wait forever!
				snooze(20000);
				mutex_lock(&ref->lock);
				goto restart;
			}
		}

		size_t bytesInPage = min_c(size_t(B_PAGE_SIZE - pageOffset), bytesLeft);
		addr_t virtualAddress;

		TRACE(("lookup page from offset %Ld: %p, size = %lu, pageOffset = %lu\n", offset, page, bytesLeft, pageOffset));
		if (page != NULL) {
			virtualAddress = page->Address();

			// and copy the contents of the page already in memory
			if (doWrite) {
				user_memcpy((void *)(virtualAddress + pageOffset), (void *)buffer, bytesInPage);

				// make sure the page is in the modified list
				if (page->state != PAGE_STATE_MODIFIED)
					vm_page_set_state(page, PAGE_STATE_MODIFIED);
			} else
				user_memcpy((void *)buffer, (void *)(virtualAddress + pageOffset), bytesInPage);

			if (bytesLeft <= bytesInPage) {
				// we've read the last page, so we're done!
				mutex_unlock(&ref->lock);
				return B_OK;
			}

			// prepare a potential gap request
			lastBuffer = buffer + bytesInPage;
			lastLeft = bytesLeft - bytesInPage;
			lastOffset = offset + B_PAGE_SIZE;
			lastPageOffset = 0;
		}

		if (bytesLeft <= bytesInPage)
			break;

		buffer += bytesInPage;
		bytesLeft -= bytesInPage;
		pageOffset = 0;
	}

	// fill the last remaining bytes of the request (either write or read)

	status_t status;
	if (doWrite)
		status = write_to_cache(ref, lastOffset + lastPageOffset, lastLeft, lastBuffer, lastLeft);
	else
		status = read_into_cache(ref, lastOffset + lastPageOffset, lastLeft, lastBuffer, lastLeft);

	mutex_unlock(&ref->lock);
	return status;
}


//	#pragma mark - public FS API


void *
file_cache_create(mount_id mountID, vnode_id vnodeID, off_t size, int fd)
{
	TRACE(("file_cache_create(mountID = %ld, vnodeID = %Ld, size = %Ld, fd = %d)\n", mountID, vnodeID, size, fd));

	file_cache_ref *ref = new(nothrow) file_cache_ref;
	if (ref == NULL)
		return NULL;

	ref->mountID = mountID;
	ref->nodeID = vnodeID;
	ref->nodeHandle = NULL;
	ref->deviceFD = fd;
	ref->virtual_size = size;

	// create lock
	char buffer[32];
	snprintf(buffer, sizeof(buffer), "file cache %ld:%lld", mountID, vnodeID);
	status_t error = mutex_init(&ref->lock, buffer);
	if (error != B_OK) {
		dprintf("file_cache_create(): Failed to init mutex: %s\n",
			strerror(error));
		delete ref;
		return NULL;
	}

	return ref;
}


void
file_cache_delete(void *_cacheRef)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	if (ref == NULL)
		return;

	TRACE(("file_cache_delete(ref = %p)\n", ref));

	// write all modified pages and resize the cache to 0 to free all pages
	// it contains
	vm_cache_write_modified(ref, false);

	mutex_lock(&ref->lock);
	vm_cache_resize(ref, 0);

	mutex_destroy(&ref->lock);

	delete ref;
}


status_t
file_cache_set_size(void *_cacheRef, off_t size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	TRACE(("file_cache_set_size(ref = %p, size = %Ld)\n", ref, size));

	if (ref == NULL)
		return B_OK;

	file_cache_invalidate_file_map(_cacheRef, 0, size);
		// ToDo: make this better (we would only need to extend or shrink the map)

	mutex_lock(&ref->lock);
	status_t status = vm_cache_resize(ref, size);
	mutex_unlock(&ref->lock);

	return status;
}


status_t
file_cache_sync(void *_cacheRef)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;
	if (ref == NULL)
		return B_BAD_VALUE;

	return vm_cache_write_modified(ref, true);
}


status_t
file_cache_read_pages(void *_cacheRef, off_t offset, const iovec *vecs, size_t count, size_t *_numBytes)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	return pages_io(ref, offset, vecs, count, _numBytes, false);
}


status_t
file_cache_write_pages(void *_cacheRef, off_t offset, const iovec *vecs, size_t count, size_t *_numBytes)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	status_t status = pages_io(ref, offset, vecs, count, _numBytes, true);
	TRACE(("file_cache_write_pages(ref = %p, offset = %Ld, vecs = %p, count = %lu, bytes = %lu) = %ld\n",
		ref, offset, vecs, count, *_numBytes, status));

	return status;
}


status_t
file_cache_read(void *_cacheRef, off_t offset, void *bufferBase, size_t *_size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	TRACE(("file_cache_read(ref = %p, offset = %Ld, buffer = %p, size = %lu)\n",
		ref, offset, bufferBase, *_size));

	return cache_io(ref, offset, (addr_t)bufferBase, _size, false);
}


status_t
file_cache_write(void *_cacheRef, off_t offset, const void *buffer, size_t *_size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	status_t status = cache_io(ref, offset, (addr_t)const_cast<void *>(buffer), _size, true);
	TRACE(("file_cache_write(ref = %p, offset = %Ld, buffer = %p, size = %lu) = %ld\n",
		ref, offset, buffer, *_size, status));

	return status;
}


status_t
file_cache_invalidate_file_map(void *_cacheRef, off_t offset, off_t size)
{
	file_cache_ref *ref = (file_cache_ref *)_cacheRef;

	// ToDo: honour offset/size parameters

	TRACE(("file_cache_invalidate_file_map(offset = %Ld, size = %Ld)\n", offset, size));
	mutex_lock(&ref->lock);
	ref->map.Free();
	mutex_unlock(&ref->lock);
	return B_OK;
}

}	// namespace HaikuKernelEmu
}	// namespace UserlandFS
