/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _RW_LOCK_MANAGER_H
#define _RW_LOCK_MANAGER_H

#include <Locker.h>

#include <util/DoublyLinkedList.h>


namespace BPrivate {


class RWLockManager;


class RWLockable {
public:
								RWLockable();

private:
	struct Waiter : DoublyLinkedListLinkImpl<Waiter> {
		Waiter(bool writer)
			:
			thread(find_thread(NULL)),
			writer(writer),
			queued(false)
		{
		}

		thread_id		thread;
		status_t		status;
		bool			writer;
		bool			queued;
	};

	typedef DoublyLinkedList<Waiter> WaiterList;

	friend class RWLockManager;

private:
				thread_id		fOwner;
				int32			fOwnerCount;
				int32			fReaderCount;
				WaiterList		fWaiters;
};


class RWLockManager {
public:
								RWLockManager();
								~RWLockManager();

			status_t			Init()		{ return fLock.InitCheck(); }

			bool				Lock()		{ return fLock.Lock(); }
			void				Unlock()	{ return fLock.Unlock(); }

			bool				ReadLock(RWLockable* lockable);
			bool				TryReadLock(RWLockable* lockable);
			status_t			ReadLockWithTimeout(RWLockable* lockable,
									bigtime_t timeout);
			void				ReadUnlock(RWLockable* lockable);

			bool				WriteLock(RWLockable* lockable);
			bool				TryWriteLock(RWLockable* lockable);
			status_t			WriteLockWithTimeout(RWLockable* lockable,
									bigtime_t timeout);
			void				WriteUnlock(RWLockable* lockable);

	inline	bool				GenericLock(bool write, RWLockable* lockable);
	inline	bool				TryGenericLock(bool write,
									RWLockable* lockable);
	inline	status_t			GenericLockWithTimeout(bool write,
									RWLockable* lockable, bigtime_t timeout);
	inline	void				GenericUnlock(bool write, RWLockable* lockable);

private:
			status_t			_Wait(RWLockable* lockable, bool writer,
									bigtime_t timeout);
			void				_Unblock(RWLockable* lockable);

private:
			BLocker				fLock;
};


inline bool
RWLockManager::GenericLock(bool write, RWLockable* lockable)
{
	return write ? WriteLock(lockable) : ReadLock(lockable);
}


inline bool
RWLockManager::TryGenericLock(bool write, RWLockable* lockable)
{
	return write ? TryWriteLock(lockable) : TryReadLock(lockable);
}


inline status_t
RWLockManager::GenericLockWithTimeout(bool write, RWLockable* lockable,
	bigtime_t timeout)
{
	return write
		? WriteLockWithTimeout(lockable, timeout)
		: ReadLockWithTimeout(lockable, timeout);
}


inline void
RWLockManager::GenericUnlock(bool write, RWLockable* lockable)
{
	if (write)
		WriteUnlock(lockable);
	else
		ReadUnlock(lockable);
}


}	// namespace BPrivate


using BPrivate::RWLockable;
using BPrivate::RWLockManager;


#endif	// _RW_LOCK_MANAGER_H
