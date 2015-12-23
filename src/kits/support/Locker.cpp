/*
 * Copyright 2001-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Erik Jaesler, erik@cgsoftware.com
 */


/*!	Semaphore-type class for thread safety */


#include <OS.h>
#include <Locker.h>
#include <SupportDefs.h>

#include <stdio.h>

#include "support_kit_config.h"


// Data Member Documentation:
//
// The "fBenaphoreCount" member is set to 1 if the BLocker style is
// semaphore.  If the style is benaphore, it is initialized to 0 and
// is incremented atomically when it is acquired, decremented when it
// is released.  By setting the benaphore count to 1 when the style is
// semaphore, the benaphore effectively becomes a semaphore.  I was able
// to determine this is what Be's implementation does by testing the
// result of the CountLockRequests() member.
//
// The "fSemaphoreID" member holds the sem_id returned from create_sem()
// when the BLocker is constructed.  It is used to acquire and release
// the lock regardless of the lock style (semaphore or benaphore).
//
// The "fLockOwner" member holds the thread_id of the thread which
// currently holds the lock.  If no thread holds the lock, it is set to
// B_ERROR.
//
// The "fRecursiveCount" member holds a count of the number of times the
// thread holding the lock has acquired the lock without a matching unlock.
// It is basically the number of times the thread must call Unlock() before
// the lock can be acquired by a different thread.
//


BLocker::BLocker()
{
	InitLocker(NULL, true);
}


BLocker::BLocker(const char *name)
{
	InitLocker(name, true);
}


BLocker::BLocker(bool benaphoreStyle)
{
	InitLocker(NULL, benaphoreStyle);
}


BLocker::BLocker(const char *name, bool benaphoreStyle)
{
	InitLocker(name, benaphoreStyle);
}


/*!	This constructor is not documented.  The final argument is ignored for
	now.  In Be's headers, its called "for_IPC".  DO NOT USE THIS
	CONSTRUCTOR!
*/
BLocker::BLocker(const char *name, bool benaphoreStyle,
	bool)
{
	InitLocker(name, benaphoreStyle);
}


BLocker::~BLocker()
{
	delete_sem(fSemaphoreID);
}


status_t
BLocker::InitCheck() const
{
	return fSemaphoreID >= 0 ? B_OK : fSemaphoreID;
}


bool
BLocker::Lock()
{
	status_t result;
	return AcquireLock(B_INFINITE_TIMEOUT, &result);
}


status_t
BLocker::LockWithTimeout(bigtime_t timeout)
{
	status_t result;

	AcquireLock(timeout, &result);
	return result;
}


void
BLocker::Unlock()
{
	// The Be Book explicitly allows any thread, not just the lock owner, to
	// unlock. This is bad practice, but we must allow it for compatibility
	// reasons. We can at least warn the developer that something is probably
	// wrong.
	if (!IsLocked()) {
		fprintf(stderr, "Unlocking BLocker with sem %" B_PRId32
			" from wrong thread %" B_PRId32 ", current holder %" B_PRId32
			" (see issue #6400).\n", fSemaphoreID, find_thread(NULL),
			fLockOwner);
	}

	// Decrement the number of outstanding locks this thread holds
	// on this BLocker.
	fRecursiveCount--;

	// If the recursive count is now at 0, that means the BLocker has
	// been released by the thread.
	if (fRecursiveCount == 0) {
		// The BLocker is no longer owned by any thread.
		fLockOwner = B_ERROR;

		// Decrement the benaphore count and store the undecremented
		// value in oldBenaphoreCount.
		int32 oldBenaphoreCount = atomic_add(&fBenaphoreCount, -1);

		// If the oldBenaphoreCount is greater than 1, then there is
		// at least one thread waiting for the lock in the case of a
		// benaphore.
		if (oldBenaphoreCount > 1) {
			// Since there are threads waiting for the lock, it must
			// be released.  Note, the old benaphore count will always be
			// greater than 1 for a semaphore so the release is always done.
			release_sem(fSemaphoreID);
		}
	}
}


thread_id
BLocker::LockingThread() const
{
	return fLockOwner;
}


bool
BLocker::IsLocked() const
{
	// This member returns true if the calling thread holds the lock.
	// The easiest way to determine this is to compare the result of
	// find_thread() to the fLockOwner.
	return find_thread(NULL) == fLockOwner;
}


int32
BLocker::CountLocks() const
{
	return fRecursiveCount;
}


int32
BLocker::CountLockRequests() const
{
	return fBenaphoreCount;
}


sem_id
BLocker::Sem() const
{
	return fSemaphoreID;
}


void
BLocker::InitLocker(const char *name, bool benaphore)
{
	if (name == NULL)
		name = "some BLocker";

	if (benaphore && !BLOCKER_ALWAYS_SEMAPHORE_STYLE) {
		// Because this is a benaphore, initialize the benaphore count and
		// create the semaphore.  Because this is a benaphore, the semaphore
		// count starts at 0 (ie acquired).
		fBenaphoreCount = 0;
		fSemaphoreID = create_sem(0, name);
	} else {
		// Because this is a semaphore, initialize the benaphore count to -1
		// and create the semaphore.  Because this is semaphore style, the
		// semaphore count starts at 1 so that one thread can acquire it and
		// the next thread to acquire it will block.
		fBenaphoreCount = 1;
		fSemaphoreID = create_sem(1, name);
	}

	// The lock is currently not acquired so there is no owner.
	fLockOwner = B_ERROR;

	// The lock is currently not acquired so the recursive count is zero.
	fRecursiveCount = 0;
}


bool
BLocker::AcquireLock(bigtime_t timeout, status_t *error)
{
	// By default, return no error.
	status_t status = B_OK;

	// Only try to acquire the lock if the thread doesn't already own it.
	if (!IsLocked()) {
		// Increment the benaphore count and test to see if it was already
		// greater than 0. If it is greater than 0, then some thread already has
		// the benaphore or the style is a semaphore. Either way, we need to
		// acquire the semaphore in this case.
		int32 oldBenaphoreCount = atomic_add(&fBenaphoreCount, 1);
		if (oldBenaphoreCount > 0) {
			do {
				status = acquire_sem_etc(fSemaphoreID, 1, B_RELATIVE_TIMEOUT,
					timeout);
			} while (status == B_INTERRUPTED);

			// Note, if the lock here does time out, the benaphore count
			// is not decremented.  By doing this, the benaphore count will
			// never go back to zero.  This means that the locking essentially
			// changes to semaphore style if this was a benaphore.
			//
			// Doing the decrement of the benaphore count when the acquisition
			// fails is a risky thing to do.  If you decrement the counter at
			// the same time the thread which holds the benaphore does an
			// Unlock(), there is serious risk of a race condition.
			//
			// If the Unlock() sees a positive count and releases the semaphore
			// and then the timed out thread decrements the count to 0, there
			// is no one to take the semaphore.  The next two threads will be
			// able to acquire the benaphore at the same time!  The first will
			// increment the counter and acquire the lock.  The second will
			// acquire the semaphore and therefore the lock.  Not good.
			//
			// This has been discussed on the becodetalk mailing list and
			// Trey from Be had this to say:
			//
			// I looked at the LockWithTimeout() code, and it does not have
			// _this_ (ie the race condition) problem.  It circumvents it by
			// NOT doing the atomic_add(&count, -1) if the semaphore
			// acquisition fails.  This means that if a
			// BLocker::LockWithTimeout() times out, all other Lock*() attempts
			// turn into guaranteed semaphore grabs, _with_ the overhead of a
			// (now) useless atomic_add().
			//
			// Given Trey's comments, it looks like Be took the same approach
			// I did.  The output of CountLockRequests() of Be's implementation
			// confirms Trey's comments also.
			//
			// Finally some thoughts for the future with this code:
			//   - If 2^31 timeouts occur on a 32-bit machine (ie today),
			//     the benaphore count will wrap to a negative number.  This
			//     would have unknown consequences on the ability of the BLocker
			//     to continue to function.
			//
		}
	}

	// If the lock has successfully been acquired.
	if (status == B_OK) {
		// Set the lock owner to this thread and increment the recursive count
		// by one.  The recursive count is incremented because one more Unlock()
		// is now required to release the lock (ie, 0 => 1, 1 => 2 etc).
		if (fLockOwner < 0) {
			fLockOwner = find_thread(NULL);
			fRecursiveCount = 1;
		} else
			fRecursiveCount++;
	}

	if (error != NULL)
		*error = status;

	// Return true if the lock has been acquired.
	return (status == B_OK);
}
