//------------------------------------------------------------------------------
//	LockLooperTestCommon.h
//
//------------------------------------------------------------------------------

#ifndef LOCKLOOPERTESTCOMMON_H
#define LOCKLOOPERTESTCOMMON_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <OS.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BLooper;

class TLockLooperInfo
{
	public:
	TLockLooperInfo(BLooper* Looper) : fLooper(Looper)
	{
		// Create it "acquired"
		fThreadLock = create_sem(0, NULL);
		fTestLock = create_sem(0, NULL);
	}

	void LockTest()		{ acquire_sem(fTestLock); }
	void UnlockTest()	{ release_sem(fTestLock); }
	void LockThread()	{ acquire_sem(fThreadLock); }
	void UnlockThread()	{ release_sem(fThreadLock); }
	void UnlockLooper()	{ fLooper->Unlock(); }
	void LockLooper()
	{
		fLooper->Lock();
	}

	private:
		BLooper*	fLooper;
		sem_id		fTestLock;
		sem_id		fThreadLock;
};

int32 LockLooperThreadFunc(void* data);

#endif	//LOCKLOOPERTESTCOMMON_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

