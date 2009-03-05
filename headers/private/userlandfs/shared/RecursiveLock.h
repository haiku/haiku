/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_USERLAND_FS_RECURSIVE_LOCK_H
#define	_USERLAND_FS_RECURSIVE_LOCK_H

#include <lock.h> // private/kernel


namespace UserlandFSUtil {


class RecursiveLock {
public:
	RecursiveLock(bool benaphore_style = true)
	{
		recursive_lock_init(&fLock, "anonymous locker");
	}

	RecursiveLock(const char *name, bool benaphore_style = true)
	{
		recursive_lock_init_etc(&fLock, name, MUTEX_FLAG_CLONE_NAME);
	}

	~RecursiveLock()
	{
		recursive_lock_destroy(&fLock);
	}

	status_t InitCheck() const
	{
		return B_OK;
	}

	bool Lock(void)
	{
		return recursive_lock_lock(&fLock) == B_OK;
	}

	void Unlock(void)
	{
		recursive_lock_unlock(&fLock);
	}

	thread_id LockingThread(void) const
	{
		return RECURSIVE_LOCK_HOLDER(&fLock);
	}

	bool IsLocked(void) const
	{
		return RECURSIVE_LOCK_HOLDER(&fLock) == find_thread(NULL);
	}

	int32 CountLocks(void) const
	{
		int32 count = recursive_lock_get_recursion(
			const_cast<recursive_lock*>(&fLock));
		return count >= 0 ? count : 0;
	}

private:
	recursive_lock	fLock;
};


};	// namespace UserlandFSUtil

using UserlandFSUtil::RecursiveLock;

#endif	// _USERLAND_FS_RECURSIVE_LOCK_H
