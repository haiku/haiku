/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de
 * Distributed under the terms of the MIT License.
 */


#include <debug_heap.h>

#include <new>

#include <util/AutoLock.h>
#include <vm/vm.h>


#define INITIAL_DEBUG_HEAP_SIZE	B_PAGE_SIZE

static char sInitialHeap[INITIAL_DEBUG_HEAP_SIZE] __attribute__ ((aligned (8)));
static void* sHeapBase = sInitialHeap;
static size_t sHeapSize = INITIAL_DEBUG_HEAP_SIZE;

const kdebug_alloc_t kdebug_alloc = {};


struct allocation_header {
	uint32	size : 31;	// size in allocation_header units
	bool	free : 1;
	uint32	previous;
};

struct free_entry : allocation_header {
	uint32	previous_free;
	uint32	next_free;
};


struct DebugAllocPool {
	void Init(void* heap, size_t heapSize)
	{
		fParent = NULL;
		fChild = NULL;

		uint32 size = heapSize / 8;
		fBase = (allocation_header*)heap - 1;
		fEnd = size + 1;
		fFirstFree = 0;
		fLastFree = 0;

		// add free entry spanning the whole area
		fBase[1].size = size - 1;
		fBase[1].previous = 0;
		_InsertFreeEntry(1);
	}

	DebugAllocPool* CreateChildPool()
	{
		// do we already have a child pool?
		if (fChild != NULL)
			return NULL;

		// create the pool object
		DebugAllocPool* pool
			= (DebugAllocPool*)Allocate(sizeof(DebugAllocPool));
		if (pool == NULL)
			return NULL;

		// do we have enough free space?
		if (fLastFree == 0 || fBase[fLastFree].size < 2) {
			Free(pool);
			return NULL;
		}

		allocation_header* header = &fBase[fLastFree];
		_RemoveFreeEntry(fLastFree);

		pool->Init(header + 1, header->size * 8);
		pool->fParent = this;

		return fChild = pool;
	}

	void Destroy()
	{
		if (fParent != NULL) {
			fParent->fChild = NULL;
			fParent->Free(fBase + 1);
		}
	}

	DebugAllocPool* Parent() const
	{
		return fParent;
	}

	void* Allocate(size_t size)
	{
		size = (size + 7) / 8;
		uint32 index = fFirstFree;
		while (index != 0 && fBase[index].size < size)
			index = ((free_entry*)&fBase[index])->next_free;

		if (index == 0)
			return NULL;

		_RemoveFreeEntry(index);

		// if the entry is big enough, we split it
		if (fBase[index].size - size >= 2) {
			uint32 next = index + 1 + size;
			uint32 nextNext = index + 1 + fBase[index].size;
			fBase[next].size = fBase[index].size - size - 1;
			fBase[next].previous = index;
			fBase[index].size = size;
			_InsertFreeEntry(next);

			if (nextNext < fEnd)
				fBase[nextNext].previous = next;
		}

		return &fBase[index + 1];
	}

	void Free(void* address)
	{
		// check address
		if (((addr_t)address & 7) != 0 || address <= fBase + 1
			|| address >= fBase + fEnd) {
			kprintf("DebugAllocPool::Free(%p): bad address\n", address);
			return;
		}

		// get header
		allocation_header* header = (allocation_header*)address - 1;
		uint32 index = header - fBase;
		if (header->free) {
			kprintf("DebugAllocPool::Free(%p): double free\n", address);
			return;
		}

		uint32 next = index + 1 + header->size;

		// join with previous, if possible
		if (index > 1 && fBase[header->previous].free) {
			uint32 previous = header->previous;
			_RemoveFreeEntry(previous);

			fBase[previous].size += 1 + header->size;
			fBase[next].previous = previous;

			index = previous;
			header = fBase + index;
		}

		// join with next, if possible
		if (next < fEnd && fBase[next].free) {
			_RemoveFreeEntry(next);

			header->size += 1 + fBase[next].size;

			uint32 nextNext = index + 1 + header->size;
			if (nextNext < fEnd)
				fBase[nextNext].previous = index;
		}

		_InsertFreeEntry(index);
	}

private:
	void _InsertFreeEntry(uint32 index)
	{
		// find the insertion point -- list is sorted by ascending size
		uint32 size = fBase[index].size;
		uint32 next = fFirstFree;
		while (next != 0 && size > fBase[next].size)
			next = ((free_entry*)&fBase[next])->next_free;

		// insert
		uint32 previous;
		if (next != 0) {
			previous = ((free_entry*)&fBase[next])->previous_free;
			((free_entry*)&fBase[next])->previous_free = index;
		} else {
			previous = fLastFree;
			fLastFree = index;
		}

		if (previous != 0)
			((free_entry*)&fBase[previous])->next_free = index;
		else
			fFirstFree = index;

		((free_entry*)&fBase[index])->previous_free = previous;
		((free_entry*)&fBase[index])->next_free = next;

		fBase[index].free = true;
	}

	void _RemoveFreeEntry(uint32 index)
	{
		uint32 previous = ((free_entry*)&fBase[index])->previous_free;
		uint32 next = ((free_entry*)&fBase[index])->next_free;

		if (previous != 0)
			((free_entry*)&fBase[previous])->next_free = next;
		else
			fFirstFree = next;

		if (next != 0)
			((free_entry*)&fBase[next])->previous_free = previous;
		else
			fLastFree = previous;

		fBase[index].free = false;
	}

private:
	DebugAllocPool*		fParent;
	DebugAllocPool*		fChild;
	allocation_header*	fBase;		// actually base - 1, so that index 0 is
									// invalid
	uint32				fEnd;
	uint32				fFirstFree;
	uint32				fLastFree;
};


static DebugAllocPool* sCurrentPool;
static DebugAllocPool sInitialPool;


debug_alloc_pool*
create_debug_alloc_pool()
{
	if (sCurrentPool == NULL) {
		sInitialPool.Init(sHeapBase, sHeapSize);
		sCurrentPool = &sInitialPool;
		return sCurrentPool;
	}

	DebugAllocPool* pool = sCurrentPool->CreateChildPool();
	if (pool == NULL)
		return NULL;

	sCurrentPool = pool;
	return sCurrentPool;
}


void
delete_debug_alloc_pool(debug_alloc_pool* pool)
{
	if (pool == NULL || sCurrentPool == NULL)
		return;

	// find the pool in the hierarchy
	DebugAllocPool* otherPool = sCurrentPool;
	while (otherPool != NULL && otherPool != pool)
		otherPool = otherPool->Parent();

	if (otherPool == NULL)
		return;

	// destroy the pool
	sCurrentPool = pool->Parent();
	pool->Destroy();

	if (pool != &sInitialPool)
		debug_free(pool);
}


void*
debug_malloc(size_t size)
{
	if (sCurrentPool == NULL)
		return NULL;

	return sCurrentPool->Allocate(size);
}


void*
debug_calloc(size_t num, size_t size)
{
	size_t allocationSize = num * size;
	void* allocation = debug_malloc(allocationSize);
	if (allocation == NULL)
		return NULL;

	memset(allocation, 0, allocationSize);
	return allocation;
}


void
debug_free(void* address)
{
	if (address != NULL && sCurrentPool != NULL)
		sCurrentPool->Free(address);
}


void
debug_heap_init()
{
	// create the heap area
	void* base;
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	area_id area = create_area_etc(B_SYSTEM_TEAM, "kdebug heap", KDEBUG_HEAP,
		B_FULL_LOCK, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		CREATE_AREA_DONT_WAIT, &virtualRestrictions, &physicalRestrictions,
		(void**)&base);
	if (area < 0)
		return;

	// switch from the small static buffer to the area
	InterruptsLocker locker;
	sHeapBase = base;
	sHeapSize = KDEBUG_HEAP;
}
