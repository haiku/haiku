/*
 * Copyright 2005-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 *
 * Copyright 1999, Be Incorporated. All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
#ifndef MULTI_LOCKER_H
#define MULTI_LOCKER_H


/*! multiple-reader single-writer locking class

	IMPORTANT:
	 * nested read locks are not supported
	 * a reader becoming the write is not supported
	 * nested write locks are supported
	 * a writer can do read locks, even nested ones
	 * in case of problems, #define DEBUG 1 in the .cpp
*/


#include <OS.h>
#include <locks.h>


#define MULTI_LOCKER_TIMING	0
#if DEBUG
#	include <assert.h>
#	define MULTI_LOCKER_DEBUG	DEBUG
#endif

#if MULTI_LOCKER_DEBUG
#	define ASSERT_MULTI_LOCKED(x) assert((x).IsWriteLocked() || (x).IsReadLocked())
#	define ASSERT_MULTI_READ_LOCKED(x) assert((x).IsReadLocked())
#	define ASSERT_MULTI_WRITE_LOCKED(x) assert((x).IsWriteLocked())
#else
#	define MULTI_LOCKER_DEBUG	0
#	define ASSERT_MULTI_LOCKED(x) ;
#	define ASSERT_MULTI_READ_LOCKED(x) ;
#	define ASSERT_MULTI_WRITE_LOCKED(x) ;
#endif


class MultiLocker {
public:
								MultiLocker(const char* baseName);
	virtual						~MultiLocker();

			status_t			InitCheck();

			// locking for reading or writing
			bool				ReadLock();
			bool				WriteLock();

			// unlocking after reading or writing
			bool				ReadUnlock();
			bool				WriteUnlock();

			// does the current thread hold a write lock?
			bool				IsWriteLocked() const;

#if MULTI_LOCKER_DEBUG
			// in DEBUG mode returns whether the lock is held
			// in non-debug mode returns true
			bool				IsReadLocked() const;
#endif

private:
								MultiLocker();
								MultiLocker(const MultiLocker& other);
			MultiLocker&		operator=(const MultiLocker& other);
									// not implemented

#if !MULTI_LOCKER_DEBUG
			rw_lock				fLock;
#else
			// functions for managing the DEBUG reader array
			void				_RegisterThread();
			void				_UnregisterThread();

			sem_id				fLock;
			int32*				fDebugArray;
			int32				fMaxThreads;
			int32				fWriterNest;
			thread_id			fWriterThread;
#endif	// MULTI_LOCKER_DEBUG

			status_t			fInit;

#if MULTI_LOCKER_TIMING
			uint32 				rl_count;
			bigtime_t 			rl_time;
			uint32 				ru_count;
			bigtime_t	 		ru_time;
			uint32				wl_count;
			bigtime_t			wl_time;
			uint32				wu_count;
			bigtime_t			wu_time;
			uint32				islock_count;
			bigtime_t			islock_time;
#endif
};


class AutoWriteLocker {
public:
	AutoWriteLocker(MultiLocker* lock)
		:
		fLock(*lock)
	{
		fLocked = fLock.WriteLock();
	}

	AutoWriteLocker(MultiLocker& lock)
		:
		fLock(lock)
	{
		fLocked = fLock.WriteLock();
	}

	~AutoWriteLocker()
	{
		if (fLocked)
			fLock.WriteUnlock();
	}

	bool IsLocked() const
	{
		return fLock.IsWriteLocked();
	}

	void Unlock()
	{
		if (fLocked) {
			fLock.WriteUnlock();
			fLocked = false;
		}
	}

private:
 	MultiLocker&	fLock;
	bool			fLocked;
};


class AutoReadLocker {
public:
	AutoReadLocker(MultiLocker* lock)
		:
		fLock(*lock)
	{
		fLocked = fLock.ReadLock();
	}

	AutoReadLocker(MultiLocker& lock)
		:
		fLock(lock)
	{
		fLocked = fLock.ReadLock();
	}

	~AutoReadLocker()
	{
		Unlock();
	}

	void Unlock()
	{
		if (fLocked) {
			fLock.ReadUnlock();
			fLocked = false;
		}
	}

private:
	MultiLocker&	fLock;
	bool			fLocked;
};

#endif	// MULTI_LOCKER_H
