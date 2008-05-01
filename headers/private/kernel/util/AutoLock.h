/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_UTIL_AUTO_LOCKER_H
#define KERNEL_UTIL_AUTO_LOCKER_H


#include <KernelExport.h>

#include <shared/AutoLocker.h>

#include <int.h>
#include <lock.h>


namespace BPrivate {

// MutexLocking
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

// MutexLocker
typedef AutoLocker<mutex, MutexLocking> MutexLocker;

// RecursiveLockLocking
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

// RecursiveLocker
typedef AutoLocker<recursive_lock, RecursiveLockLocking> RecursiveLocker;

// BenaphoreLocking
class BenaphoreLocking {
public:
	inline bool Lock(benaphore *lockable)
	{
		return benaphore_lock(lockable) == B_OK;
	}

	inline void Unlock(benaphore *lockable)
	{
		benaphore_unlock(lockable);
	}
};

// BenaphoreLocker
typedef AutoLocker<benaphore, BenaphoreLocking> BenaphoreLocker;

// InterruptsLocking
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

// InterruptsLocker
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

// SpinLocking
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

// SpinLocker
typedef AutoLocker<spinlock, SpinLocking> SpinLocker;

// InterruptsSpinLocking
class InterruptsSpinLocking {
public:
	struct State {
		State(spinlock* lock)
			: lock(lock)
		{
		}

		int			state;
		spinlock*	lock;
	};

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

// InterruptsSpinLocker
typedef AutoLocker<spinlock, InterruptsSpinLocking> InterruptsSpinLocker;

}	// namespace BPrivate

using BPrivate::AutoLocker;
using BPrivate::MutexLocker;
using BPrivate::RecursiveLocker;
using BPrivate::BenaphoreLocker;
using BPrivate::InterruptsLocker;
using BPrivate::SpinLocker;
using BPrivate::InterruptsSpinLocker;

#endif	// KERNEL_UTIL_AUTO_LOCKER_H
