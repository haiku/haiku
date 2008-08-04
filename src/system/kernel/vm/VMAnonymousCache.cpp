/*
 * Copyright 2008, Zhao Shuai, upczhsh@163.com.
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

#include "VMAnonymousCache.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <KernelExport.h>
#include <NodeMonitor.h>

#include <arch_config.h>
#include <driver_settings.h>
#include <fs_interface.h>
#include <heap.h>
#include <slab/Slab.h>
#include <syscalls.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>
#include <vfs.h>
#include <vm.h>
#include <vm_page.h>
#include <vm_priv.h>


#if	ENABLE_SWAP_SUPPORT

//#define TRACE_STORE
#ifdef TRACE_STORE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define SWAP_BLOCK_PAGES 32
#define SWAP_BLOCK_SHIFT 5		/* 1 << SWAP_BLOCK_SHIFT == SWAP_BLOCK_PAGES */
#define SWAP_BLOCK_MASK  (SWAP_BLOCK_PAGES - 1)

#define SWAP_PAGE_NONE (~(swap_addr_t)0)

// bitmap allocation macros
#define MAP_SHIFT  5
#define NUM_BITS_PER_WORD  32    // number of bits per word

#define TESTBIT(map, i) \
	(((map)[(i) >> MAP_SHIFT] & (1 << (i) % NUM_BITS_PER_WORD)))
#define SETBIT(map, i) \
	(((map)[(i) >> MAP_SHIFT] |= (1 << (i) % NUM_BITS_PER_WORD)))
#define CLEARBIT(map, i) \
	(((map)[(i) >> MAP_SHIFT] &= ~(1 << (i) % NUM_BITS_PER_WORD)))

// The stack functionality looks like a good candidate to put into its own
// store. I have not done this because once we have a swap file backing up
// the memory, it would probably not be a good idea to separate this
// anymore.

struct swap_file : DoublyLinkedListLinkImpl<swap_file> {
	struct vnode	*vnode;
	swap_addr_t		first_page;
	swap_addr_t		last_page;
	swap_addr_t		used;   // # of pages used
	uint32			*maps;  // bitmap for the pages
	swap_addr_t		hint;   // next free page
};

struct swap_hash_key {
	VMAnonymousCache	*cache;
	off_t				cache_offset;
};

// Each swap block contains SWAP_BLOCK_PAGES pages
struct swap_block : HashTableLink<swap_block> {
	swap_hash_key	key;
	uint32			used;
	swap_addr_t		swap_pages[SWAP_BLOCK_PAGES];
};

static swap_addr_t swap_page_alloc(uint32 npages);
static void swap_page_dealloc(swap_addr_t pageIndex, uint32 npages);
static swap_file *find_swap_file(swap_addr_t pageIndex);
static off_t swap_space_reserve(off_t amount);
static void swap_space_unreserve(off_t amount);

static object_cache *sSwapBlockCache;

struct SwapHashTableDefinition {
	typedef swap_hash_key KeyType;
	typedef swap_block ValueType;

	SwapHashTableDefinition() {}

	size_t HashKey(const swap_hash_key& key) const
	{
		off_t cacheOffset = key.cache_offset >> SWAP_BLOCK_SHIFT;
		VMAnonymousCache *cache = key.cache;
		return cacheOffset ^ (int)(int *)cache;
	}

	size_t Hash(const swap_block *value) const
	{
		return HashKey(value->key);
	}

	bool Compare(const swap_hash_key& key, const swap_block *value) const
	{
		return (key.cache_offset & ~(off_t)SWAP_BLOCK_MASK)
				== (value->key.cache_offset & ~(off_t)SWAP_BLOCK_MASK)
			&& key.cache == value->key.cache;
	}

	HashTableLink<swap_block> *GetLink(swap_block *value) const
	{
		return value;
	}
};

typedef OpenHashTable<SwapHashTableDefinition> SwapHashTable;
typedef DoublyLinkedList<swap_file> SwapFileList;

static SwapHashTable sSwapHashTable;
static mutex sSwapHashLock;

static SwapFileList sSwapFileList;
static mutex sSwapFileListLock;
static swap_file *sSwapFileAlloc = NULL; // allocate from here
static uint32 sSwapFileCount = 0;

static off_t sAvailSwapSpace = 0;
static mutex sAvailSwapSpaceLock;


VMAnonymousCache::~VMAnonymousCache()
{
	swap_space_unreserve(fCommittedSwapSize);
	vm_unreserve_memory(committed_size);
}


status_t
VMAnonymousCache::Init(bool canOvercommit, int32 numPrecommittedPages,
	int32 numGuardPages)
{
	TRACE(("VMAnonymousCache::Init(canOvercommit = %s, numGuardPages = %ld) "
		"at %p\n", canOvercommit ? "yes" : "no", numGuardPages, store));

	status_t error = VMCache::Init(CACHE_TYPE_RAM);
	if (error != B_OK)
		return error;

	fCanOvercommit = canOvercommit;
	fHasPrecommitted = false;
	fPrecommittedPages = min_c(numPrecommittedPages, 255);
	fGuardedSize = numGuardPages * B_PAGE_SIZE;
	fCommittedSwapSize = 0;
	fAllocatedSwapSize = 0;

	return B_OK;
}


status_t
VMAnonymousCache::Commit(off_t size)
{
	// if we can overcommit, we don't commit here, but in anonymous_fault()
	if (fCanOvercommit) {
		if (fHasPrecommitted)
			return B_OK;

		// pre-commit some pages to make a later failure less probable
		fHasPrecommitted = true;
		uint32 precommitted = fPrecommittedPages * B_PAGE_SIZE;
		if (size > precommitted)
			size = precommitted;
	}

	return _Commit(size);
}


bool
VMAnonymousCache::HasPage(off_t offset)
{
	offset >>= PAGE_SHIFT;
	swap_addr_t pageIndex = _SwapBlockGetAddress(offset);
	if (pageIndex != SWAP_PAGE_NONE)
		return true;
	return false;
}


status_t
VMAnonymousCache::Read(off_t offset, const iovec *vecs, size_t count,
	size_t *_numBytes)
{
	off_t cacheOffset = offset >> PAGE_SHIFT;

	for (uint32 i = 0, j = 0; i < count; i = j) {
		swap_addr_t startPageIndex = _SwapBlockGetAddress(cacheOffset + i);
		for (j = i + 1; j < count; j++) {
			swap_addr_t pageIndex = _SwapBlockGetAddress(cacheOffset + j);
			if (pageIndex != startPageIndex + j - i)
				break;
		}

		swap_file *swapFile = find_swap_file(startPageIndex);
		if (swapFile == NULL) {
			panic("can't find swap file for page index %ld\n", startPageIndex);
			return B_ERROR;
		}

		off_t pos = (startPageIndex - swapFile->first_page) * PAGE_SIZE;

		status_t status = vfs_read_pages(swapFile->vnode, NULL, pos, vecs + i,
				j - i, 0, _numBytes);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
VMAnonymousCache::Write(off_t offset, const iovec *vecs, size_t count,
	size_t *_numBytes)
{
	if (fAllocatedSwapSize + count * PAGE_SIZE > fCommittedSwapSize)
		return B_ERROR;

	offset >>= PAGE_SHIFT;
	uint32 n = count;
	for (uint32 i = 0; i < count; i += n) {
		swap_addr_t pageIndex;
		// try to allocate n pages, if fail, try to allocate n/2
		while ((pageIndex = swap_page_alloc(n)) == SWAP_PAGE_NONE
				&& n >= 2)
			n >>= 1;
		if (pageIndex == SWAP_PAGE_NONE)
			panic("can't allocate swap space\n");

		fAllocatedSwapSize += n * PAGE_SIZE;

		for (uint32 j = 0; j < n; j++)
			_SwapBlockBuild(offset + i + j, pageIndex + j);

		swap_file *swapFile = find_swap_file(pageIndex);
		if (swapFile == NULL) {
			panic("can't find swap file for page index %ld\n", pageIndex);
			return B_ERROR;
		}

		off_t pos = (pageIndex - swapFile->first_page) * PAGE_SIZE;
		status_t status = vfs_write_pages(swapFile->vnode, NULL, pos, vecs + i,
				n, 0, _numBytes);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
VMAnonymousCache::Fault(struct vm_address_space *aspace, off_t offset)
{
	if (fCanOvercommit) {
		if (fGuardedSize > 0) {
			uint32 guardOffset;

#ifdef STACK_GROWS_DOWNWARDS
			guardOffset = 0;
#elif defined(STACK_GROWS_UPWARDS)
			guardOffset = virtual_size - fGuardedSize;
#else
#	error Stack direction has not been defined in arch_config.h
#endif

			// report stack fault, guard page hit!
			if (offset >= guardOffset && offset < guardOffset + fGuardedSize) {
				TRACE(("stack overflow!\n"));
				return B_BAD_ADDRESS;
			}
		}

		if (fPrecommittedPages == 0) {
			// try to commit additional swap space/memory
			if (swap_space_reserve(PAGE_SIZE) == PAGE_SIZE)
				fCommittedSwapSize += PAGE_SIZE;
			else if (vm_try_reserve_memory(B_PAGE_SIZE, 0) != B_OK)
				return B_NO_MEMORY;

			committed_size += B_PAGE_SIZE;
		} else
			fPrecommittedPages--;
	}

	// This will cause vm_soft_fault() to handle the fault
	return B_BAD_HANDLER;
}


void
VMAnonymousCache::MergeStore(VMCache* _source)
{
	VMAnonymousCache* source = dynamic_cast<VMAnonymousCache*>(_source);
	if (source == NULL) {
		panic("VMAnonymousCache::MergeStore(): merge with incompatible cache "
			"%p requested", _source);
		return;
	}

	// take over the source' committed size
	fCommittedSwapSize += source->fCommittedSwapSize;
	source->fCommittedSwapSize = 0;
	committed_size += source->committed_size;
	source->committed_size = 0;

	off_t actualSize = virtual_end - virtual_base;
	if (committed_size > actualSize)
		_Commit(actualSize);

	// TODO: iterate over swap blocks
	for (off_t offset = source->virtual_base; offset < source->virtual_end;
			offset += PAGE_SIZE) {
		swap_addr_t sourceSwapIndex = source->_SwapBlockGetAddress(offset);

		if (sourceSwapIndex == SWAP_PAGE_NONE)
			// this page is not swapped out
			continue;

		if (LookupPage(offset))
			// this page is shadowed and we can find it in the new cache,
			// free the swap space
			swap_page_dealloc(sourceSwapIndex, 1);
		else {
			swap_addr_t swapIndex = _SwapBlockGetAddress(offset);

			if (swapIndex == SWAP_PAGE_NONE) {
				// the page is not shadowed,
				// assign the swap address to the new cache
				_SwapBlockBuild(offset, sourceSwapIndex);
			} else {
				// the page is shadowed and is also swapped out
				swap_page_dealloc(sourceSwapIndex, 1);
			}
		}
		source->_SwapBlockFree(offset);
	}
}


void
VMAnonymousCache::_SwapBlockBuild(off_t cacheOffset, swap_addr_t pageIndex)
{
	mutex_lock(&sSwapHashLock);

	swap_hash_key key = { this, cacheOffset };

	swap_block *swap = sSwapHashTable.Lookup(key);
	if (swap == NULL) {
		swap = (swap_block *)object_cache_alloc(sSwapBlockCache,
				CACHE_DONT_SLEEP);
		if (swap == NULL) {
			// TODO: wait until memory can be allocated
			mutex_unlock(&sSwapHashLock);
			return;
		}

		swap->key.cache = this;
		swap->key.cache_offset = cacheOffset & ~(off_t)SWAP_BLOCK_MASK;
		swap->used = 0;
		for (uint32 i = 0; i < SWAP_BLOCK_PAGES; i++)
			swap->swap_pages[i] = SWAP_PAGE_NONE;

		sSwapHashTable.Insert(swap);
	}

	swap_addr_t blockIndex = cacheOffset & SWAP_BLOCK_MASK;
	swap->swap_pages[blockIndex] = pageIndex;

	swap->used++;

	mutex_unlock(&sSwapHashLock);
}


void
VMAnonymousCache::_SwapBlockFree(off_t cacheOffset)
{
	mutex_lock(&sSwapHashLock);

	swap_hash_key key = { this, cacheOffset };

	swap_block *swap = sSwapHashTable.Lookup(key);
	if (swap != NULL) {
		swap_addr_t pageIndex = swap->swap_pages[cacheOffset & SWAP_BLOCK_MASK];
		if (pageIndex != SWAP_PAGE_NONE) {
			swap->swap_pages[cacheOffset & SWAP_BLOCK_MASK] = SWAP_PAGE_NONE;
			swap->used--;
			if (swap->used == 0) {
				sSwapHashTable.Remove(swap);
				object_cache_free(sSwapBlockCache, swap);
			}
		}
	}

	mutex_unlock(&sSwapHashLock);
}


swap_addr_t
VMAnonymousCache::_SwapBlockGetAddress(off_t cacheOffset)
{
	mutex_lock(&sSwapHashLock);

	swap_hash_key key = { this, cacheOffset };
	swap_block *swap = sSwapHashTable.Lookup(key);
	swap_addr_t pageIndex = SWAP_PAGE_NONE;

	if (swap != NULL) {
		swap_addr_t blockIndex = cacheOffset & SWAP_BLOCK_MASK;
		pageIndex = swap->swap_pages[blockIndex];
	}

	mutex_unlock(&sSwapHashLock);

	return pageIndex;
}


status_t
VMAnonymousCache::_Commit(off_t size)
{
	// Basic strategy: reserve swap space first, only when running out of swap
	// space, reserve real memory.

	off_t committedMemory = committed_size - fCommittedSwapSize;

	// Regardless of whether we're asked to grow or shrink the commitment,
	// we always try to reserve as much as possible of the final commitment
	// in the swap space.
	if (size > fCommittedSwapSize) {
		fCommittedSwapSize += swap_space_reserve(size - committed_size);
		committed_size = fCommittedSwapSize + committedMemory;
	}

	if (committed_size == size)
		return B_OK;

	if (committed_size > size) {
		// The commitment shrinks -- unreserve real memory first.
		off_t toUnreserve = committed_size - size;
		if (committedMemory > 0) {
			off_t unreserved = min_c(toUnreserve, committedMemory);
			vm_unreserve_memory(unreserved);
			committedMemory -= unreserved;
			committed_size -= unreserved;
			toUnreserve -= unreserved;
		}

		// Unreserve swap space.
		if (toUnreserve > 0) {
			swap_space_unreserve(toUnreserve);
			fCommittedSwapSize -= toUnreserve;
			committed_size -= toUnreserve;
		}

		return B_OK;
	}

	// The commitment grows -- we have already tried to reserve swap space at
	// the start of the method, so we try to reserve real memory, now.

	off_t toReserve = size - committed_size;
	if (vm_try_reserve_memory(toReserve, 1000000) != B_OK)
		return B_NO_MEMORY;

	committed_size = size;
	return B_OK;
}


static swap_file *
find_swap_file(swap_addr_t pageIndex)
{
	for (SwapFileList::Iterator it = sSwapFileList.GetIterator();
			swap_file *swapFile = it.Next();) {
		if (pageIndex >= swapFile->first_page
				&& pageIndex < swapFile->last_page)
			return swapFile;
	}

	return NULL;
}


static swap_addr_t
swap_page_alloc(uint32 npages)
{
	swap_addr_t hint = 0;
	swap_addr_t j;

	if (sSwapFileList.IsEmpty())
		return SWAP_PAGE_NONE;

	mutex_lock(&sSwapFileListLock);
	for (j = 0; j < sSwapFileCount; j++) {
		if (sSwapFileAlloc == NULL)
			sSwapFileAlloc = sSwapFileList.First();

		hint = sSwapFileAlloc->hint;
		swap_addr_t pageCount = sSwapFileAlloc->last_page
			- sSwapFileAlloc->first_page;

		swap_addr_t i = 0;
		while (i < npages && (hint + npages) < pageCount) {
			if (TESTBIT(sSwapFileAlloc->maps, hint + i)) {
				hint++;
				i = 0;
			} else
				i++;
		}

		if (i == npages)
			break;

		// this swap_file is full, find another
		sSwapFileAlloc = sSwapFileList.GetNext(sSwapFileAlloc);
	}

	if (j == sSwapFileCount) {
		panic("swap space exhausted\n");
		mutex_unlock(&sSwapFileListLock);
		return SWAP_PAGE_NONE;
	}

	swap_addr_t pageIndex = sSwapFileAlloc->first_page + hint;

	for (uint32 i = 0; i < npages; i++)
		SETBIT(sSwapFileAlloc->maps, hint + i);
	if (hint == sSwapFileAlloc->hint)
		sSwapFileAlloc->hint += npages;

	sSwapFileAlloc->used += npages;

	// if this swap file has used more than 90% percent of its pages
	// switch to another
    if (sSwapFileAlloc->used
			> 9 * (sSwapFileAlloc->last_page - sSwapFileAlloc->first_page) / 10)
		sSwapFileAlloc = sSwapFileList.GetNext(sSwapFileAlloc);

	mutex_unlock(&sSwapFileListLock);
	return pageIndex;
}


static void
swap_page_dealloc(swap_addr_t pageIndex, uint32 npages)
{
	mutex_lock(&sSwapFileListLock);
	swap_file *swapFile = find_swap_file(pageIndex);

	pageIndex -= swapFile->first_page;

	for (uint32 i = 0; i < npages; i++)
		CLEARBIT(swapFile->maps, pageIndex + i);

	if (swapFile->hint > pageIndex)
		swapFile->hint = pageIndex;

	swapFile->used -= npages;
	mutex_unlock(&sSwapFileListLock);
}


static off_t
swap_space_reserve(off_t amount)
{
	mutex_lock(&sAvailSwapSpaceLock);
	if (sAvailSwapSpace >= amount)
		sAvailSwapSpace -= amount;
	else {
		sAvailSwapSpace = 0;
		amount = sAvailSwapSpace;
	}
	mutex_unlock(&sAvailSwapSpaceLock);

	return amount;
}


static void
swap_space_unreserve(off_t amount)
{
	mutex_lock(&sAvailSwapSpaceLock);
	sAvailSwapSpace += amount;
	mutex_unlock(&sAvailSwapSpaceLock);
}


status_t
swap_file_add(char *path)
{
	vnode *node = NULL;
	status_t status = vfs_get_vnode_from_path(path, true, &node);
	if (status != B_OK)
		return status;

	swap_file *swap = (swap_file *)malloc(sizeof(swap_file));
	if (swap == NULL) {
		vfs_put_vnode(node);
		return B_NO_MEMORY;
	}

	swap->vnode = node;

	struct stat st;
	status = vfs_stat_vnode(node, &st);
	if (status != B_OK) {
		free(swap);
		vfs_put_vnode(node);
		return status;
    }

	if (!(S_ISREG(st.st_mode) || S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode))) {
		free(swap);
		vfs_put_vnode(node);
		return B_ERROR;
	}

	int32 pageCount = st.st_size >> PAGE_SHIFT;
	swap->used = 0;

	swap->maps = (uint32 *)
		malloc((pageCount + NUM_BITS_PER_WORD - 1) / NUM_BITS_PER_WORD);
	if (swap->maps == NULL) {
		free(swap);
		vfs_put_vnode(node);
		return B_NO_MEMORY;
	}
	memset(swap->maps, 0, pageCount % 8 + 1);
	swap->hint = 0;

	// set start page index and add this file to swap file list
	mutex_lock(&sSwapFileListLock);
	if (sSwapFileList.IsEmpty()) {
		swap->first_page = 0;
		swap->last_page = pageCount;
	}
	else {
		// leave one page gap between two swap files
		swap->first_page = sSwapFileList.Last()->last_page + 1;
		swap->last_page = swap->first_page + pageCount;
	}
	sSwapFileList.Add(swap);
	sSwapFileCount++;
	mutex_unlock(&sSwapFileListLock);

	mutex_lock(&sAvailSwapSpaceLock);
	sAvailSwapSpace += pageCount * PAGE_SIZE;
	mutex_unlock(&sAvailSwapSpaceLock);

	return B_OK;
}


status_t
swap_file_delete(char *path)
{
	vnode *node = NULL;
	status_t status = vfs_get_vnode_from_path(path, true, &node);
	if (status != B_OK) {
		vfs_put_vnode(node);
		return status;
	}

	mutex_lock(&sSwapFileListLock);

	swap_file *swapFile = NULL;
	for (SwapFileList::Iterator it = sSwapFileList.GetIterator();
			(swapFile = it.Next()) != NULL;) {
		if (swapFile->vnode == node)
			break;
	}

	vfs_put_vnode(node);

	if (swapFile == NULL)
		return B_ERROR;

	// if this file is currently used, we can't delete
	// TODO: mark this swap file deleting, and remove it after releasing
	// all the swap space
	if (swapFile->used > 0)
		return B_ERROR;

	sSwapFileList.Remove(swapFile);
	sSwapFileCount--;
	mutex_unlock(&sSwapFileListLock);

	mutex_lock(&sAvailSwapSpaceLock);
	sAvailSwapSpace -= (swapFile->last_page - swapFile->first_page) * PAGE_SIZE;
	mutex_unlock(&sAvailSwapSpaceLock);

	vfs_put_vnode(swapFile->vnode);
	free(swapFile->maps);
	free(swapFile);

	return B_OK;
}


void
swap_init(void)
{
	// create swap block cache
	sSwapBlockCache = create_object_cache("swapblock",
			sizeof(swap_block), sizeof(void*), NULL, NULL, NULL);
	if (sSwapBlockCache == NULL)
		panic("can't create object cache for swap blocks\n");

	// init swap hash table
	sSwapHashTable.Init();
	mutex_init(&sSwapHashLock, "swaphash");

	// init swap file list
	mutex_init(&sSwapFileListLock, "swaplist");
	sSwapFileAlloc = NULL;
	sSwapFileCount = 0;

	// init available swap space
	mutex_init(&sAvailSwapSpaceLock, "avail swap space");
	sAvailSwapSpace = 0;
}

void
swap_init_post_modules()
{
	off_t size = 0;

	void *settings = load_driver_settings("virtual_memory");
	if (settings != NULL) {
		if (!get_driver_boolean_parameter(settings, "vm", false, false))
			return;

		const char *string = get_driver_parameter(settings, "swap_size", NULL,
			NULL);
		size = string ? atoll(string) : 0;

		unload_driver_settings(settings);
	} else
		size = vm_page_num_pages() * B_PAGE_SIZE * 2;

	if (size < B_PAGE_SIZE)
		return;

	int fd = open("/var/swap", O_RDWR | O_CREAT | O_NOCACHE, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		dprintf("Can't open/create /var/swap: %s\n", strerror(errno));
		return;
	}

	struct stat stat;
	stat.st_size = size;
	if (_kern_write_stat(fd, NULL, false, &stat, sizeof(struct stat),
			B_STAT_SIZE | B_STAT_SIZE_INSECURE) != B_OK) {
		dprintf("Failed to resize /var/swap to %lld bytes: %s\n", size,
			strerror(errno));
	}

	close(fd);
		// TODO: if we don't keep the file open, O_NOCACHE is going to be
		// removed - we must not do this while we're using the swap file

	swap_file_add("/var/swap");
}

#endif	// ENABLE_SWAP_SUPPORT
