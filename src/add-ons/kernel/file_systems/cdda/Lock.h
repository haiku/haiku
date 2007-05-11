/*
 * Copyright 2001-2007, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef LOCK_H
#define LOCK_H


#include <OS.h>


#define USE_BENAPHORE
	// if defined, benaphores are used for the Semaphore class

class Semaphore {
	public:
		Semaphore(const char *name)
			:
#ifdef USE_BENAPHORE
			fSemaphore(create_sem(0, name)),
			fCount(1)
#else
			fSemaphore(create_sem(1, name))
#endif
		{
		}

		~Semaphore()
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
#ifdef USE_BENAPHORE
			if (atomic_add(&fCount, -1) <= 0)
#endif
				return acquire_sem(fSemaphore);
#ifdef USE_BENAPHORE
			return B_OK;
#endif
		}
	
		status_t Unlock()
		{
#ifdef USE_BENAPHORE
			if (atomic_add(&fCount, 1) < 0)
#endif
				return release_sem(fSemaphore);
#ifdef USE_BENAPHORE
			return B_OK;
#endif
		}

	private:
		sem_id	fSemaphore;
#ifdef USE_BENAPHORE
		vint32	fCount;
#endif
};

// a convenience class to lock a Semaphore object

class Locker {
	public:
		Locker(Semaphore &lock)
			: fLock(lock)
		{
			fStatus = lock.Lock();
		}

		~Locker()
		{
			if (fStatus == B_OK)
				fLock.Unlock();
		}

		status_t Status() const
		{
			return fStatus;
		}

	private:
		Semaphore	&fLock;
		status_t	fStatus;
};

#endif	/* LOCK_H */
