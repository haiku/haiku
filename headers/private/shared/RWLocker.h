/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		IngoWeinhold <bonefish@cs.tu-berlin.de>
 */

// This class provides a reader/writer locking mechanism:
// * A writer needs an exclusive lock.
// * For a reader a non-exclusive lock to be shared with other readers is
//   sufficient.
// * The ownership of a lock is bound to the thread that requested the lock;
//   the same thread has to call Unlock() later.
// * Nested locking is supported: a number of XXXLock() calls needs to be
//   bracketed by the same number of XXXUnlock() calls.
// * The lock acquiration strategy is fair: a lock applicant needs to wait
//   only for those threads that already own a lock or requested one before
//   the current thread. No one can overtake. E.g. if a thread owns a read
//   lock, another one is waiting for a write lock, then a third one
//   requesting a read lock has to wait until the write locker is done.
//   This does not hold for threads that already own a lock (nested locking).
//   A read lock owner is immediately granted another read lock and a write
//   lock owner another write or a read lock.
// * A write lock owner is allowed to request a read lock and a read lock
//   owner a write lock. While the first case is not problematic, the
//   second one needs some further explanation: A read lock owner requesting
//   a write lock temporarily looses its read lock(s) until the write lock
//   is granted. Otherwise two read lock owning threads trying to get
//   write locks at the same time would dead lock each other. The only
//   problem with this solution is, that the write lock acquiration must
//   not fail, because in that case the thread could not be given back
//   its read lock(s), since another thread may have been given a write lock
//   in the mean time. Fortunately locking can fail only either, if the
//   locker has been deleted, or, if a timeout occured. Therefore
//   WriteLockWithTimeout() immediatlely returns with a B_WOULD_BLOCK error
//   code, if the caller already owns a read lock (but no write lock) and
//   another thread already owns or has requested a read or write lock.
// * Calls to read and write locking methods may interleave arbitrarily,
//   e.g.: ReadLock(); WriteLock(); ReadUnlock(); WriteUnlock();
//
// Important note: Read/WriteLock() can fail only, if the locker has been
// deleted. However, it is NOT save to invoke any method on a deleted
// locker object.
//
// Implementation details:
// A locker needs three semaphores (a BLocker and two semaphores): one
// to protect the lockers data, one as a reader/writer mutex (to be
// acquired by each writer and the first reader) and one for queueing
// waiting readers and writers. The simplified locking/unlocking
// algorithm is the following:
//
//		writer				reader
//	queue.acquire()		queue.acquire()
//	mutex.acquire()		if (first reader) mutex.acquire()
//	queue.release()		queue.release()
//		 ...				 ...
//	mutex.release()		if (last reader) mutex.release()
//
// One thread at maximum waits at the mutex, the others at the queueing
// semaphore. Unfortunately features as nested locking and timeouts make
// things more difficult. Therefore readers as well as writers need to check
// whether they already own a lock before acquiring the queueing semaphore.
// The data for the readers are stored in a list of ReadLockInfo structures;
// the writer data are stored in some special fields. /fReaderCount/ and
// /fWriterCount/ contain the total count of unbalanced Read/WriteLock()
// calls, /fWriterReaderCount/ and /fWriterWriterCount/ only from those of
// the current write lock owner (/fWriter/). To be a bit more precise:
// /fWriterReaderCount/ is not contained in /fReaderCount/, but
// /fWriterWriterCount/ is contained in /fWriterCount/. Therefore
// /fReaderCount/ can be considered to be the count of true reader's read
// locks.

#ifndef RW_LOCKER_H
#define RW_LOCKER_H

#include <List.h>
#include <Locker.h>

#include "AutoLocker.h"

class RWLocker {
 public:
								RWLocker();
								RWLocker(const char* name);
	virtual						~RWLocker();

			bool				ReadLock();
			status_t			ReadLockWithTimeout(bigtime_t timeout);
			void				ReadUnlock();
			bool				IsReadLocked() const;

			bool				WriteLock();
			status_t			WriteLockWithTimeout(bigtime_t timeout);
			void				WriteUnlock();
			bool				IsWriteLocked() const;

 private:
	struct	ReadLockInfo;
	struct	Benaphore {
			sem_id	semaphore;
			int32	counter;
	};

 private:
			void				_Init(const char* name);
			status_t			_ReadLock(bigtime_t timeout);
			status_t			_WriteLock(bigtime_t timeout);

			int32				_AddReadLockInfo(ReadLockInfo* info);
			int32				_NewReadLockInfo(thread_id thread,
												 int32 count = 1);
			void				_DeleteReadLockInfo(int32 index);
			ReadLockInfo*		_ReadLockInfoAt(int32 index) const;
			int32				_IndexOf(thread_id thread) const;

	static	status_t			_AcquireBenaphore(Benaphore& benaphore,
												  bigtime_t timeout);
	static	void				_ReleaseBenaphore(Benaphore& benaphore);

 private:
	mutable	BLocker				fLock;				// data lock
			Benaphore			fMutex;				// critical code mutex
			Benaphore			fQueue;				// queueing semaphore
			int32				fReaderCount;		// total count...
			int32				fWriterCount;		// total count...
			BList				fReadLockInfos;
			thread_id			fWriter;			// current write lock owner
			int32				fWriterWriterCount;	// write lock owner count
			int32				fWriterReaderCount;	// writer read lock owner
													// count
};

typedef AutoLocker<RWLocker, AutoLockerReadLocking<RWLocker> > AutoReadLocker;
typedef AutoLocker<RWLocker, AutoLockerWriteLocking<RWLocker> > AutoWriteLocker;

#endif	// RW_LOCKER_H
