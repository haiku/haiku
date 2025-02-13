/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */

#include "PagesAllocator.h"

#include <cstdio>
#include <errno.h>
#include <new>
#include <sys/mman.h>

#include <locks.h>
#include <syscalls.h>
#include <util/SplayTree.h>


/*! The local page size. Must be a multiple of the system page size. */
static const size_t kPageSize = B_PAGE_SIZE;

/*! The "largest useful" chunk size: any allocations larger than this
 * will get their own area, rather than sharing the common one(s). */
static const size_t kLargestUsefulChunk = 512 * kPageSize;

/*! Amount of virtual address space to reserve when creating new areas. */
static const size_t kReserveAddressSpace = 128 * 1024 * 1024;

/*! Cache up to this many percentage points of free memory (compared to used.) */
static const size_t kFreePercentage = 25;

/*! Always allow this much free memory, even if it's above kFreePercentage. */
static const size_t kFreeMinimum = 128 * kPageSize;


namespace {

class PagesAllocator {
	struct FreeChunk;

public:
	PagesAllocator()
	{
		mutex_init(&fLock, "PagesAllocator lock");
		fUsed = fFree = 0;
		fLastArea = -1;
	}

	~PagesAllocator()
	{
	}

	void BeforeFork()
	{
		mutex_lock(&fLock);
	}

	void AfterFork(bool parent)
	{
		if (parent) {
			mutex_unlock(&fLock);
		} else {
			if (fLastArea >= 0)
				fLastArea = area_for((void*)(fLastAreaTop - 1));

			mutex_init(&fLock, "PagesAllocator lock");
		}
	}

	status_t AllocatePages(void*& address, size_t allocate)
	{
		MutexLocker locker(fLock);

		if (allocate > kLargestUsefulChunk) {
			// Create an area just for this allocation.
			locker.Unlock();
			area_id area = create_area("heap large allocation",
				&address, B_ANY_ADDRESS, allocate,
				B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
			if (area >= 0) {
				locker.Lock();
				fUsed += allocate;
				return B_OK;
			}
			return area;
		}

		if (fFree >= allocate) {
			// Try to use memory from the cache.
			FreeChunk* chunk = fChunksBySizeTree.FindClosest(allocate, true, true);
			if (chunk != NULL) {
				address = _Use(chunk, allocate);
				return B_OK;
			}
		}

		// Not enough memory in the cache. Allocate some more.
		FreeChunk* chunk;
		status_t status = _Map(allocate, chunk);
		if (status != B_OK)
			return status;

		address = _Use(chunk, allocate);
		return B_OK;
	}

	status_t AllocatePagesAt(void* _address, size_t allocate)
	{
		const addr_t address = (addr_t)_address;
		MutexLocker locker(fLock);

		if (allocate <= kLargestUsefulChunk) {
			FreeChunk* chunk = fChunksByAddressTree.FindClosest(address, false, true);
			if (chunk != NULL && chunk->NextAddress() > address) {
				// The address is in a free chunk.
				size_t remainingAfter = chunk->size - (address - (addr_t)chunk);
				if (remainingAfter < allocate) {
					if (chunk->NextAddress() != fLastAreaTop)
						return B_NO_MEMORY;

					size_t add = allocate - remainingAfter;
					status_t status = _ResizeLastArea(add);
					if (status != B_OK)
						return status;

					chunk = _Insert((void*)chunk->NextAddress(), add);
				}

				// Cut the beginning?
				if (address == (addr_t)chunk) {
					_Use(chunk, allocate);
					return B_OK;
				}

				// Why would anyone want to allocate such a specific address, anyway?
				debugger("PagesAllocator: middle-chunk allocation not implemented!");
				return B_ERROR;
			}
		}

		if (address == fLastAreaTop) {
			if (allocate > kLargestUsefulChunk)
				return B_NO_MEMORY;

			status_t status = _ResizeLastArea(allocate);
			if (status == B_OK) {
				fUsed += allocate;
				return B_OK;
			}
			return status;
		}
		if (address >= (fLastAreaTop - fLastAreaSize) && address < fLastAreaTop) {
			// We didn't find a matching free chunk, so that isn't going to work.
			return EEXIST;
		}

		locker.Unlock();

		// One last try: see if we can resize the area this belongs to.
		area_info info;
		info.area = area_for((void*)(address - 1));
		if (info.area < 0)
			return info.area;

		status_t status = get_area_info(info.area, &info);
		if (status != B_OK)
			return status;

		if (((addr_t)info.address + info.size) != address)
			return B_NO_MEMORY;

		status = resize_area(info.area, info.size + allocate);
		if (status == B_OK) {
			locker.Lock();
			fUsed += allocate;
			return B_OK;
		}

		// TODO: We could add a "resize allowing move" feature to resize_area,
		// which would avoid having to memcpy() the data of large allocations.
		return status;
	}

	status_t FreePages(void* _address, size_t size)
	{
		MutexLocker locker(fLock);

		if (size > fUsed)
			debugger("PagesAllocator: request to free more than allocated");
		fUsed -= size;

		FreeChunk* chunk = _Insert(_address, size);

		if (size > kLargestUsefulChunk) {
			// TODO: This doesn't deal with a free of a smaller number of pages
			// within a "large allocation" area. We should probably free those
			// immediately also, or at least mark them specially in the trees
			// (so they don't get reused for anything but the large allocation.)

			locker.Detach();
			return _UnmapLocked(chunk);
		}

		if (fFree <= _FreeLimit()) {
			// TODO: If not decommitting/unmapping, we might free the pages
			// using MADV_FREE (perhaps on medium-sized chunks with others
			// of equal sizes already present in the tree, at least.)
			return B_OK;
		}

		while (fFree > _FreeLimit()) {
			FreeChunk* chunk = fChunksBySizeTree.FindMax();
			status_t status = _UnmapLocked(chunk);
			if (status != B_OK)
				return status;
			mutex_lock(&fLock);
		}

		if (fFree > kLargestUsefulChunk && fFree > (_FreeLimit() / 2)
				&& fChunksBySizeTree.FindMax()->size == kPageSize) {
			// All the free chunks are single pages, and there's more of them
			// than a "largest useful" chunk. Just evict them all at this point.
			while (FreeChunk* chunk = fChunksBySizeTree.FindMin()) {
				if (chunk->size != kPageSize)
					break;

				status_t status = _UnmapLocked(chunk);
				if (status != B_OK)
					return status;
				mutex_lock(&fLock);
			}
		}

		return B_OK;
	}

private:
	void* _Use(FreeChunk* chunk, size_t amount)
	{
		fChunksBySizeTree.Remove(chunk);
		fChunksByAddressTree.Remove(chunk);

		if (chunk->size == amount) {
			// The whole chunk will be used.
			// Nothing special to do in this case.
		} else {
			// Some will be left over.
			// Break the remainder off and reinsert into the trees.
			FreeChunk* newChunk = (FreeChunk*)((addr_t)chunk + amount);
			newChunk->size = (chunk->size - amount);

			fChunksBySizeTree.Insert(newChunk);
			fChunksByAddressTree.Insert(newChunk);
		}

		fUsed += amount;
		fFree -= amount;
		return (void*)chunk;
	}

	FreeChunk* _Insert(void* _address, size_t size)
	{
		fFree += size;

		const addr_t address = (addr_t)_address;
		FreeChunk* chunk;

		FreeChunk* preceding = fChunksByAddressTree.FindClosest(address, false, false);
		if (preceding != NULL && preceding->NextAddress() == address) {
			fChunksBySizeTree.Remove(preceding);
			chunk = preceding;
			chunk->size += size;
		} else {
			chunk = (FreeChunk*)_address;
			chunk->size = size;
			fChunksByAddressTree.Insert(chunk);
		}

		FreeChunk* following = chunk->address_tree_list_link;
		if (following != NULL && chunk->NextAddress() == (addr_t)following) {
			fChunksBySizeTree.Remove(following);
			fChunksByAddressTree.Remove(following);
			chunk->size += following->size;
		}
		fChunksBySizeTree.Insert(chunk);

		return chunk;
	}

	size_t _FreeLimit() const
	{
		size_t freeLimit = ((fUsed * kFreePercentage) / 100);
		if (freeLimit < kFreeMinimum)
			freeLimit = kFreeMinimum;
		else
			freeLimit = (freeLimit + (kPageSize - 1)) & ~(kPageSize - 1);
		return freeLimit;
	}

private:
	status_t _Map(size_t allocate, FreeChunk*& allocated)
	{
		if (fLastArea >= 0) {
			addr_t oldTop = fLastAreaTop;
			status_t status = _ResizeLastArea(allocate);
			if (status == B_OK) {
				allocated = _Insert((void*)oldTop, allocate);
				return B_OK;
			}
		}

		// Create a new area.
		// TODO: We could use an inner lock here to avoid contention.
		addr_t newAreaBase;
		status_t status = _kern_reserve_address_range(&newAreaBase,
			B_RANDOMIZED_ANY_ADDRESS, kReserveAddressSpace);
		size_t newReservation = (status == B_OK) ? kReserveAddressSpace : 0;

		status = create_area("heap area", (void**)&newAreaBase,
			(status == B_OK) ? B_EXACT_ADDRESS : B_RANDOMIZED_ANY_ADDRESS,
			allocate, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (status < B_OK)
			return status;

		if (fLastAreaReservedTop > fLastAreaTop)
			_kern_unreserve_address_range(fLastAreaTop, fLastAreaReservedTop - fLastAreaTop);

		fLastArea = status;
		fLastAreaTop = newAreaBase + allocate;
		fLastAreaSize = allocate;
		fLastAreaReservedTop = newAreaBase + newReservation;

		allocated = _Insert((void*)newAreaBase, allocate);
		return B_OK;
	}

	status_t _ResizeLastArea(size_t amount)
	{
		// TODO: We could use an inner lock here to avoid contention.
		status_t status = resize_area(fLastArea, fLastAreaSize + amount);
		if (status == B_OK) {
			fLastAreaTop += amount;
			fLastAreaSize += amount;
		}
		return status;
	}

	status_t _UnmapLocked(FreeChunk* chunk)
	{
		// TODO: We could use an inner lock here to avoid contention.
		MutexLocker locker(fLock, true);

		fChunksByAddressTree.Remove(chunk);
		fChunksBySizeTree.Remove(chunk);
		fFree -= chunk->size;

		const size_t size = chunk->size;
		const addr_t address = (addr_t)chunk;
		const addr_t top = address + size;

		const addr_t lastAreaBase = (fLastAreaTop - fLastAreaSize);
		if (address <= lastAreaBase && top >= fLastAreaTop) {
			// The whole area is being deleted.
			const addr_t lastTop = fLastAreaTop, reservedTop = fLastAreaReservedTop;
			fLastArea = -1;
			fLastAreaTop = fLastAreaSize = fLastAreaReservedTop = 0;
			locker.Unlock();

			if (reservedTop > lastTop)
				_kern_unreserve_address_range(lastTop, reservedTop - lastTop);
		} else if (top == fLastAreaTop) {
			// Shrink the top.
			status_t status = resize_area(fLastArea, fLastAreaSize - size);
			if (status != B_OK)
				return status;

			fLastAreaSize -= size;
			fLastAreaTop -= size;
			return B_OK;
		} else if (address == lastAreaBase) {
			// Shrink the bottom.
			if (munmap(chunk, size) != 0)
				return errno;
			fLastAreaSize -= size;
			return B_OK;
		} else if (address >= lastAreaBase && address < fLastAreaTop) {
			// Cut the middle and get the new ID.
			if (munmap(chunk, size) != 0)
				return errno;

			fLastAreaSize = fLastAreaTop - top;
			fLastArea = area_for((void*)(fLastAreaTop - 1));
			return B_OK;
		} else {
			// Not in the last area.
			locker.Unlock();
		}

		if (munmap(chunk, size) != 0)
			return errno;
		return B_OK;
	}

private:
	struct FreeChunk {
		SplayTreeLink<FreeChunk> address_tree_link;
		SplayTreeLink<FreeChunk> size_tree_link;
		FreeChunk* address_tree_list_link;
		FreeChunk* size_tree_list_link;

		size_t size;

	public:
		inline addr_t NextAddress() const { return ((addr_t)this + size); }
	};

	struct ChunksByAddressTreeDefinition {
		typedef addr_t		KeyType;
		typedef FreeChunk	NodeType;

		static addr_t GetKey(const FreeChunk* node)
		{
			return (addr_t)node;
		}

		static SplayTreeLink<FreeChunk>* GetLink(FreeChunk* node)
		{
			return &node->address_tree_link;
		}

		static int Compare(const addr_t& key, const FreeChunk* node)
		{
			if (key == (addr_t)node)
				return 0;
			return (key < (addr_t)node) ? -1 : 1;
		}

		static FreeChunk** GetListLink(FreeChunk* node)
		{
			return &node->address_tree_list_link;
		}
	};
	typedef IteratableSplayTree<ChunksByAddressTreeDefinition> ChunksByAddressTree;

	struct ChunksBySizeTreeDefinition {
		struct KeyType {
			size_t size;
			addr_t address;

		public:
			KeyType(size_t _size) : size(_size), address(0) {}
			KeyType(const FreeChunk* chunk) : size(chunk->size), address((addr_t)chunk) {}
		};
		typedef FreeChunk	NodeType;

		static KeyType GetKey(const FreeChunk* node)
		{
			return KeyType(node);
		}

		static SplayTreeLink<FreeChunk>* GetLink(FreeChunk* node)
		{
			return &node->size_tree_link;
		}

		static int Compare(const KeyType& key, const FreeChunk* node)
		{
			if (key.size == node->size)
				return ChunksByAddressTreeDefinition::Compare(key.address, node);
			return (key.size < node->size) ? -1 : 1;
		}

		static FreeChunk** GetListLink(FreeChunk* node)
		{
			return &node->size_tree_list_link;
		}
	};
	typedef IteratableSplayTree<ChunksBySizeTreeDefinition> ChunksBySizeTree;

private:
	mutex	fLock;

	size_t	fUsed;
	size_t	fFree;

	ChunksByAddressTree	fChunksByAddressTree;
	ChunksBySizeTree	fChunksBySizeTree;

	area_id		fLastArea;
	addr_t		fLastAreaTop;
	size_t		fLastAreaSize;
	size_t		fLastAreaReservedTop;
};

} // namespace


static char sPagesAllocatorStorage[sizeof(PagesAllocator)]
#if defined(__GNUC__) && __GNUC__ >= 4
	__attribute__((__aligned__(alignof(PagesAllocator))))
#endif
	;
static PagesAllocator* sPagesAllocator;


void
__init_pages_allocator()
{
	sPagesAllocator = new(sPagesAllocatorStorage) PagesAllocator;
}


void
__pages_allocator_before_fork()
{
	sPagesAllocator->BeforeFork();
}


void
__pages_allocator_after_fork(int parent)
{
	sPagesAllocator->AfterFork(parent);
}


status_t
__allocate_pages(void** address, size_t length, int flags)
{
	if ((length % kPageSize) != 0)
		debugger("PagesAllocator: incorrectly sized allocate");

	if ((flags & MAP_FIXED) != 0)
		return sPagesAllocator->AllocatePagesAt(*address, length);

	//fprintf(stderr, "AllocatePages! 0x%x\n", (int)length);
	return sPagesAllocator->AllocatePages(*address, length);
}


status_t
__free_pages(void* address, size_t length)
{
	if ((length % kPageSize) != 0)
		debugger("PagesAllocator: incorrectly sized free");

	//fprintf(stderr, "FreePages! %p, 0x%x\n", address, (int)length);
	return sPagesAllocator->FreePages(address, length);
}
