//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

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

#include <Locker.h>


class LockerHelper {
	private:
		// copies are not allowed!
		LockerHelper(const LockerHelper& copy);
		LockerHelper& operator= (const LockerHelper& copy);

	public:
		LockerHelper(BLocker& lock) : fLock(&lock)
		{
			if(!fLock->Lock())
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
