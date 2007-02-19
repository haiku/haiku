// Locking.h

#ifndef LOCKING_H
#define LOCKING_H

class Volume;

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
	inline AutoLocker(Lockable *lockable)
		: fLockable(lockable),
		  fLocked(false)
	{
		_Lock();
	}

	inline AutoLocker(Lockable &lockable)
		: fLockable(&lockable),
		  fLocked(false)
	{
		_Lock();
	}

	inline ~AutoLocker()
	{
		_Unlock();
	}

	inline AutoLocker<Lockable, Locking> &operator=(Lockable *lockable)
	{
		_Unlock();
		fLockable = lockable;
		_Lock();
	}

	inline AutoLocker<Lockable, Locking> &operator=(Lockable &lockable)
	{
		_Unlock();
		fLockable = &lockable;
		_Lock();
	}

	inline bool IsLocked() const	{ return fLocked; }

	inline operator bool() const	{ return fLocked; }

private:
	inline void _Lock()
	{
		if (fLockable)
			fLocked = fLocking.Lock(fLockable);
	}

	inline void _Unlock()
	{
		if (fLockable && fLocked) {
			fLocking.Unlock(fLockable);
			fLocked = false;
		}
	}

private:
	Lockable	*fLockable;
	bool		fLocked;
	Locking		fLocking;
};

// instantiations
typedef AutoLocker<Volume, AutoLockerReadLocking<Volume> >	VolumeReadLocker;
typedef AutoLocker<Volume, AutoLockerWriteLocking<Volume> >	VolumeWriteLocker;

#endif LOCKING_H
