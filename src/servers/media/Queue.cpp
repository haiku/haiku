/* 
 * Copyright 2002, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
/* This is a simple multi thread save queue.
 *
 * One thread calls AddItem() to add items, and when it
 * is finished doing so, it calls Terminate(). Another
 * thread calls RemoveItem() to remove items from the
 * queue. RemoveItem() blocks when no items are available.
 * As soon as Terminate() is called and the queue is empty,
 * RemoveItem() returns NULL.
 */
 
#include <List.h>
#include <OS.h>
#include <Locker.h>

#include "Queue.h"

Queue::Queue()
 :	fList(new BList),
	fLocker(new BLocker("queue locker")),
	fSem(create_sem(0, "queue sem"))
{
}

Queue::~Queue()
{
	if (fSem > 0)
		delete_sem(fSem);
	delete fLocker;
	delete fList;
}

status_t
Queue::Terminate()
{
	status_t rv;

	fLocker->Lock();
	if (fSem < 0) {
		rv = B_ERROR;
	} else {
		delete_sem(fSem);
		fSem = -1;
		rv = B_OK;
	}
	fLocker->Unlock();
	return rv;
}
	
status_t
Queue::AddItem(void *item)
{
	status_t rv;

	fLocker->Lock();
	if (fSem < 0) {
		rv = B_ERROR;
	} else {
		if (fList->AddItem(item)) { // AddItem returns a bool
			release_sem(fSem);
			rv = B_OK;
		} else {
			rv = B_ERROR;
		}
	}
	fLocker->Unlock();
	return rv;
}

void *
Queue::RemoveItem()
{
	void *item;

	// if the semaphore is deleted by Terminate(),
	// this will no longer block
	while (acquire_sem(fSem) == B_INTERRUPTED)
		;
	
	// if the list is empty, which can only happen after
	// Terminate() was called, item will be NULL
	fLocker->Lock();
	item = fList->RemoveItem((int32)0);
	fLocker->Unlock();
	
	return item;	
}
