/*
 * Copyright 2005-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */


#include "MultiLocker.h"

#include <Debug.h>
#include <Errors.h>
#include <OS.h>


#define TIMING	MULTI_LOCKER_TIMING
#ifndef DEBUG
#	define DEBUG	MULTI_LOCKER_DEBUG
#endif


const int32 LARGE_NUMBER = 100000;


MultiLocker::MultiLocker(const char* baseName)
	:
#if DEBUG
	fDebugArray(NULL),
	fMaxThreads(0),
#else
	fReadCount(0),
	fWriteCount(0),
	fLockCount(0),
#endif
	fInit(B_NO_INIT),
	fWriterNest(0),
	fWriterThread(-1),
	fWriterStackBase(0)
{
	// build the semaphores
#if !DEBUG
	if (baseName) {
		char name[128];
		snprintf(name, sizeof(name), "%s-%s", baseName, "ReadSem");
		fReadSem = create_sem(0, name);
		snprintf(name, sizeof(name), "%s-%s", baseName, "WriteSem");
		fWriteSem = create_sem(0, name);
		snprintf(name, sizeof(name), "%s-%s", baseName, "WriterLock");
		fWriterLock = create_sem(0, name);
	} else {
		fReadSem = create_sem(0, "MultiLocker_ReadSem");
		fWriteSem = create_sem(0, "MultiLocker_WriteSem");
		fWriterLock = create_sem(0, "MultiLocker_WriterLock");
	}

	if (fReadSem >= 0 && fWriteSem >=0 && fWriterLock >= 0)
		fInit = B_OK;
#else
	fLock = create_sem(LARGE_NUMBER, baseName != NULL ? baseName : "MultiLocker");
	if (fLock >= 0)
		fInit = B_OK;

	// we are in debug mode!
	// create the reader tracking list
	// the array needs to be large enough to hold all possible threads
	system_info sys;
	get_system_info(&sys);
	fMaxThreads = sys.max_threads;
	fDebugArray = (int32 *) malloc(fMaxThreads * sizeof(int32));
	for (int32 i = 0; i < fMaxThreads; i++) {
		fDebugArray[i] = 0;
	}
#endif
#if TIMING
	//initialize the counter variables
	rl_count = ru_count = wl_count = wu_count = islock_count = 0;
	rl_time = ru_time = wl_time = wu_time = islock_time = 0;
	#if DEBUG
		reg_count = unreg_count = 0;
		reg_time = unreg_time = 0;
	#endif
#endif
}


MultiLocker::~MultiLocker()
{
	// become the writer
	if (!IsWriteLocked())
		WriteLock();

	// set locker to be uninitialized
	fInit = B_NO_INIT;

#if !DEBUG
	// delete the semaphores
	delete_sem(fReadSem);
	delete_sem(fWriteSem);
	delete_sem(fWriterLock);
#else
	delete_sem(fLock);
	free(fDebugArray);
#endif
#if TIMING
	// let's produce some performance numbers
	printf("MultiLocker Statistics:\n"
		"Avg ReadLock: %lld\n"
		"Avg ReadUnlock: %lld\n"
		"Avg WriteLock: %lld\n"
		"Avg WriteUnlock: %lld\n"
		"Avg IsWriteLocked: %lld\n",
		rl_count > 0 ? rl_time / rl_count : 0,
		ru_count > 0 ? ru_time / ru_count : 0,
		wl_count > 0 ? wl_time / wl_count : 0,
		wu_count > 0 ? wu_time / wu_count : 0,
		islock_count > 0 ? islock_time / islock_count : 0);
#endif
}


status_t
MultiLocker::InitCheck()
{
	return fInit;
}


/*!
	This function demonstrates a nice method of determining if the current thread
	is the writer or not.  The method involves caching the index of the page in memory
	where the thread's stack is located.  Each time a new writer acquires the lock,
	its thread_id and stack_page are recorded.  IsWriteLocked gets the stack_page of the
	current thread and sees if it is a match.  If the stack_page matches you are guaranteed
	to have the matching thread.  If the stack page doesn't match the more traditional
	find_thread(NULL) method of matching the thread_ids is used.

	This technique is very useful when dealing with a lock that is acquired in a nested fashion.
	It could be expanded to cache the information of the last thread in the lock, and then if
	the same thread returns while there is no one in the lock, it could save some time, if the
	same thread is likely to acquire the lock again and again.
	I should note another shortcut that could be implemented here
	If fWriterThread is set to -1 then there is no writer in the lock, and we could
	return from this function much faster.  However the function is currently set up
	so all of the stack_base and thread_id info is determined here.  WriteLock passes
	in some variables so that if the lock is not held it does not have to get the thread_id
	and stack base again.  Instead this function returns that information.  So this shortcut
	would only move this information gathering outside of this function, and I like it all
	contained.
*/
bool
MultiLocker::IsWriteLocked(addr_t* _stackBase, thread_id* _thread) const
{
#if TIMING
	bigtime_t start = system_time();
#endif

	// get a variable on the stack
	bool writeLockHolder = false;

	if (fInit == B_OK) {
		// determine which page in memory this stack represents
		// this is managed by taking the address of the item on the
		// stack and dividing it by the size of the memory pages
		// if it is the same as the cached stack_page, there is a match
		addr_t stackBase = (addr_t)&writeLockHolder / B_PAGE_SIZE;
		thread_id thread = 0;

		if (fWriterStackBase == stackBase) {
			writeLockHolder = true;
		} else {
			// as there was no stack page match we resort to the
			// tried and true methods
			thread = find_thread(NULL);
			if (fWriterThread == thread)
				writeLockHolder = true;
		}

		// if someone wants this information, give it to them
		if (_stackBase != NULL)
			*_stackBase = stackBase;
		if (_thread != NULL)
			*_thread = thread;
	}

#if TIMING
	bigtime_t end = system_time();
	islock_time += (end - start);
	islock_count++;
#endif

	return writeLockHolder;
}


#if !DEBUG
//	#pragma mark - Standard versions


bool
MultiLocker::ReadLock()
{
#if TIMING
	bigtime_t start = system_time();
#endif

	bool locked = false;

	// the lock must be initialized
	if (fInit == B_OK) {
		if (IsWriteLocked()) {
			// the writer simply increments the nesting
			fWriterNest++;
			locked = true;
		} else {
			// increment and retrieve the current count of readers
			int32 currentCount = atomic_add(&fReadCount, 1);
			if (currentCount < 0) {
				// a writer holds the lock so wait for fReadSem to be released
				status_t status;
				do {
					status = acquire_sem(fReadSem);
				} while (status == B_INTERRUPTED);

				locked = status == B_OK;
			} else
				locked = true;
		}
	}

#if TIMING
	bigtime_t end = system_time();
	rl_time += (end - start);
	rl_count++;
#endif

	return locked;
}


bool
MultiLocker::WriteLock()
{
#if TIMING
	bigtime_t start = system_time();
#endif

	bool locked = false;

	if (fInit == B_OK) {
		addr_t stackBase = 0;
		thread_id thread = -1;

		if (IsWriteLocked(&stackBase, &thread)) {
			// already the writer - increment the nesting count
			fWriterNest++;
			locked = true;
		} else {
			// new writer acquiring the lock
			if (atomic_add(&fLockCount, 1) >= 1) {
				// another writer in the lock - acquire the semaphore
				status_t status;
				do {
					status = acquire_sem(fWriterLock);
				} while (status == B_INTERRUPTED);

				locked = status == B_OK;
			} else
				locked = true;

			if (locked) {
				// new holder of the lock

				// decrement fReadCount by a very large number
				// this will cause new readers to block on fReadSem
				int32 readers = atomic_add(&fReadCount, -LARGE_NUMBER);
				if (readers > 0) {
					// readers hold the lock - acquire fWriteSem
					status_t status;
					do {
						status = acquire_sem_etc(fWriteSem, readers, 0, 0);
					} while (status == B_INTERRUPTED);

					locked = status == B_OK;
				}
				if (locked) {
					ASSERT(fWriterThread == -1);
					// record thread information
					fWriterThread = thread;
					fWriterStackBase = stackBase;
				}
			}
		}
	}

#if TIMING
	bigtime_t end = system_time();
	wl_time += (end - start);
	wl_count++;
#endif

	return locked;
}


bool
MultiLocker::ReadUnlock()
{
#if TIMING
	bigtime_t start = system_time();
#endif

	bool unlocked = false;

	if (IsWriteLocked()) {
		// writers simply decrement the nesting count
		fWriterNest--;
		unlocked = true;
	} else {
		// decrement and retrieve the read counter
		int32 current_count = atomic_add(&fReadCount, -1);
		if (current_count < 0) {
			// a writer is waiting for the lock so release fWriteSem
			unlocked = release_sem_etc(fWriteSem, 1, B_DO_NOT_RESCHEDULE) == B_OK;
		} else
			unlocked = true;
	}

#if TIMING
	bigtime_t end = system_time();
	ru_time += (end - start);
	ru_count++;
#endif

	return unlocked;
}


bool
MultiLocker::WriteUnlock()
{
#if TIMING
	bigtime_t start = system_time();
#endif

	bool unlocked = false;

	if (IsWriteLocked()) {
		// if this is a nested lock simply decrement the nest count
		if (fWriterNest > 0) {
			fWriterNest--;
			unlocked = true;
		} else {
			// writer finally unlocking

			// increment fReadCount by a large number
			// this will let new readers acquire the read lock
			// retrieve the number of current waiters
			int32 readersWaiting = atomic_add(&fReadCount, LARGE_NUMBER)
				+ LARGE_NUMBER;

			if (readersWaiting > 0) {
				// readers are waiting to acquire the lock
				unlocked = release_sem_etc(fReadSem, readersWaiting,
					B_DO_NOT_RESCHEDULE) == B_OK;
			} else
				unlocked = true;

			if (unlocked) {
				// clear the information
				fWriterThread = -1;
				fWriterStackBase = 0;

				// decrement and retrieve the lock count
				if (atomic_add(&fLockCount, -1) > 1) {
					// other writers are waiting so release fWriterLock
					unlocked = release_sem_etc(fWriterLock, 1,
						B_DO_NOT_RESCHEDULE) == B_OK;
				}
			}
		}
	} else
		debugger("Non-writer attempting to WriteUnlock()");

#if TIMING
	bigtime_t end = system_time();
	wu_time += (end - start);
	wu_count++;
#endif

	return unlocked;
}


#else	// DEBUG
//	#pragma mark - Debug versions


bool
MultiLocker::ReadLock()
{
	bool locked = false;

	if (fInit != B_OK)
		debugger("lock not initialized");

	if (IsWriteLocked()) {
		if (fWriterNest < 0)
			debugger("ReadLock() - negative writer nest count");

		fWriterNest++;
		locked = true;
	} else {
		status_t status;
		do {
			status = acquire_sem(fLock);
		} while (status == B_INTERRUPTED);

		locked = status == B_OK;

		if (locked)
			_RegisterThread();
	}

	return locked;
}


bool
MultiLocker::WriteLock()
{
	bool locked = false;

	if (fInit != B_OK)
		debugger("lock not initialized");

	addr_t stackBase = 0;
	thread_id thread = -1;

	if (IsWriteLocked(&stackBase, &thread)) {
		if (fWriterNest < 0)
			debugger("WriteLock() - negative writer nest count");

		fWriterNest++;
		locked = true;
	} else {
		// new writer acquiring the lock
		if (IsReadLocked())
			debugger("Reader wants to become writer!");

		status_t status;
		do {
			status = acquire_sem_etc(fLock, LARGE_NUMBER, 0, 0);
		} while (status == B_INTERRUPTED);

		locked = status == B_OK;
		if (locked) {
			// record thread information
			fWriterThread = thread;
			fWriterStackBase = stackBase;
		}
	}

	return locked;
}


bool
MultiLocker::ReadUnlock()
{
	bool unlocked = false;

	if (IsWriteLocked()) {
		// writers simply decrement the nesting count
		fWriterNest--;
		if (fWriterNest < 0)
			debugger("ReadUnlock() - negative writer nest count");

		unlocked = true;
	} else {
		// decrement and retrieve the read counter
		unlocked = release_sem_etc(fLock, 1, B_DO_NOT_RESCHEDULE) == B_OK;
		if (unlocked)
			_UnregisterThread();
	}

	return unlocked;
}


bool
MultiLocker::WriteUnlock()
{
	bool unlocked = false;

	if (IsWriteLocked()) {
		// if this is a nested lock simply decrement the nest count
		if (fWriterNest > 0) {
			fWriterNest--;
			unlocked = true;
		} else {
			// clear the information while still holding the lock
			fWriterThread = -1;
			fWriterStackBase = 0;
			unlocked = release_sem_etc(fLock, LARGE_NUMBER,
				B_DO_NOT_RESCHEDULE) == B_OK;
		}
	} else {
		char message[256];
		snprintf(message, sizeof(message), "Non-writer attempting to "
			"WriteUnlock() - write holder: %" B_PRId32, fWriterThread);
		debugger(message);
	}

	return unlocked;
}


bool
MultiLocker::IsReadLocked() const
{
	if (fInit == B_NO_INIT)
		return false;

	// determine if the lock is actually held
	thread_id thread = find_thread(NULL);
	return fDebugArray[thread % fMaxThreads] > 0;
}


/* these two functions manage the debug array for readers */
/* an array is created in the constructor large enough to hold */
/* an int32 for each of the maximum number of threads the system */
/* can have at one time. */
/* this array does not need to be locked because each running thread */
/* can be uniquely mapped to a slot in the array by performing: */
/* 		thread_id % max_threads */
/* each time ReadLock is called while in debug mode the thread_id	*/
/* is retrived in register_thread() and the count is adjusted in the */
/* array.  If register thread is ever called and the count is not 0 then */
/* an illegal, potentially deadlocking nested ReadLock occured */
/* unregister_thread clears the appropriate slot in the array */

/* this system could be expanded or retracted to include multiple arrays of information */
/* in all fairness for it's current use, fDebugArray could be an array of bools */

/* The disadvantage of this system for maintaining state is that it sucks up a ton of */
/* memory.  The other method (which would be slower), would involve an additional lock and */
/* traversing a list of cached information.  As this is only for a debug mode, the extra memory */
/* was not deemed to be a problem */

void
MultiLocker::_RegisterThread()
{
	thread_id thread = find_thread(NULL);

	if (fDebugArray[thread % fMaxThreads] != 0)
		debugger("Nested ReadLock!");

	fDebugArray[thread % fMaxThreads]++;
}


void
MultiLocker::_UnregisterThread()
{
	thread_id thread = find_thread(NULL);

	ASSERT(fDebugArray[thread % fMaxThreads] == 1);
	fDebugArray[thread % fMaxThreads]--;
}

#endif	// DEBUG
