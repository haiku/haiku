/*
 * Copyright 2008-2010, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>
#include <string.h>

#include <heap.h>
#include <util/AutoLock.h>
#include <util/SinglyLinkedList.h>


struct DeferredFreeListEntry : SinglyLinkedListLinkImpl<DeferredFreeListEntry> {
};

typedef SinglyLinkedList<DeferredFreeListEntry> DeferredFreeList;
typedef SinglyLinkedList<DeferredDeletable> DeferredDeletableList;

static DeferredFreeList sDeferredFreeList;
static DeferredDeletableList sDeferredDeletableList;
static spinlock sDeferredFreeListLock;


void *
calloc(size_t numElements, size_t size)
{
	if (size != 0 && numElements > (((size_t)(-1)) / size))
		return NULL;

	void *address = malloc(numElements * size);
	if (address != NULL)
		memset(address, 0, numElements * size);

	return address;
}


void *
aligned_alloc(size_t alignment, size_t size)
{
	if ((size % alignment) != 0)
		return NULL;

	return memalign(alignment, size);
}


//	#pragma mark -


static void
deferred_deleter(void *arg, int iteration)
{
	// move entries and deletables to on-stack lists
	InterruptsSpinLocker locker(sDeferredFreeListLock);
	if (sDeferredFreeList.IsEmpty() && sDeferredDeletableList.IsEmpty())
		return;

	DeferredFreeList entries;
	entries.TakeFrom(&sDeferredFreeList);

	DeferredDeletableList deletables;
	deletables.TakeFrom(&sDeferredDeletableList);

	locker.Unlock();

	// free the entries
	while (DeferredFreeListEntry* entry = entries.RemoveHead())
		free(entry);

	// delete the deletables
	while (DeferredDeletable* deletable = deletables.RemoveHead())
		delete deletable;
}


void
deferred_free(void *block)
{
	if (block == NULL)
		return;

	DeferredFreeListEntry *entry = new(block) DeferredFreeListEntry;

	InterruptsSpinLocker _(sDeferredFreeListLock);
	sDeferredFreeList.Add(entry);
}


DeferredDeletable::~DeferredDeletable()
{
}


void
deferred_delete(DeferredDeletable *deletable)
{
	if (deletable == NULL)
		return;

	InterruptsSpinLocker _(sDeferredFreeListLock);
	sDeferredDeletableList.Add(deletable);
}


void
deferred_free_init()
{
	// run the deferred deleter roughly once a second
	if (register_kernel_daemon(deferred_deleter, NULL, 10) != B_OK)
		panic("failed to init deferred deleter");
}
