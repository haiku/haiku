///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// Hoard: A Fast, Scalable, and Memory-Efficient Allocator
//        for Shared-Memory Multiprocessors
// Contact author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2000, The University of Texas at Austin.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation, http://www.fsf.org.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
//////////////////////////////////////////////////////////////////////////////

#include "arch-specific.h"
#include "heap.h"

#include <OS.h>
#include <Debug.h>

#include <stdlib.h>
#include <unistd.h>

using namespace BPrivate;


struct free_chunk {
	free_chunk	*next;
	size_t		size;
};

typedef DoublyLinkedList<hoardLockType> LockList;


static const size_t kInitialHeapSize = 50 * B_PAGE_SIZE;
	// that's about what hoard allocates anyway

static area_id sHeapArea;
static hoardLockType sHeapLock;
static void *sHeapBase;
static addr_t sFreeHeapBase;
static size_t sFreeHeapSize, sHeapAreaSize;
static free_chunk *sFreeChunks;
static LockList sLockList;

static void initialize_hoard_lock(hoardLockType &lock, const char *name);
static void reinitialize_hoard_lock(hoardLockType &lock);


static void
init_after_fork()
{
	// re-initialize all locks
	for (LockList::Iterator it = sLockList.GetIterator(); it.HasNext();) {
		hoardLockType *lock = it.Next();
		reinitialize_hoard_lock(*lock);
	}

	// find the heap area
	sHeapArea = area_for(sHeapBase);
	if (sHeapArea < 0) {
		// Where is it gone?
		debug_printf("hoard: init_after_fork(): thread %ld, Heap area not "
			"found! Base address: %p\n", find_thread(NULL), sHeapBase);
		exit(1);
	}
}


extern "C" status_t
__init_heap(void)
{
	hoardHeap::initNumProcs();

	sHeapAreaSize = kInitialHeapSize;
	// ToDo: add a VM call that instructs other areas to avoid the space after the heap when possible
	//	(and if not, create it at the end of that range, so that the heap can grow as much as possible)
	//	Then, move the heap back to 256 or 512 MB
	sHeapBase = (void*)0x30000000;
		// let the heap start at 3*256 MB for now

	sHeapArea = create_area("heap", (void **)&sHeapBase, B_BASE_ADDRESS,
		sHeapAreaSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	sFreeHeapBase = (addr_t)sHeapBase;

	// init the lock list, and the heap lock
	// Thereafter all locks should be initialized with hoardLockInit(). They
	// will be properly re-initialized after a fork(). Note, that also the
	// heap lock is initialized with hoardLockInit() -- this works fine
	// and has the advantage, that it is in the lock list itself and we won't
	// need any special handling on fork().
	new (&sLockList) LockList;

	hoardLockInit(sHeapLock, "heap");

	if (sHeapArea < 0)
		return sHeapArea;

	atfork(&init_after_fork);
		// Note: Needs malloc(). Hence we need to be fully initialized.
		// ToDo: We should actually also install a hook that is called before
		// fork() is being executed. In a multithreaded app it would need to
		// acquire *all* allocator locks, so that we don't fork() an
		// inconsistent state.

	return B_OK;
}


static void
initialize_hoard_lock(hoardLockType &lock, const char *name)
{
	lock.ben = 0;
	lock.sem = create_sem(0, name);
	if (lock.sem < 0) {
		debug_printf("hoard: initialize_hoard_lock(): Failed to create "
			"semaphore");
	}
}


static void
reinitialize_hoard_lock(hoardLockType &lock)
{
	// Get an info for the original semaphore, so we can name it just the same.
	// This can fail e.g. in case the original team is already gone.
	sem_info info;
	if (get_sem_info(lock.sem, &info) == B_OK)
		initialize_hoard_lock(lock, info.name);
	else
		initialize_hoard_lock(lock, "reinitialized hoard lock");
}


namespace BPrivate {

void *
hoardSbrk(long size)
{
	assert(size > 0);

	// align size request
	size = (size + hoardHeap::ALIGNMENT - 1) & ~(hoardHeap::ALIGNMENT - 1);

	hoardLock(sHeapLock);

	// find chunk in free list
	free_chunk *chunk = sFreeChunks, *last = NULL;
	for (; chunk != NULL; chunk = chunk->next, last = chunk) {
		if (chunk->size < (size_t)size)
			continue;

		// this chunk is large enough to satisfy the request

		SERIAL_PRINT(("HEAP-%ld: found free chunk to hold %ld bytes\n",
			find_thread(NULL), size));

		void *address = (void *)chunk;

		if (chunk->size + sizeof(free_chunk) > (size_t)size)
			chunk = (free_chunk *)((addr_t)chunk + size);
		else
			chunk = chunk->next;
		
		if (last != NULL)
			last->next = chunk;
		else
			sFreeChunks = chunk;

		hoardUnlock(sHeapLock);

		return address;
	}

	// There was no chunk, let's see if the area is large enough

	size_t oldHeapSize = sFreeHeapSize;
	sFreeHeapSize += size;

	// round to next page size
	size_t pageSize = (sFreeHeapSize + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	if (pageSize < sHeapAreaSize) {
		SERIAL_PRINT(("HEAP-%ld: heap area large enough for %ld\n", find_thread(NULL), size));
		// the area is large enough already
		hoardUnlock(sHeapLock);
		return (void *)(sFreeHeapBase + oldHeapSize);
	}

	// We need to grow the area

	SERIAL_PRINT(("HEAP-%ld: need to resize heap area to %ld (%ld requested)\n",
		find_thread(NULL), pageSize, size));

	if (resize_area(sHeapArea, pageSize) < B_OK) {
		// out of memory - ToDo: as a fall back, we could try to allocate another area
		hoardUnlock(sHeapLock);
		return NULL;
	}

	sHeapAreaSize = pageSize;

	hoardUnlock(sHeapLock);

	return (void *)(sFreeHeapBase + oldHeapSize);
}


void
hoardUnsbrk(void *ptr, long size)
{
	// NOT CURRENTLY IMPLEMENTED!
}


void
hoardLockInit(hoardLockType &lock, const char *name)
{
	new (&lock) hoardLockType;
		// init's the list link

	initialize_hoard_lock(lock, name);

	// add the lock to the lock list (the heap lock also protects the lock list)
	hoardLock(sHeapLock);
	sLockList.Add(&lock);
	hoardUnlock(sHeapLock);
}


void
hoardLock(hoardLockType &lock)
{
	if (atomic_add(&(lock.ben), 1) >= 1)
		acquire_sem(lock.sem);
}


void
hoardUnlock(hoardLockType &lock)
{
	if (atomic_add(&(lock.ben), -1) > 1)
		release_sem(lock.sem);
}


void
hoardYield(void)
{
}

}	// namespace BPrivate
