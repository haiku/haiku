//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		MessageQueue.cpp
//	Author(s):		unknown
//					
//	Description:	Queue for holding BMessages
//					
//------------------------------------------------------------------------------

#include <MessageQueue.h>
#include <Autolock.h>
#include <Message.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif


/*
 *  Method: BMessageQueue::BMessageQueue()
 *   Descr: This method is the only constructor for a BMessageQueue.  Once the
 *          constructor completes, the BMessageQueue is created with no BMessages
 *          in it.
 *
 */
BMessageQueue::BMessageQueue() :
	fTheQueue(NULL), fQueueTail(NULL), fMessageCount(0)
{
}


/*
 *  Method: BMessageQueue::~BMessageQueue()
 *   Descr: This is the desctructor for the BMessageQueue.  It iterates over
 *          any messages left on the queue and deletes them.
 *
 *		    The implementation is careful not to release the lock when the
 *          BMessageQueue is deconstructed.  If the lock is released, it is
 *          possible another thread will start an AddMessage() operation before
 *          the BLocker is deleted.  The safe thing to do is not to unlock the
 *          BLocker from the destructor once it is acquired. That way, any thread
 *          waiting to do a AddMessage() will fail to acquire the lock since the
 *          BLocker will be deleted before they can acquire it.
 *
 */
BMessageQueue::~BMessageQueue()
{
	if (fLocker.Lock()) {
		BMessage *theMessage = fTheQueue;
		while (theMessage != NULL) {
			BMessage *messageToDelete = theMessage;
			theMessage = theMessage->link;
			delete messageToDelete;
		}
	}
}


/*
 *  Method: BMessageQueue::AddMessage()
 *   Descr: This method adds a BMessage to the queue.  It makes a couple of
 *          assumptions:
 *             - The BMessage was allocated on the heap with new.  Since the
 *               destructor delete's BMessages left on the queue, this must be
 *               true.  The same assumption is made with Be's implementation.
 *             - The BMessage is not already on this or any other BMessageQueue.
 *               If it is, the queue it is already on will be corrupted.  Be's
 *               implementation makes this assumption also and does corrupt
 *               BMessageQueues where this is violated.
 *
 */
void
BMessageQueue::AddMessage(BMessage *message)
{
	if (message == NULL) {
		return;
	}
	
	// The Be implementation does not seem to check that the lock acquisition
	// was successful.  This will specifically cause problems when the
	// message queue is deleted.  On delete, any thread waiting for the lock
	// will be notified that the lock failed.  Be's implementation, because
	// they do not check proceeds with the operation, potentially corrupting
	// memory.  This implementation is different, but I can't imagine that
	// Be's implementation is worth emulating.
	//
	BAutolock theAutoLocker(fLocker);
	
	if (theAutoLocker.IsLocked()) {
		
		// The message passed in will be the last message on the queue so its
		// link member should be set to null.
		message->link = NULL;
		
		// We now have one more BMessage on the queue.
		fMessageCount++;
		
		// If there are no BMessages on the queue.
		if (fQueueTail == NULL) {
			// Then this message is both the start and the end of the queue.
			fTheQueue = message;
			fQueueTail = message;
		} else {
			// If there are already messages on the queue, then the put this
			// BMessage at the end.  The last BMessage prior to this AddMessage()
			// is fQueueTail.  The BMessage at fQueueTail needs to point to the
			// new last message, the one being added.
			fQueueTail->link = message;
			
			// Now update the fQueueTail to point to this new last message.
			fQueueTail = message;
		}
	}
}


/*
 *  Method: BMessageQueue::RemoveMessage()
 *   Descr: This method searches the queue for a particular BMessage.  If
 *          it is found, it is removed from the queue.
 * 
 */
void
BMessageQueue::RemoveMessage(BMessage *message)
{
	if (message == NULL) {
		return;
	}
	
	BAutolock theAutoLocker(fLocker);
	
	// The Be implementation does not seem to check that the lock acquisition
	// was successful.  This will specifically cause problems when the
	// message queue is deleted.  On delete, any thread waiting for the lock
	// will be notified that the lock failed.  Be's implementation, because
	// they do not check proceeds with the operation, potentially corrupting
	// memory.  This implementation is different, but I can't imagine that
	// Be's implementation is worth emulating.
	//
	if (theAutoLocker.IsLocked()) {
		
		// If the message to be removed is at the front of the queue.
		if (fTheQueue == message) {
			// We need to special case the handling of removing the first element.
			// First, the new front element will be the next one.
			fTheQueue = fTheQueue->link;
			
			// Must decrement the count of elements since the front one is being
			// removed.
			fMessageCount--;
			
			// If the new front element is NULL, then that means that the queue
			// is now empty.  That means that fQueueTail must be set to NULL.
			if (fTheQueue == NULL) {
				fQueueTail = NULL;
			}
			
			// We have found the message and removed it in this case.  We can
			// bail out now.  The autolocker will take care of releasing the
			// lock for us.
			return;
		}
		
		// The message to remove is not the first one, so we need to scan the
		// queue.  Get a message iterator and set it to the first element.
		BMessage *messageIter = fTheQueue;
		
		// While we are not at the end of the list.
		while (messageIter != NULL) {
			// If the next message after this (ie second, then third etc) is 
			// the one we are looking for.
			if (messageIter->link == message) {
				// At this point, this is what we have:
				//    messageIter - the BMessage in the queue just before the
				//                  match
				//    messageIter->link - the BMessage which matches message
				//    message - the same as messageIter->link
				//    message->link - the element after the match
				//
				// The next step is to link the BMessage just before the match
				// to the one just after the match.  This removes the match from
				// the queue.
				messageIter->link = message->link;
				
				// One less element on the queue.
				fMessageCount--;
				
				// If there is no BMessage after the match is the
				if (message->link == NULL) {
					// That means that we just removed the last element from the
					// queue.  The new last element then must be messageIter.
					fQueueTail = messageIter;
				}
				
				// We can return now because we have a match and removed it.
				return;
			}
			
			// No match yet, go to the next element in the list.
			messageIter = messageIter->link;
		}
	}
}


/*
 *  Method: BMessageQueue::CountMessages()
 *   Descr: This method just returns the number of BMessages on the queue.
 */
int32
BMessageQueue::CountMessages(void) const
{
    return fMessageCount;
}


/*
 *  Method: BMessageQueue::IsEmpty()
 *   Descr: This method just returns true if there are no BMessages on the queue.
 */
bool
BMessageQueue::IsEmpty(void) const
{
    return (fMessageCount == 0);
}


/*
 *  Method: BMessageQueue::FindMessage()
 *   Descr: This method searches the queue for the index'th BMessage.  The first
 *          BMessage is at index 0, the second at index 1 etc.  The BMessage
 *          is returned if it is found.  If no BMessage exists at that index
 *          (ie the queue is not that long or the index is invalid) NULL is
 *          returned.
 *
 *          This method does not lock the BMessageQueue so there is risk that
 *          the queue could change in the course of the search.  Be's
 *          implementation must do the same, unless they do some funky casting.
 *          The method is declared const which means it cannot modify the data
 *          members.  Because it cannot modify the data members, it cannot
 *          acquire a lock.  So unless they are casting away the const-ness
 *          of the this pointer, this member in Be's implementation does no
 *          locking either.
 */
BMessage *
BMessageQueue::FindMessage(int32 index) const
{
	// If the index is negative or larger than the number of messages on the
	// queue.
	if ((index < 0) || (index >= fMessageCount)) {
		// No match is possible, bail out now.
		return NULL;
	}
	
	// Get a message iterator and initialize it to the start of the queue.
	BMessage *messageIter = fTheQueue;
	
	// While this is not the end of the queue.
	while (messageIter != NULL) {
		// If the index reaches zero, then we have found a match.
		if (index == 0) {
			// Because this is a match, break out of the while loop so we can
			// return the message pointed to messageIter.		
			break;
		}
		
		// No match yet, decrement the index.  We will have a match once index
		// reaches zero.
		index--;
		// Increment the messageIter to the next BMessage on the queue.
		messageIter = messageIter->link;
	}
	
	// If no match was found, messageIter will be NULL since that is the only
	// way out of the loop.  If a match was found, the messageIter will point
	// to that match.
    return messageIter;
}


/*
 *  Method: BMessageQueue::FindMessage()
 *   Descr: This method searches the queue for the index'th BMessage that has a
 *          particular what code.  The first BMessage with that what value is at
 *          index 0, the second at index 1 etc.  The BMessage is returned if it
 *          is found.  If no matching BMessage exists at that index NULL is
 *          returned.
 *
 *          This method does not lock the BMessageQueue so there is risk that
 *          the queue could change in the course of the search.  Be's
 *          implementation must do the same, unless they do some funky casting.
 *          The method is declared const which means it cannot modify the data
 *          members.  Because it cannot modify the data members, it cannot
 *          acquire a lock.  So unless they are casting away the const-ness
 *          of the this pointer, this member in Be's implementation does no
 *          locking either.
 */
BMessage *
BMessageQueue::FindMessage(uint32 what,
                           int32 index) const
{
	// If the index is negative or larger than the number of messages on the
	// queue.
	if ((index < 0) || (index >= fMessageCount)) {
		// No match is possible, bail out now.
		return NULL;
	}
	
	// Get a message iterator and initialize it to the start of the queue.
	BMessage *messageIter = fTheQueue;
	
	// While this is not the end of the queue.
	while (messageIter != NULL) {
		// If the messageIter points to a BMessage with the what code we are
		// looking for.
		if (messageIter->what == what) {
			// If the index reaches zero, then we have found a match.
			if (index == 0) {
				// Because this is a match, break out of the while loop so we can
				// return the message pointed to messageIter.	
				break;
			}
			// No match yet, decrement the index.  We will have a match once index
			// reaches zero.
			index--;
		}
		// Increment the messageIter to the next BMessage on the queue.
		messageIter = messageIter->link;
	}
	
	// If no match was found, messageIter will be NULL since that is the only
	// way out of the loop.  If a match was found, the messageIter will point
	// to that match.
    return messageIter;
}


/*
 *  Method: BMessageQueue::Lock()
 *   Descr: This member just locks the BMessageQueue so no other thread can acquire
 *          the lock nor make changes to the queue through members like
 *          AddMessage(), RemoveMessage(), NextMessage() or ~BMessageQueue().
 */
bool
BMessageQueue::Lock(void)
{
    return fLocker.Lock();
}


/*
 *  Method: BMessageQueue::Unlock()
 *   Descr: This member releases the lock which was acquired by Lock().
 */
void
BMessageQueue::Unlock(void)
{
	fLocker.Unlock();
}


/*
 *  Method: BMessageQueue::IsLocked()
 *   Descr: This member returns whether or not the queue is locked
 */
bool 
BMessageQueue::IsLocked(void)
{
	return fLocker.IsLocked();
}


/*
 *  Method: BMessageQueue::NextMessage()
 *   Descr: This member removes the first BMessage on the queue and returns
 *          it to the caller.  If the queue is empty, NULL is returned.
 */
BMessage *
BMessageQueue::NextMessage(void)
{
	// By default, we will assume that no BMessage is on the queue.
	BMessage *result = NULL;
	BAutolock theAutoLocker(fLocker);
	
	// The Be implementation does not seem to check that the lock acquisition
	// was successful.  This will specifically cause problems when the
	// message queue is deleted.  On delete, any thread waiting for the lock
	// will be notified that the lock failed.  Be's implementation, because
	// they do not check proceeds with the operation, potentially corrupting
	// memory.  This implementation is different, but I can't imagine that
	// Be's implementation is worth emulating.
	//
	if (theAutoLocker.IsLocked()) {
		// Store the first BMessage in the queue in result.
		result = fTheQueue;
		
		// If the queue is not empty.
		if (fTheQueue != NULL) {
			// Decrement the message count since we are removing an element.
			fMessageCount--;
			// The new front of the list is moved forward thereby removing the
			// first element from the queue.
			fTheQueue = fTheQueue->link;
			// If the queue is empty after removing the front element.
			if (fTheQueue == NULL) {
				// We need to set the tail of the queue to NULL since the queue
				// is now empty.
				fQueueTail = NULL;
			}
		}
	}
    return result;
}


void 
BMessageQueue::_ReservedMessageQueue1(void)
{
}


void 
BMessageQueue::_ReservedMessageQueue2(void)
{
}


void 
BMessageQueue::_ReservedMessageQueue3(void)
{
}

#ifdef USE_OPENBEOS_NAMESPACE
}
#endif
