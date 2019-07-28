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
	fWriterNest(0),
	fWriterThread(-1),
#endif
	fInit(B_NO_INIT)
{
#if !DEBUG
	rw_lock_init_etc(&fLock, baseName != NULL ? baseName : "some MultiLocker",
		baseName != NULL ? RW_LOCK_FLAG_CLONE_NAME : 0);
	fInit = B_OK;
#else
	// we are in debug mode!
	fLock = create_sem(LARGE_NUMBER, baseName != NULL ? baseName : "MultiLocker");
	if (fLock >= 0)
		fInit = B_OK;

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
	rw_lock_destroy(&fLock);
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


#if !DEBUG
//	#pragma mark - Standard versions


bool
MultiLocker::IsWriteLocked() const
{
#if TIMING
	bigtime_t start = system_time();
#endif

	bool writeLockHolder = false;

	if (fInit == B_OK)
		writeLockHolder = (find_thread(NULL) == fLock.holder);

#if TIMING
	bigtime_t end = system_time();
	islock_time += (end - start);
	islock_count++;
#endif

	return writeLockHolder;
}


bool
MultiLocker::ReadLock()
{
#if TIMING
	bigtime_t start = system_time();
#endif

	bool locked = (rw_lock_read_lock(&fLock) == B_OK);

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

	bool locked = (rw_lock_write_lock(&fLock) == B_OK);

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

	bool unlocked = (rw_lock_read_unlock(&fLock) == B_OK);

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

	bool unlocked = (rw_lock_write_unlock(&fLock) == B_OK);

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
MultiLocker::IsWriteLocked() const
{
#if TIMING
	bigtime_t start = system_time();
#endif

	bool writeLockHolder = false;

	if (fInit == B_OK)
		writeLockHolder = (find_thread(NULL) == fWriterThread);

#if TIMING
	bigtime_t end = system_time();
	islock_time += (end - start);
	islock_count++;
#endif

	return writeLockHolder;
}


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

	if (IsWriteLocked()) {
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
			fWriterThread = find_thread(NULL);
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
