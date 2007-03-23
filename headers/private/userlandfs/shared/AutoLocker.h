// AutoLocker.h
// 
// Copyright (c) 2004, Ingo Weinhold (bonefish@cs.tu-berlin.de)
// 
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
// 
// Except as contained in this notice, the name of a copyright holder shall
// not be used in advertising or otherwise to promote the sale, use or other
// dealings in this Software without prior written authorization of the
// copyright holder.

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

#endif	// AUTO_LOCKER_H
