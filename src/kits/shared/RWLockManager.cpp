/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <RWLockManager.h>

#include <AutoLocker.h>

#include <syscalls.h>
#include <user_thread.h>


RWLockable::RWLockable()
	:
	fOwner(-1),
	fOwnerCount(0),
	fReaderCount(0)
{
}


RWLockManager::RWLockManager()
	:
	fLock("r/w lock manager")
{
}


RWLockManager::~RWLockManager()
{
}


bool
RWLockManager::ReadLock(RWLockable* lockable)
{
	AutoLocker<RWLockManager> locker(this);

	if (lockable->fWaiters.IsEmpty()) {
		lockable->fReaderCount++;
		return true;
	}

	return _Wait(lockable, false, B_INFINITE_TIMEOUT) == B_OK;
}


bool
RWLockManager::TryReadLock(RWLockable* lockable)
{
	AutoLocker<RWLockManager> locker(this);

	if (lockable->fWaiters.IsEmpty()) {
		lockable->fReaderCount++;
		return true;
	}

	return false;
}


status_t
RWLockManager::ReadLockWithTimeout(RWLockable* lockable, bigtime_t timeout)
{
	AutoLocker<RWLockManager> locker(this);

	if (lockable->fWaiters.IsEmpty()) {
		lockable->fReaderCount++;
		return B_OK;
	}

	return _Wait(lockable, false, timeout);
}


void
RWLockManager::ReadUnlock(RWLockable* lockable)
{
	AutoLocker<RWLockManager> locker(this);

	if (lockable->fReaderCount <= 0) {
		debugger("RWLockManager::ReadUnlock(): Not read-locked!");
		return;
	}

	if (--lockable->fReaderCount == 0)
		_Unblock(lockable);
}


bool
RWLockManager::WriteLock(RWLockable* lockable)
{
	AutoLocker<RWLockManager> locker(this);

	thread_id thread = find_thread(NULL);

	if (lockable->fOwner == thread) {
		lockable->fOwnerCount++;
		return true;
	}

	if (lockable->fReaderCount == 0 && lockable->fWaiters.IsEmpty()) {
		lockable->fOwnerCount = 1;
		lockable->fOwner = find_thread(NULL);
		return true;
	}

	return _Wait(lockable, true, B_INFINITE_TIMEOUT) == B_OK;
}


bool
RWLockManager::TryWriteLock(RWLockable* lockable)
{
	AutoLocker<RWLockManager> locker(this);

	thread_id thread = find_thread(NULL);

	if (lockable->fOwner == thread) {
		lockable->fOwnerCount++;
		return true;
	}

	if (lockable->fReaderCount == 0 && lockable->fWaiters.IsEmpty()) {
		lockable->fOwnerCount++;
		lockable->fOwner = thread;
		return true;
	}

	return false;
}


status_t
RWLockManager::WriteLockWithTimeout(RWLockable* lockable, bigtime_t timeout)
{
	AutoLocker<RWLockManager> locker(this);

	thread_id thread = find_thread(NULL);

	if (lockable->fOwner == thread) {
		lockable->fOwnerCount++;
		return B_OK;
	}

	if (lockable->fReaderCount == 0 && lockable->fWaiters.IsEmpty()) {
		lockable->fOwnerCount++;
		lockable->fOwner = thread;
		return B_OK;
	}

	return _Wait(lockable, true, timeout);
}


void
RWLockManager::WriteUnlock(RWLockable* lockable)
{
	AutoLocker<RWLockManager> locker(this);

	if (find_thread(NULL) != lockable->fOwner) {
		debugger("RWLockManager::WriteUnlock(): Not write-locked by calling "
			"thread!");
		return;
	}

	if (--lockable->fOwnerCount == 0) {
		lockable->fOwner = -1;
		_Unblock(lockable);
	}
}


status_t
RWLockManager::_Wait(RWLockable* lockable, bool writer, bigtime_t timeout)
{
	if (timeout == 0)
		return B_TIMED_OUT;

	// enqueue a waiter
	RWLockable::Waiter waiter(writer);
	lockable->fWaiters.Add(&waiter);
	waiter.queued = true;
	get_user_thread()->wait_status = 1;

	// wait
	Unlock();

	status_t error;
	do {
		error = _kern_block_thread(
			timeout >= 0 ? B_RELATIVE_TIMEOUT : 0, timeout);
			// TODO: When interrupted we should adjust the timeout, respectively
			// convert to an absolute timeout in the first place!
	} while (error == B_INTERRUPTED);

	Lock();

	if (!waiter.queued)
		return waiter.status;

	// we're still queued, which means an error (timeout, interrupt)
	// occurred
	lockable->fWaiters.Remove(&waiter);

	_Unblock(lockable);

	return error;
}


void
RWLockManager::_Unblock(RWLockable* lockable)
{
	// Check whether there any waiting threads at all and whether anyone
	// has the write lock
	RWLockable::Waiter* waiter = lockable->fWaiters.Head();
	if (waiter == NULL || lockable->fOwner >= 0)
		return;

	// writer at head of queue?
	if (waiter->writer) {
		if (lockable->fReaderCount == 0) {
			waiter->status = B_OK;
			waiter->queued = false;
			lockable->fWaiters.Remove(waiter);
			lockable->fOwner = waiter->thread;
			lockable->fOwnerCount = 1;

			_kern_unblock_thread(waiter->thread, B_OK);
		}
		return;
	}

	// wake up one or more readers -- we unblock more than one reader at
	// a time to save trips to the kernel
	while (!lockable->fWaiters.IsEmpty()
			&& !lockable->fWaiters.Head()->writer) {
		static const int kMaxReaderUnblockCount = 128;
		thread_id readers[kMaxReaderUnblockCount];
		int readerCount = 0;

		while (readerCount < kMaxReaderUnblockCount
				&& (waiter = lockable->fWaiters.Head()) != NULL
				&& !waiter->writer) {
			waiter->status = B_OK;
			waiter->queued = false;
			lockable->fWaiters.Remove(waiter);

			readers[readerCount++] = waiter->thread;
			lockable->fReaderCount++;
		}

		if (readerCount > 0)
			_kern_unblock_threads(readers, readerCount, B_OK);
	}
}
