/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef LOCK_H
#define LOCK_H

/*!	Simple semaphores, read/write lock implementation
	Roughly based on a Be sample code written by Nathan Schrenk.
*/

#include "system_dependencies.h"

#include "Utility.h"
#include "Debug.h"


// Configure here if and when real benaphores should be used
//#define USE_BENAPHORE
	// if defined, benaphores are used for the Semaphore/RecursiveLock classes
//#	define FAST_LOCK
	// the ReadWriteLock class uses a second Semaphore to
	// speed up locking - only makes sense if USE_BENAPHORE
	// is defined, too.
#ifdef FAST_LOCK
#	error implement recursive write locking first
#endif

//	#pragma mark - Many Reader/Single Writer Lock

// This is a "fast" implementation of a single writer/many reader
// locking scheme. It's fast because it uses the benaphore idea
// to do lazy semaphore locking - in most cases it will only have
// to do some simple integer arithmetic.
// The second semaphore (fWriteLock) is needed to prevent the situation
// that a second writer can acquire the lock when there are still readers
// holding it.

#define MAX_READERS 100000

// Note: this code will break if you actually have 100000 readers
// at once. With the current thread/... limits in BeOS you can't
// touch that value, but it might be possible in the future.
// Also, you can only have about 20000 concurrent writers until
// the semaphore count exceeds the int32 bounds

// Timeouts:
// It may be a good idea to have timeouts for the WriteLocked class,
// in case something went wrong - we'll see if this is necessary,
// but it would be a somewhat poor work-around for a deadlock...
// But the only real problem with timeouts could be for things like
// "chkbfs" - because such a tool may need to lock for some more time


// define if you want to have fast locks as the foundation for the
// ReadWriteLock class - the benefit is that acquire_sem() doesn't
// have to be called when there is no one waiting.
// The disadvantage is the use of 2 real semaphores which is quite
// expensive regarding that BeOS only allows for a total of 64k
// semaphores (since every open BFS inode has such a lock).

#ifdef FAST_LOCK
class ReadWriteLock {
	public:
		ReadWriteLock(const char *name)
			:
			fWriteLock(name)
		{
			Initialize(name);
		}

		ReadWriteLock()
			:
			fWriteLock("bfs r/w w-lock")
		{
		}

		~ReadWriteLock()
		{
			delete_sem(fSemaphore);
		}

		status_t Initialize(const char *name = "bfs r/w lock")
		{
			fSemaphore = create_sem(0, name);
			fCount = MAX_READERS;
			return fSemaphore;
		}

		status_t InitCheck()
		{
			if (fSemaphore < B_OK)
				return fSemaphore;

			return B_OK;
		}

		status_t Lock()
		{
			if (atomic_add(&fCount, -1) <= 0)
				return acquire_sem(fSemaphore);

			return B_OK;
		}

		void Unlock()
		{
			if (atomic_add(&fCount, 1) < 0)
				release_sem(fSemaphore);
		}

		status_t LockWrite()
		{
			if (fWriteLock.Lock() < B_OK)
				return B_ERROR;

			int32 readers = atomic_add(&fCount, -MAX_READERS);
			status_t status = B_OK;

			if (readers < MAX_READERS) {
				// Acquire sem for all readers currently not using a semaphore.
				// But if we are not the only write lock in the queue, just get
				// the one for us
				status = acquire_sem_etc(fSemaphore, readers <= 0 ? 1 : MAX_READERS - readers, 0, 0);
			}
			fWriteLock.Unlock();

			return status;
		}

		void UnlockWrite()
		{
			int32 readers = atomic_add(&fCount, MAX_READERS);
			if (readers < 0) {
				// release sem for all readers only when we were the only writer
				release_sem_etc(fSemaphore, readers <= -MAX_READERS ? 1 : -readers, 0);
			}
		}

	private:
		friend class ReadLocked;
		friend class WriteLocked;

		sem_id		fSemaphore;
		vint32		fCount;
		Semaphore	fWriteLock;
};
#else	// FAST_LOCK
class ReadWriteLock {
	public:
		ReadWriteLock(const char *name)
			:
			fOwner(-1)
		{
			Initialize(name);
		}

		ReadWriteLock()
			:
			fSemaphore(-1),
			fOwner(-1)
		{
		}

		~ReadWriteLock()
		{
			delete_sem(fSemaphore);
		}

		status_t Initialize(const char *name = "bfs r/w lock")
		{
			fSemaphore = create_sem(MAX_READERS, name);
			return fSemaphore;
		}

		status_t InitCheck()
		{
			if (fSemaphore < B_OK)
				return fSemaphore;

			return B_OK;
		}

		status_t Lock()
		{
			// This allows nested locking when holding a write lock
			thread_id currentThread = find_thread(NULL);
			if (currentThread == fOwner) {
				fOwnerCount++;
				return B_OK;
			}
			return acquire_sem(fSemaphore);
		}

		status_t TryLock()
		{
			// This allows nested locking when holding a write lock
			thread_id currentThread = find_thread(NULL);
			if (currentThread == fOwner) {
				fOwnerCount++;
				return B_OK;
			}
			return acquire_sem_etc(fSemaphore, 1, B_RELATIVE_TIMEOUT, 0);
		}

		void Unlock()
		{
			thread_id currentThread = find_thread(NULL);
			if (fOwner == currentThread && --fOwnerCount > 0)
				return;

			release_sem(fSemaphore);
		}

		status_t LockWrite()
		{
			thread_id currentThread = find_thread(NULL);
			if (currentThread == fOwner) {
				fOwnerCount++;
				return B_OK;
			}
			status_t status = acquire_sem_etc(fSemaphore, MAX_READERS, 0, 0);
			if (status >= B_OK) {
				fOwner = currentThread;
				fOwnerCount = 1;
			}
			return status;
		}

		void UnlockWrite()
		{
			if (--fOwnerCount == 0) {
				fOwner = -1;
				release_sem_etc(fSemaphore, MAX_READERS, 0);
			}
		}

	private:
		friend class ReadLocked;
		friend class WriteLocked;

		sem_id		fSemaphore;
		thread_id	fOwner;
		int32		fOwnerCount;
};
#endif	// FAST_LOCK


class ReadLocked {
	public:
		ReadLocked(ReadWriteLock &lock)
			:
			fLock(lock)
		{
			fStatus = lock.Lock();
		}

		~ReadLocked()
		{
			if (fStatus == B_OK)
				fLock.Unlock();
		}

		status_t
		IsLocked()
		{
			return fStatus;
		}

		void
		Unlock()
		{
			fLock.Unlock();
			fStatus = B_NO_INIT;
		}

	private:
		ReadWriteLock	&fLock;
		status_t		fStatus;
};


class WriteLocked {
	public:
		WriteLocked(ReadWriteLock &lock)
			:
			fLock(&lock)
		{
			fStatus = lock.LockWrite();
		}

		WriteLocked(ReadWriteLock *lock)
			:
			fLock(lock)
		{
			fStatus = lock != NULL ? lock->LockWrite() : B_NO_INIT;
		}

		~WriteLocked()
		{
			if (fStatus == B_OK)
				fLock->UnlockWrite();
		}

		status_t
		Lock()
		{
			if (fStatus == B_OK)
				return B_ERROR;
			return fStatus = fLock->LockWrite();
		}

		status_t
		IsLocked()
		{
			return fStatus;
		}

		void
		Unlock()
		{
			if (fStatus == B_OK)
				fLock->UnlockWrite();
			fStatus = B_ERROR;
		}

	private:
		ReadWriteLock	*fLock;
		status_t		fStatus;
};

#endif	/* LOCK_H */
