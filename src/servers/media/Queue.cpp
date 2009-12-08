/*
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


/*!	This is a simple multi thread save queue.

	One thread calls AddItem() to add items, and when it is finished doing
	so, it calls Terminate(). Another thread calls RemoveItem() to remove
	items from the queue. RemoveItem() blocks when no items are available.
	As soon as Terminate() is called and the queue is empty, RemoveItem()
	returns NULL.
*/


#include "Queue.h"

#include <Autolock.h>
#include <OS.h>


Queue::Queue()
	:
	BLocker("queue locker"),
	fSem(create_sem(0, "queue sem"))
{
}


Queue::~Queue()
{
	if (fSem >= 0)
		delete_sem(fSem);
}


status_t
Queue::Terminate()
{
	BAutolock _(this);

	if (fSem < 0)
		return B_ERROR;

	delete_sem(fSem);
	fSem = -1;
	return B_OK;
}


status_t
Queue::AddItem(void* item)
{
	BAutolock _(this);

	if (fSem < 0)
		return B_ERROR;

	if (!fList.AddItem(item))
		return B_NO_MEMORY;

	release_sem(fSem);
	return B_OK;
}


void*
Queue::RemoveItem()
{
	// if the semaphore is deleted by Terminate(),
	// this will no longer block
	while (acquire_sem(fSem) == B_INTERRUPTED)
		;

	BAutolock _(this);

	// if the list is empty, which can only happen after
	// Terminate() was called, item will be NULL
	return fList.RemoveItem((int32)0);
}
