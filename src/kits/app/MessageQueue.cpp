/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Unknown? Eric?
 *		Axel Dörfler, axeld@pinc-software.de
 */

/**	Queue for holding BMessages */


#include <MessageQueue.h>
#include <Autolock.h>
#include <Message.h>


BMessageQueue::BMessageQueue()
	:
	fHead(NULL),
 	fTail(NULL),
 	fMessageCount(0),
 	fLock("BMessageQueue Lock")
{
}


/*!
	\brief This is the desctructor for the BMessageQueue.  It iterates over
		any messages left on the queue and deletes them.

	The implementation is careful not to release the lock when the
	BMessageQueue is deconstructed.  If the lock is released, it is
	possible another thread will start an AddMessage() operation before
	the BLocker is deleted.  The safe thing to do is not to unlock the
	BLocker from the destructor once it is acquired. That way, any thread
	waiting to do a AddMessage() will fail to acquire the lock since the
	BLocker will be deleted before they can acquire it.
*/
BMessageQueue::~BMessageQueue()
{
	if (!Lock())
		return;

	BMessage* message = fHead;
	while (message != NULL) {
		BMessage *next = message->fQueueLink;

		delete message;
		message = next;
	}
}


/*!
	\brief This method adds a BMessage to the queue.

	It makes a couple of assumptions:
		- The BMessage was allocated on the heap with new, since the
		  destructor delete's BMessages left on the queue.
		- The BMessage is not already on this or any other BMessageQueue.
		  If it is, the queue it is already on will be corrupted.
*/
void
BMessageQueue::AddMessage(BMessage* message)
{
	if (message == NULL)
		return;

	BAutolock _(fLock);
	if (!IsLocked())
		return;

	// The message passed in will be the last message on the queue so its
	// link member should be set to null.
	message->fQueueLink = NULL;

	fMessageCount++;

	if (fTail == NULL) {
		// there are no messages in the queue yet
		fHead = fTail = message;
	} else {
		// just add it after the tail
		fTail->fQueueLink = message;
		fTail = message;
	}
}


/*!
	\brief This method searches the queue for a particular BMessage.
 		If it is found, it is removed from the queue.
*/
void
BMessageQueue::RemoveMessage(BMessage* message)
{
	if (message == NULL)
		return;

	BAutolock _(fLock);
	if (!IsLocked())
		return;

	BMessage* last = NULL;
	for (BMessage* entry = fHead; entry != NULL; entry = entry->fQueueLink) {
		if (entry == message) {
			// remove this one
			if (entry == fHead)
				fHead = entry->fQueueLink;
			else
				last->fQueueLink = entry->fQueueLink;

			if (entry == fTail)
				fTail = last;

			fMessageCount--;
			return;
		}
		last = entry;
	}
}


/*!
	\brief This method just returns the number of BMessages on the queue.
*/
int32
BMessageQueue::CountMessages() const
{
    return fMessageCount;
}


/*!
	\brief This method just returns true if there are no BMessages on the queue.
*/
bool
BMessageQueue::IsEmpty() const
{
    return fMessageCount == 0;
}


/*!
	\brief This method searches the queue for the index'th BMessage.
	
	The first BMessage is at index 0, the second at index 1 etc.
	The BMessage is returned if it is found.  If no BMessage exists at that
	index (ie the queue is not that long or the index is invalid) NULL is
	returned.
*/
BMessage *
BMessageQueue::FindMessage(int32 index) const
{
	BAutolock _(fLock);
	if (!IsLocked())
		return NULL;

	if (index < 0 || index >= fMessageCount)
		return NULL;
	
	for (BMessage* message = fHead; message != NULL; message = message->fQueueLink) {
		// If the index reaches zero, then we have found a match.
		if (index == 0)
			return message;

		index--;
	}

    return NULL;
}


/*!
	\brief Searches the queue for the index'th BMessage that has a
		particular what code.
*/
BMessage *
BMessageQueue::FindMessage(uint32 what, int32 index) const
{
	BAutolock _(fLock);
	if (!IsLocked())
		return NULL;

	if (index < 0 || index >= fMessageCount)
		return NULL;

	for (BMessage* message = fHead; message != NULL; message = message->fQueueLink) {
		if (message->what == what) {
			// If the index reaches zero, then we have found a match.
			if (index == 0)
				return message;

			index--;
		}
	}

    return NULL;
}


/*!
	\brief Locks the BMessageQueue so no other thread can change
		or search the queue.
*/
bool
BMessageQueue::Lock()
{
    return fLock.Lock();
}


/*!
	\brief Releases the lock which was acquired by Lock().
*/
void
BMessageQueue::Unlock()
{
	fLock.Unlock();
}


/*!
	\brief Returns whether or not the queue is locked
*/
bool
BMessageQueue::IsLocked() const
{
	return fLock.IsLocked();
}


/*!
	\brief Removes the first BMessage on the queue and returns
		it to the caller.  If the queue is empty, NULL is returned.
*/
BMessage *
BMessageQueue::NextMessage()
{
	BAutolock _(fLock);
	if (!IsLocked())
		return NULL;

	// remove the head of the queue, if any, and return it

	BMessage* head = fHead;
	if (head == NULL)
		return NULL;

	fMessageCount--;
	fHead = head->fQueueLink;

	if (fHead == NULL) {
		// If the queue is empty after removing the front element,
		// we need to set the tail of the queue to NULL since the queue
		// is now empty.
		fTail = NULL;
	}

    return head;
}


/*!
	\brief Checks wether or not the specified \a message is the message
		you would get when calling NextMessage().
*/
bool
BMessageQueue::IsNextMessage(const BMessage* message) const
{
	BAutolock _(fLock);
	return fHead == message;
}


/*!
	\brief This method is only here for R5 binary compatibility!
		It should be dropped as soon as possible (it misses the const qualifier).
*/
bool
BMessageQueue::IsLocked()
{
	return fLock.IsLocked();
}


void BMessageQueue::_ReservedMessageQueue1() {}
void BMessageQueue::_ReservedMessageQueue2() {}
void BMessageQueue::_ReservedMessageQueue3() {}

