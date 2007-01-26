/*
 * Copyright 2001-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_LOCKER_H
#define	_LOCKER_H


#include <OS.h>


class BLocker {
	public:
		BLocker();
		BLocker(const char* name);
		BLocker(bool benaphoreStyle);
		BLocker(const char* name, bool benaphoreStyle);
		virtual ~BLocker();

		bool Lock(void);
		status_t LockWithTimeout(bigtime_t timeout);
		void Unlock(void);

		thread_id LockingThread(void) const;
		bool IsLocked(void) const;
		int32 CountLocks(void) const;
		int32 CountLockRequests(void) const;
		sem_id Sem(void) const;

	private:
		BLocker(const char *name, bool benaphoreStyle, bool);
		void InitLocker(const char *name, bool benaphore_style);
		bool AcquireLock(bigtime_t timeout, status_t *error);

		int32	fBenaphoreCount;
		sem_id	fSemaphoreID;
		thread_id fLockOwner;
		int32	fRecursiveCount;

		int32 _reserved[4];
};

#endif // _LOCKER_H
