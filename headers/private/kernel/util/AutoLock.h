/*
 * Copyright 2005-2007, Ingo Weinhold, bonefish@users.sf.net. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef KERNEL_UTIL_AUTO_LOCKER_H
#define KERNEL_UTIL_AUTO_LOCKER_H


#include <lock.h>


namespace BPrivate {

// AutoLockerStandardLocking
template<typename Lockable>
class AutoLockerStandardLocking {
public:
	inline bool Lock(Lockable *lockable)
	{
		return lockable->Lock();
	}

	inline void Unlock(Lockable *lockable)
	{
		lockable->Unlock();
	}
};

// AutoLockerReadLocking
template<typename Lockable>
class AutoLockerReadLocking {
public:
	inline bool Lock(Lockable *lockable)
	{
		return lockable->ReadLock();
	}

	inline void Unlock(Lockable *lockable)
	{
		lockable->ReadUnlock();
	}
};

// AutoLockerWriteLocking
template<typename Lockable>
class AutoLockerWriteLocking {
public:
	inline bool Lock(Lockable *lockable)
	{
		return lockable->WriteLock();
	}

	inline void Unlock(Lockable *lockable)
	{
		lockable->WriteUnlock();
	}
};

// AutoLocker
template<typename Lockable,
		 typename Locking = AutoLockerStandardLocking<Lockable> >
class AutoLocker {
private:
	typedef AutoLocker<Lockable, Locking>	ThisClass;
public:
	inline AutoLocker()
		: fLockable(NULL),
		  fLocked(false)
	{
	}

	inline AutoLocker(Lockable *lockable, bool alreadyLocked = false,
		bool lockIfNotLocked = true)
		: fLockable(lockable),
		  fLocked(fLockable && alreadyLocked)
	{
		if (!alreadyLocked && lockIfNotLocked)
			Lock();
	}

	inline AutoLocker(Lockable &lockable, bool alreadyLocked = false,
		bool lockIfNotLocked = true)
		: fLockable(&lockable),
		  fLocked(fLockable && alreadyLocked)
	{
		if (!alreadyLocked && lockIfNotLocked)
			Lock();
	}

	inline ~AutoLocker()
	{
		Unlock();
	}

	inline void SetTo(Lockable *lockable, bool alreadyLocked,
		bool lockIfNotLocked = true)
	{
		Unlock();
		fLockable = lockable;
		fLocked = alreadyLocked;
		if (!alreadyLocked && lockIfNotLocked)
			Lock();
	}

	inline void SetTo(Lockable &lockable, bool alreadyLocked,
		bool lockIfNotLocked = true)
	{
		SetTo(&lockable, alreadyLocked, lockIfNotLocked);
	}

	inline void Unset()
	{
		Unlock();
		Detach();
	}

	inline bool Lock()
	{
		if (fLockable && !fLocked)
			fLocked = fLocking.Lock(fLockable);
		return fLocked;
	}

	inline void Unlock()
	{
		if (fLockable && fLocked) {
			fLocking.Unlock(fLockable);
			fLocked = false;
		}
	}

	inline void Detach()
	{
		fLockable = NULL;
		fLocked = false;
	}

	inline AutoLocker<Lockable, Locking> &operator=(Lockable *lockable)
	{
		SetTo(lockable);
		return *this;
	}

	inline AutoLocker<Lockable, Locking> &operator=(Lockable &lockable)
	{
		SetTo(&lockable);
		return *this;
	}

	inline bool IsLocked() const	{ return fLocked; }

	inline operator bool() const	{ return fLocked; }

private:
	Lockable	*fLockable;
	bool		fLocked;
	Locking		fLocking;
};


// #pragma mark -
// #pragma mark ----- instantiations -----

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
