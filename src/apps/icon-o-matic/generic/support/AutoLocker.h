/*
 * Copyright 2004-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		IngoWeinhold <bonefish@cs.tu-berlin.de>
 */

/**	Scope-based automatic deletion of objects/arrays.
 *					ObjectDeleter - deletes an object
 *					ArrayDeleter  - deletes an array
 *					MemoryDeleter - free()s malloc()ed memory
 */

#ifndef AUTO_LOCKER_H
#define AUTO_LOCKER_H

#include <SupportDefs.h>

// locking

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
	inline AutoLocker(Lockable *lockable, bool alreadyLocked = false)
		: fLockable(lockable),
		  fLocked(fLockable && alreadyLocked)
	{
		if (!fLocked)
			_Lock();
	}

	inline AutoLocker(Lockable &lockable, bool alreadyLocked = false)
		: fLockable(&lockable),
		  fLocked(fLockable && alreadyLocked)
	{
		if (!fLocked)
			_Lock();
	}

	inline ~AutoLocker()
	{
		Unlock();
	}

	inline void SetTo(Lockable *lockable, bool alreadyLocked)
	{
		Unlock();
		fLockable = lockable;
		fLocked = alreadyLocked;
		if (!fLocked)
			_Lock();
	}

	inline void SetTo(Lockable &lockable, bool alreadyLocked)
	{
		SetTo(&lockable, alreadyLocked);
	}

	inline void Unset()
	{
		Unlock();
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

	inline void Unlock()
	{
		if (fLockable && fLocked) {
			fLocking.Unlock(fLockable);
			fLocked = false;
		}
	}

	inline operator bool() const	{ return fLocked; }

private:
	inline void _Lock()
	{
		if (fLockable)
			fLocked = fLocking.Lock(fLockable);
	}

private:
	Lockable	*fLockable;
	bool		fLocked;
	Locking		fLocking;
};

#endif	// AUTO_LOCKER_H
