/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

/*! \class LockerHelper
 	\brief Helper class for automatically releasing a lock.
	
	The LockerHelper acquires a lock on construction and releases it on destruction.
	This is a very useful class because you do not have to worry about
	releasing the lock at every possible point. It is done automatically.
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
		//!	Locks the given BLocker.
		LockerHelper(BLocker& lock) : fLock(&lock)
		{
			if(!fLock->Lock())
				fLock = NULL;
		}
		
		/*!	\brief Unlocks the BLocker that was passed to the constructor.
			
			If you called \c UnlockNow() the BLocker will not be unlocked.
		*/
		~LockerHelper()
		{
			if(fLock)
				fLock->Unlock();
		}
		
		/*!	\brief Unlocks the BLocker that was passed to the constructor.
			
			The destructor will not unlock the BLocker anymore and any subsequent
			calls to \c UnlockNow() will do \e nothing.
		*/
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
