//------------------------------------------------------------------------------
//	LockLooperTestCommon.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LockLooperTestCommon.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
int32 LockLooperThreadFunc(void* data)
{
	TLockLooperInfo* info = (TLockLooperInfo*)data;

	// Forces the test to encounter a pre-locked looper
	info->LockLooper();

	// Let the test proceed (finding the locked looper)
	info->UnlockTest();

	// Wait until the thread is dead
	info->LockThread();

	return 0;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


