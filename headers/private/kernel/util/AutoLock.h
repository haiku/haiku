/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_UTIL_AUTO_LOCKER_H
#define KERNEL_UTIL_AUTO_LOCKER_H


#include <lock.h>
#include <shared/AutoLocker.h>


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

}	// namespace BPrivate

using BPrivate::AutoLocker;
using BPrivate::MutexLocker;
using BPrivate::RecursiveLocker;
using BPrivate::BenaphoreLocker;

#endif	// KERNEL_UTIL_AUTO_LOCKER_H
