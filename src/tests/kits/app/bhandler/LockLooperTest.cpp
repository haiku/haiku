//------------------------------------------------------------------------------
//	LockLooper.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LockLooperTest.h"
#include "LockLooperTestCommon.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	LockLooper();
	@case		handler has no looper
	@results	Returns false
 */
void TLockLooperTest::LockLooper1()
{
	BHandler Handler;
	CPPUNIT_ASSERT(!Handler.LockLooper());
}
//------------------------------------------------------------------------------
/**
	LockLooper();
	@case		handler has a looper which is initially unlocked
	@results	Returns true
 */
void TLockLooperTest::LockLooper2()
{
	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	if (Looper.IsLocked())
	{
		// Make sure the looper is unlocked
		Looper.Unlock();
	}
	CPPUNIT_ASSERT(Handler.LockLooper());
}
//------------------------------------------------------------------------------
/**
	LockLooper();
	@case		handler has a looper which is initially locked
	@results	Returns true
 */
void TLockLooperTest::LockLooper3()
{
	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	Looper.Lock();
	CPPUNIT_ASSERT(Handler.LockLooper());
}
//------------------------------------------------------------------------------
/**
	LockLooper();
	@case		handler has a looper which is locked in another thread
	@results	Returns false
 */
void TLockLooperTest::LockLooper4()
{
	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	if (Looper.IsLocked())
	{
		Looper.Unlock();
	}

	TLockLooperInfo info(&Looper);
	thread_id tid = spawn_thread(LockLooperThreadFunc, "LockLooperHelperThread",
								 B_NORMAL_PRIORITY, (void*)&info);
	resume_thread(tid);
	info.LockTest();

	CPPUNIT_ASSERT(!Handler.LockLooper());
	info.UnlockThread();
}
//------------------------------------------------------------------------------
Test* TLockLooperTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::LockLooper");

	ADD_TEST4(BHandler, SuiteOfTests, TLockLooperTest, LockLooper1);
	ADD_TEST4(BHandler, SuiteOfTests, TLockLooperTest, LockLooper2);
	ADD_TEST4(BHandler, SuiteOfTests, TLockLooperTest, LockLooper3);
//	ADD_TEST4(BHandler, SuiteOfTests, TLockLooperTest, LockLooper4);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


