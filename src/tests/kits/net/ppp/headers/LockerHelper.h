/*
 * LockerHelper.h
 *
 * The LockerHelper acquires a lock on construction and releases it
 * on destruction.
 * This is a very useful class because you do not have to worry about
 * releasing the lock at every possible point. It is done automatically.
 *
 */


#ifndef _LOCKER_HELPER__H
#define _LOCKER_HELPER__H

#include "Locker.h"


class LockerHelper {
	public:
		LockerHelper(BLocker &lock) : fLock(&lock)
		{
			if(fLock->Lock() != B_OK)
				fLock = NULL;
		}
		
		~LockerHelper()
		{
			if(fLock)
				fLock->Unlock();
		}
		
		void UnlockNow()
		{
			if(fLock)
				fLock->Unlock();
			fLock = NULL;
		}

	private:
		BLocker *fLock;
};


#endif
