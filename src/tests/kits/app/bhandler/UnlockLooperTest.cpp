//------------------------------------------------------------------------------
//	UnlockLooperTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "UnlockLooperTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	UnlockLooper()
	@case		handler has no looper
	@results	NONE
	@note		Original implementation apparently doesn't check to see if a
				looper actually exists before trying to call Unlock() on it.
				Disabled for TEST_R5.
 */
void TUnlockLooperTest::UnlockLooper1()
{
#if !defined(TEST_R5)
	BHandler Handler;
	Handler.UnlockLooper();
#endif
}
//------------------------------------------------------------------------------
/**
	UnlockLooper()
	@case		handler has a looper which is initially unlocked
	@results	debug message "looper must be locked before proceeding" from
				BLooper::AssertLock()
 */
void TUnlockLooperTest::UnlockLooper2()
{
	DEBUGGER_ESCAPE;

	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	if (Looper.IsLocked())
	{
		// Make sure the looper is unlocked
		Looper.Unlock();
	}
	Handler.UnlockLooper();
}
//------------------------------------------------------------------------------
/**
	UnlockLooper()
	@case		handler has a looper which is initially locked
	@results	NONE
 */
void TUnlockLooperTest::UnlockLooper3()
{
	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	if (!Looper.IsLocked())
	{
		Looper.Lock();
	}
	Handler.UnlockLooper();
}
//------------------------------------------------------------------------------
Test* TUnlockLooperTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::UnlockLooper");

	ADD_TEST4(BHandler, SuiteOfTests, TUnlockLooperTest, UnlockLooper1);
	ADD_TEST4(BHandler, SuiteOfTests, TUnlockLooperTest, UnlockLooper2);
	ADD_TEST4(BHandler, SuiteOfTests, TUnlockLooperTest, UnlockLooper3);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


