/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2005-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_UTIL_AUTO_LOCKER_H
#define KERNEL_UTIL_AUTO_LOCKER_H


#include <KernelExport.h>

#include <shared/AutoLocker.h>

#include <int.h>
#include <lock.h>
#include <thread.h>


namespace BPrivate {


class MutexLocking {
public:
	inline bool Lock(mutex *lockable)
	{
		return mutex_lock(lockable) == B_OK;
	}

	inline void Unlock(mutex *lockable)
	{
		mutex_unlock(lockable);
	}
};

typedef AutoLocker<mutex, MutexLocking> MutexLocker;


class RecursiveLockLocking {
public:
	inline bool Lock(recursive_lock *lockable)
	{
		return recursive_lock_lock(lockable) == B_OK;
	}

	inline void Unlock(recursive_lock *lockable)
	{
		recursive_lock_unlock(lockable);
	}
};

typedef AutoLocker<recursive_lock, RecursiveLockLocking> RecursiveLocker;


class ReadWriteLockReadLocking {
public:
	inline bool Lock(rw_lock *lockable)
	{
		return rw_lock_read_lock(lockable) == B_OK;
	}

	inline void Unlock(rw_lock *lockable)
	{
		rw_lock_read_unlock(lockable);
	}
};

class ReadWriteLockWriteLocking {
public:
	inline bool Lock(rw_lock *lockable)
	{
		return rw_lock_write_lock(lockable) == B_OK;
	}

	inline void Unlock(rw_lock *lockable)
	{
		rw_lock_write_unlock(lockable);
	}
};

typedef AutoLocker<rw_lock, ReadWriteLockReadLocking> ReadLocker;
typedef AutoLocker<rw_lock, ReadWriteLockWriteLocking> WriteLocker;


class InterruptsLocking {
public:
	inline bool Lock(int* lockable)
	{
		*lockable = disable_interrupts();
		return true;
	}

	inline void Unlock(int* lockable)
	{
		restore_interrupts(*lockable);
	}
};


class InterruptsLocker : public AutoLocker<int, InterruptsLocking> {
public:
	inline InterruptsLocker(bool alreadyLocked = false,
		bool lockIfNotLocked = true)
		: AutoLocker<int, InterruptsLocking>(&fState, alreadyLocked,
			lockIfNotLocked)
	{
	}

private:
	int	fState;
};


class SpinLocking {
public:
	inline bool Lock(spinlock* lockable)
	{
		acquire_spinlock(lockable);
		return true;
	}

	inline void Unlock(spinlock* lockable)
	{
		release_spinlock(lockable);
	}
};

typedef AutoLocker<spinlock, SpinLocking> SpinLocker;


class InterruptsSpinLocking {
public:
// NOTE: work-around for annoying GCC 4 "fState may be used uninitialized"
// warning.
#if __GNUC__ == 4
	InterruptsSpinLocking()
		:
		fState(0)
	{
	}
#endif

	inline bool Lock(spinlock* lockable)
	{
		fState = disable_interrupts();
		acquire_spinlock(lockable);
		return true;
	}

	inline void Unlock(spinlock* lockable)
	{
		release_spinlock(lockable);
		restore_interrupts(fState);
	}

private:
	int	fState;
};

typedef AutoLocker<spinlock, InterruptsSpinLocking> InterruptsSpinLocker;


class ThreadCPUPinLocking {
public:
	inline bool Lock(Thread* thread)
	{
		thread_pin_to_current_cpu(thread);
		return true;
	}

	inline void Unlock(Thread* thread)
	{
		thread_unpin_from_current_cpu(thread);
	}
};

typedef AutoLocker<Thread, ThreadCPUPinLocking> ThreadCPUPinner;


typedef AutoLocker<Team> TeamLocker;
typedef AutoLocker<Thread> ThreadLocker;


}	// namespace BPrivate

using BPrivate::AutoLocker;
using BPrivate::MutexLocker;
using BPrivate::RecursiveLocker;
using BPrivate::ReadLocker;
using BPrivate::WriteLocker;
using BPrivate::InterruptsLocker;
using BPrivate::SpinLocker;
using BPrivate::InterruptsSpinLocker;
using BPrivate::ThreadCPUPinner;
using BPrivate::TeamLocker;
using BPrivate::ThreadLocker;


#endif	// KERNEL_UTIL_AUTO_LOCKER_H
