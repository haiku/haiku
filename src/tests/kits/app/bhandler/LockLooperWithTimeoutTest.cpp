//------------------------------------------------------------------------------
//	LockLooperWithTimeoutTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LockLooperWithTimeoutTest.h"
#include "LockLooperTestCommon.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	LockLooperWithTimeout(bigtime_t timeout)
	@case			handler has no looper
	@param	timeout	10000 microseconds (not relevant)
	@results		Returns B_BAD_VALUE
 */
void TLockLooperWithTimeoutTest::LockLooperWithTimeout1()
{
	BHandler Handler;
	CPPUNIT_ASSERT(Handler.LockLooperWithTimeout(10000) == B_BAD_VALUE);
}
//------------------------------------------------------------------------------
/**
	LockLooperWithTimeout(bigtime_t timeout)
	@case			handler has a looper which is initially unlocked
	@param	timeout	10000 microseconds (not relevant)
	@results		Returns B_OK
 */
void TLockLooperWithTimeoutTest::LockLooperWithTimeout2()
{
	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	if (Looper.IsLocked())
	{
		// Make sure the looper is unlocked
		Looper.Unlock();
	}
	CPPUNIT_ASSERT(Handler.LockLooperWithTimeout(10000) == B_OK);
}
//------------------------------------------------------------------------------
/**
	LockLooperWithTimeout(bigtime_t timeout)
	@case			handler has a looper which is initially locked
	@param	timeout	10000 microseconds (not relevant)
	@results		Returns B_OK
 */
void TLockLooperWithTimeoutTest::LockLooperWithTimeout3()
{
	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	Looper.Lock();
	CPPUNIT_ASSERT(Handler.LockLooperWithTimeout(10000) == B_OK);
}
//------------------------------------------------------------------------------
/**
	LockLooperWithTimeout(bigtime_t timeout)
	@case			handler has a looper which is locked in another thread
	@param	timeout	10000 microseconds (not relevant)
	@results		Returns B_TIMED_OUT
 */
void TLockLooperWithTimeoutTest::LockLooperWithTimeout4()
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

	CPPUNIT_ASSERT(Handler.LockLooperWithTimeout(10000) == B_TIMED_OUT);
	info.UnlockThread();
}
//------------------------------------------------------------------------------
Test* TLockLooperWithTimeoutTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::LockLooperWithTimeout");

	ADD_TEST4(BHandler, SuiteOfTests, TLockLooperWithTimeoutTest, LockLooperWithTimeout1);
	ADD_TEST4(BHandler, SuiteOfTests, TLockLooperWithTimeoutTest, LockLooperWithTimeout2);
	ADD_TEST4(BHandler, SuiteOfTests, TLockLooperWithTimeoutTest, LockLooperWithTimeout3);
//	ADD_TEST4(BHandler, SuiteOfTests, TLockLooperWithTimeoutTest, LockLooperWithTimeout4);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


