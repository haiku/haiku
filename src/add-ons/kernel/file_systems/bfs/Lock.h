#ifndef LOCK_H
#define LOCK_H
/* Lock - benaphores, read/write lock implementation
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** Roughly based on a Be sample code written by Nathan Schrenk.
**
** This file may be used under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>


class Benaphore {
	public:
		Benaphore(const char *name = "bfs benaphore")
			:
			fSemaphore(create_sem(0, name)),
			fCount(1)
		{
		}

		~Benaphore()
		{
			delete_sem(fSemaphore);
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

	private:
		sem_id	fSemaphore;
		vint32	fCount;
};

// a convenience class to lock the benaphore

class Locker {
	public:
		Locker(Benaphore &lock)
			: fLock(lock)
		{
			fStatus = lock.Lock();
		}
		
		~Locker()
		{
			if (fStatus == B_OK)
				fLock.Unlock();
		}
	
	private:
		Benaphore	&fLock;
		status_t	fStatus;
};


//**** Many Reader/Single Writer Lock

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
// semaphores.

//#define FAST_LOCK
#ifdef FAST_LOCK
class ReadWriteLock {
	public:
		ReadWriteLock(const char *name = "bfs r/w lock")
			:
			fSemaphore(create_sem(0, name)),
			fCount(MAX_READERS),
			fWriteLock()
		{
		}

		~ReadWriteLock()
		{
			delete_sem(fSemaphore);
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
				status = acquire_sem_etc(fSemaphore,readers <= 0 ? 1 : MAX_READERS - readers,0,0);
			}
			fWriteLock.Unlock();

			return status;
		}
		
		void UnlockWrite()
		{
			int32 readers = atomic_add(&fCount,MAX_READERS);
			if (readers < 0) {
				// release sem for all readers only when we were the only writer
				release_sem_etc(fSemaphore,readers <= -MAX_READERS ? 1 : -readers,0);
			}
		}

	private:
		friend class ReadLocked;
		friend class WriteLocked;

		sem_id		fSemaphore;
		vint32		fCount;
		Benaphore	fWriteLock;
};
#else	// FAST_LOCK
class ReadWriteLock {
	public:
		ReadWriteLock(const char *name = "bfs r/w lock")
			:
			fSemaphore(create_sem(MAX_READERS, name))
		{
		}

		~ReadWriteLock()
		{
			delete_sem(fSemaphore);
		}

		status_t InitCheck()
		{
			if (fSemaphore < B_OK)
				return fSemaphore;
			
			return B_OK;
		}

		status_t Lock()
		{
			return acquire_sem(fSemaphore);
		}
		
		void Unlock()
		{
			release_sem(fSemaphore);
		}
		
		status_t LockWrite()
		{
			return acquire_sem_etc(fSemaphore,MAX_READERS,0,0);
		}
		
		void UnlockWrite()
		{
			release_sem_etc(fSemaphore,MAX_READERS,0);
		}

	private:
		friend class ReadLocked;
		friend class WriteLocked;

		sem_id		fSemaphore;
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
	
	private:
		ReadWriteLock	&fLock;
		status_t		fStatus;
};


class WriteLocked {
	public:
		WriteLocked(ReadWriteLock &lock)
			:
			fLock(lock)
		{
			fStatus = lock.LockWrite();
		}

		~WriteLocked()
		{
			if (fStatus == B_OK)
				fLock.UnlockWrite();
		}

		status_t IsLocked()
		{
			return fStatus;
		}

	private:
		ReadWriteLock	&fLock;
		status_t		fStatus;
};


// A simple locking structure that doesn't use a semaphore - it's useful
// if you have to protect critical parts with a short runtime.

class SimpleLock {
	public:
		SimpleLock()
			:
			fLock(0),
			fUnlock(0)
		{
		}

		status_t Lock(bigtime_t time = 500)
		{
			int32 turn = atomic_add(&fLock,1);
			while (turn != fUnlock)
				snooze(time);

			// ToDo: the lock cannot fail currently! We may want
			// to change this
			return B_OK;
		}

		void Unlock()
		{
			atomic_add(&fUnlock,1);
		}

	private:
		vint32	fLock;
		vint32	fUnlock;
};

// A convenience class to lock the SimpleLock, note the
// different timing compared to the direct call

class SimpleLocker {
	public:
		SimpleLocker(SimpleLock &lock,bigtime_t time = 1000)
			: fLock(lock)
		{
			lock.Lock(time);
		}

		~SimpleLocker()
		{
			fLock.Unlock();
		}

	private:
		SimpleLock	&fLock;
};

#endif	/* LOCK_H */
