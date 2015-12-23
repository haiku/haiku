/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		IngoWeinhold <bonefish@cs.tu-berlin.de>
 */

#include "RWLocker.h"

#include <String.h>

// info about a read lock owner
struct RWLocker::ReadLockInfo {
	thread_id	reader;
	int32		count;
};


// constructor
RWLocker::RWLocker()
	: fLock(),
	  fMutex(),
	  fQueue(),
	  fReaderCount(0),
	  fWriterCount(0),
	  fReadLockInfos(8),
	  fWriter(B_ERROR),
	  fWriterWriterCount(0),
	  fWriterReaderCount(0)
{
	_Init(NULL);
}

// constructor
RWLocker::RWLocker(const char* name)
	: fLock(name),
	  fMutex(),
	  fQueue(),
	  fReaderCount(0),
	  fWriterCount(0),
	  fReadLockInfos(8),
	  fWriter(B_ERROR),
	  fWriterWriterCount(0),
	  fWriterReaderCount(0)
{
	_Init(name);
}

// destructor
RWLocker::~RWLocker()
{
	fLock.Lock();
	delete_sem(fMutex.semaphore);
	delete_sem(fQueue.semaphore);
	for (int32 i = 0; ReadLockInfo* info = _ReadLockInfoAt(i); i++)
		delete info;
}

// ReadLock
bool
RWLocker::ReadLock()
{
	status_t error = _ReadLock(B_INFINITE_TIMEOUT);
	return (error == B_OK);
}

// ReadLockWithTimeout
status_t
RWLocker::ReadLockWithTimeout(bigtime_t timeout)
{
	bigtime_t absoluteTimeout = system_time() + timeout;
	// take care of overflow
	if (timeout > 0 && absoluteTimeout < 0)
		absoluteTimeout = B_INFINITE_TIMEOUT;
	return _ReadLock(absoluteTimeout);
}

// ReadUnlock
void
RWLocker::ReadUnlock()
{
	if (fLock.Lock()) {
		thread_id thread = find_thread(NULL);
		if (thread == fWriter) {
			// We (also) have a write lock.
			if (fWriterReaderCount > 0)
				fWriterReaderCount--;
			// else: error: unmatched ReadUnlock()
		} else {
			int32 index = _IndexOf(thread);
			if (ReadLockInfo* info = _ReadLockInfoAt(index)) {
				fReaderCount--;
				if (--info->count == 0) {
					// The outer read lock bracket for the thread has been
					// reached. Dispose the info.
					_DeleteReadLockInfo(index);
				}
				if (fReaderCount == 0) {
					// The last reader needs to unlock the mutex.
					_ReleaseBenaphore(fMutex);
				}
			}	// else: error: caller has no read lock
		}
		fLock.Unlock();
	}	// else: we are probably going to be destroyed
}

// IsReadLocked
//
// Returns whether or not the calling thread owns a read lock or even a
// write lock.
bool
RWLocker::IsReadLocked() const
{
	bool result = false;
	if (fLock.Lock()) {
		thread_id thread = find_thread(NULL);
		result = (thread == fWriter || _IndexOf(thread) >= 0);
		fLock.Unlock();
	}
	return result;
}

// WriteLock
bool
RWLocker::WriteLock()
{
	status_t error = _WriteLock(B_INFINITE_TIMEOUT);
	return (error == B_OK);
}

// WriteLockWithTimeout
status_t
RWLocker::WriteLockWithTimeout(bigtime_t timeout)
{
	bigtime_t absoluteTimeout = system_time() + timeout;
	// take care of overflow
	if (timeout > 0 && absoluteTimeout < 0)
		absoluteTimeout = B_INFINITE_TIMEOUT;
	return _WriteLock(absoluteTimeout);
}

// WriteUnlock
void
RWLocker::WriteUnlock()
{
	if (fLock.Lock()) {
		thread_id thread = find_thread(NULL);
		if (thread == fWriter) {
			fWriterCount--;
			if (--fWriterWriterCount == 0) {
				// The outer write lock bracket for the thread has been
				// reached.
				fWriter = B_ERROR;
				if (fWriterReaderCount > 0) {
					// We still own read locks.
					_NewReadLockInfo(thread, fWriterReaderCount);
					// A reader that expects to be the first reader may wait
					// at the mutex semaphore. We need to wake it up.
					if (fReaderCount > 0)
						_ReleaseBenaphore(fMutex);
					fReaderCount += fWriterReaderCount;
					fWriterReaderCount = 0;
				} else {
					// We don't own any read locks. So we have to release the
					// mutex benaphore.
					_ReleaseBenaphore(fMutex);
				}
			}
		}	// else: error: unmatched WriteUnlock()
		fLock.Unlock();
	}	// else: We're probably going to die.
}

// IsWriteLocked
//
// Returns whether or not the calling thread owns a write lock.
bool
RWLocker::IsWriteLocked() const
{
	return (fWriter == find_thread(NULL));
}

// _Init
void
RWLocker::_Init(const char* name)
{
	// init the mutex benaphore
	BString mutexName(name);
	mutexName += "_RWLocker_mutex";
	fMutex.semaphore = create_sem(0, mutexName.String());
	fMutex.counter = 0;
	// init the queueing benaphore
	BString queueName(name);
	queueName += "_RWLocker_queue";
	fQueue.semaphore = create_sem(0, queueName.String());
	fQueue.counter = 0;
}

// _ReadLock
//
// /timeout/ -- absolute timeout
status_t
RWLocker::_ReadLock(bigtime_t timeout)
{
	status_t error = B_OK;
	thread_id thread = find_thread(NULL);
	bool locked = false;
	if (fLock.Lock()) {
		// Check, if we already own a read (or write) lock. In this case we
		// can skip the usual locking procedure.
		if (thread == fWriter) {
			// We already own a write lock.
			fWriterReaderCount++;
			locked = true;
		} else if (ReadLockInfo* info = _ReadLockInfoAt(_IndexOf(thread))) {
			// We already own a read lock.
			info->count++;
			fReaderCount++;
			locked = true;
		}
		fLock.Unlock();
	} else	// failed to lock the data
		error = B_ERROR;
	// Usual locking, i.e. we do not already own a read or write lock.
	if (error == B_OK && !locked) {
		error = _AcquireBenaphore(fQueue, timeout);
		if (error == B_OK) {
			if (fLock.Lock()) {
				bool firstReader = false;
				if (++fReaderCount == 1) {
					// We are the first reader.
					_NewReadLockInfo(thread);
					firstReader = true;
				} else
					_NewReadLockInfo(thread);
				fLock.Unlock();
				// The first reader needs to lock the mutex.
				if (firstReader) {
					error = _AcquireBenaphore(fMutex, timeout);
					switch (error) {
						case B_OK:
							// fine
							break;
						case B_TIMED_OUT: {
							// clean up
							if (fLock.Lock()) {
								_DeleteReadLockInfo(_IndexOf(thread));
								fReaderCount--;
								fLock.Unlock();
							}
							break;
						}
						default:
							// Probably we are going to be destroyed.
							break;
					}
				}
				// Let the next candidate enter the game.
				_ReleaseBenaphore(fQueue);
			} else {
				// We couldn't lock the data, which can only happen, if
				// we're going to be destroyed.
				error = B_ERROR;
			}
		}
	}
	return error;
}

// _WriteLock
//
// /timeout/ -- absolute timeout
status_t
RWLocker::_WriteLock(bigtime_t timeout)
{
	status_t error = B_ERROR;
	if (fLock.Lock()) {
		bool infiniteTimeout = (timeout == B_INFINITE_TIMEOUT);
		bool locked = false;
		int32 readerCount = 0;
		thread_id thread = find_thread(NULL);
		int32 index = _IndexOf(thread);
		if (ReadLockInfo* info = _ReadLockInfoAt(index)) {
			// We already own a read lock.
			if (fWriterCount > 0) {
				// There are writers before us.
				if (infiniteTimeout) {
					// Timeout is infinite and there are writers before us.
					// Unregister the read locks and lock as usual.
					readerCount = info->count;
					fWriterCount++;
					fReaderCount -= readerCount;
					_DeleteReadLockInfo(index);
					error = B_OK;
				} else {
					// The timeout is finite and there are readers before us:
					// let the write lock request fail.
					error = B_WOULD_BLOCK;
				}
			} else if (info->count == fReaderCount) {
				// No writers before us.
				// We are the only read lock owners. Just move the read lock
				// info data to the special writer fields and then we are done.
				// Note: At this point we may overtake readers that already
				// have acquired the queueing benaphore, but have not yet
				// locked the data. But that doesn't harm.
				fWriter = thread;
				fWriterCount++;
				fWriterWriterCount = 1;
				fWriterReaderCount = info->count;
				fReaderCount -= fWriterReaderCount;
				_DeleteReadLockInfo(index);
				locked = true;
				error = B_OK;
			} else {
				// No writers before us, but other readers.
				// Note, we're quite restrictive here. If there are only
				// readers before us, we could reinstall our readers, if
				// our request times out. Unfortunately it is not easy
				// to ensure, that no writer overtakes us between unlocking
				// the data and acquiring the queuing benaphore.
				if (infiniteTimeout) {
					// Unregister the readers and lock as usual.
					readerCount = info->count;
					fWriterCount++;
					fReaderCount -= readerCount;
					_DeleteReadLockInfo(index);
					error = B_OK;
				} else
					error = B_WOULD_BLOCK;
			}
		} else {
			// We don't own a read lock.
			if (fWriter == thread) {
				// ... but a write lock.
				fWriterCount++;
				fWriterWriterCount++;
				locked = true;
				error = B_OK;
			} else {
				// We own neither read nor write locks.
				// Lock as usual.
				fWriterCount++;
				error = B_OK;
			}
		}
		fLock.Unlock();
		// Usual locking...
		// First step: acquire the queueing benaphore.
		if (!locked && error == B_OK) {
			error = _AcquireBenaphore(fQueue, timeout);
			switch (error) {
				case B_OK:
					break;
				case B_TIMED_OUT: {
					// clean up
					if (fLock.Lock()) {
						fWriterCount--;
						fLock.Unlock();
					}	// else: failed to lock the data: we're probably going
						// to die.
					break;
				}
				default:
					// Probably we're going to die.
					break;
			}
		}
		// Second step: acquire the mutex benaphore.
		if (!locked && error == B_OK) {
			error = _AcquireBenaphore(fMutex, timeout);
			switch (error) {
				case B_OK: {
					// Yeah, we made it. Set the special writer fields.
					fWriter = thread;
					fWriterWriterCount = 1;
					fWriterReaderCount = readerCount;
					break;
				}
				case B_TIMED_OUT: {
					// clean up
					if (fLock.Lock()) {
						fWriterCount--;
						fLock.Unlock();
					}	// else: failed to lock the data: we're probably going
						// to die.
					break;
				}
				default:
					// Probably we're going to die.
					break;
			}
			// Whatever happened, we have to release the queueing benaphore.
			_ReleaseBenaphore(fQueue);
		}
	} else	// failed to lock the data
		error = B_ERROR;
	return error;
}

// _AddReadLockInfo
int32
RWLocker::_AddReadLockInfo(ReadLockInfo* info)
{
	int32 index = fReadLockInfos.CountItems();
	fReadLockInfos.AddItem(info, index);
	return index;
}

// _NewReadLockInfo
//
// Create a new read lock info for the supplied thread and add it to the
// list. Returns the index of the info.
int32
RWLocker::_NewReadLockInfo(thread_id thread, int32 count)
{
	ReadLockInfo* info = new ReadLockInfo;
	info->reader = thread;
	info->count = count;
	return _AddReadLockInfo(info);
}

// _DeleteReadLockInfo
void
RWLocker::_DeleteReadLockInfo(int32 index)
{
	if (ReadLockInfo* info = (ReadLockInfo*)fReadLockInfos.RemoveItem(index))
		delete info;
}

// _ReadLockInfoAt
RWLocker::ReadLockInfo*
RWLocker::_ReadLockInfoAt(int32 index) const
{
	return (ReadLockInfo*)fReadLockInfos.ItemAt(index);
}

// _IndexOf
int32
RWLocker::_IndexOf(thread_id thread) const
{
	int32 count = fReadLockInfos.CountItems();
	for (int32 i = 0; i < count; i++) {
		if (_ReadLockInfoAt(i)->reader == thread)
			return i;
	}
	return -1;
}

// _AcquireBenaphore
status_t
RWLocker::_AcquireBenaphore(Benaphore& benaphore, bigtime_t timeout)
{
	status_t error = B_OK;
	if (atomic_add(&benaphore.counter, 1) > 0) {
		error = acquire_sem_etc(benaphore.semaphore, 1, B_ABSOLUTE_TIMEOUT,
								timeout);
	}
	return error;
}

// _ReleaseBenaphore
void
RWLocker::_ReleaseBenaphore(Benaphore& benaphore)
{
	if (atomic_add(&benaphore.counter, -1) > 1)
		release_sem(benaphore.semaphore);
}

