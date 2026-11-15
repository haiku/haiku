/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Handler.h>
#include <Looper.h>

#include "common.h"

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class TLockLooperInfo
{
public:
	TLockLooperInfo(BLooper* Looper) : fLooper(Looper)
	{
		// Create it "acquired"
		fThreadLock = create_sem(0, "TLockLooperInfo fThreadLock");
		fTestLock = create_sem(0, "TLockLooperInfo fTestLock");
	}

	void LockTest()		{ acquire_sem(fTestLock); }
	void UnlockTest()	{ release_sem(fTestLock); }
	void LockThread()	{ acquire_sem(fThreadLock); }
	void UnlockThread()	{ release_sem(fThreadLock); }
	void UnlockLooper()	{ fLooper->Unlock(); }
	void LockLooper()	{ fLooper->Lock(); }

private:
	BLooper*	fLooper;
	sem_id		fTestLock;
	sem_id		fThreadLock;
};


static int32
LockLooperThreadFunc(void* data)
{
	TLockLooperInfo* info = (TLockLooperInfo*)data;

	// Forces the test to encounter a pre-locked looper
	info->LockLooper();

	// Let the test proceed (finding the locked looper)
	info->UnlockTest();

	// Wait until the thread is dead
	info->LockThread();

	// Unlock looper so BHandler destructor can proceed
	info->UnlockLooper();

	return 0;
}


class HandlerLooperTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(HandlerLooperTest);
	CPPUNIT_TEST(NoLooper_Looper_ReturnsNull);
	CPPUNIT_TEST(LooperAddedThenRemoved_Looper_ReturnsCorrectValues);

	CPPUNIT_TEST(NoLooper_LockLooper_ReturnsFalse);
	CPPUNIT_TEST(UnlockedLooper_LockLooper_ReturnsTrue);
	CPPUNIT_TEST(LockedLooper_LockLooper_ReturnsTrue);
	// CPPUNIT_TEST(LockedAnotherThread_LockLooper_ReturnsFalse);
	CPPUNIT_TEST(NoLooper_LockLooperWithTimeout_ReturnsBadValue);
	CPPUNIT_TEST(UnlockedLooper_LockLooperWithTimeout_ReturnsOk);
	CPPUNIT_TEST(LockedLooper_LockLooperWithTimeout_ReturnsOk);
	CPPUNIT_TEST(LockedAnotherThread_LockLooperWithTimeout_ReturnsTimedOut);

	CPPUNIT_TEST(NoLooper_UnlockLooper_Succeeds);
	CPPUNIT_TEST(InitiallyUnlocked_UnlockLooper_Succeeds);
	CPPUNIT_TEST(InitiallyLocked_UnlockLooper_Succeeds);
	CPPUNIT_TEST_SUITE_END();

	BHandler fHandler;

public:
	void NoLooper_Looper_ReturnsNull()
	{
		CPPUNIT_ASSERT(fHandler.Looper() == NULL);
	}

	void LooperAddedThenRemoved_Looper_ReturnsCorrectValues()
	{
		BLooper Looper;
		Looper.AddHandler(&fHandler);
		CPPUNIT_ASSERT(fHandler.Looper() == &Looper);

		CPPUNIT_ASSERT(Looper.RemoveHandler(&fHandler));
		CPPUNIT_ASSERT(fHandler.Looper() == NULL);
	}

	void NoLooper_LockLooper_ReturnsFalse()
	{
		BHandler Handler;
		CPPUNIT_ASSERT(!Handler.LockLooper());
	}

	void UnlockedLooper_LockLooper_ReturnsTrue()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		if (Looper.IsLocked())
			Looper.Unlock();

		CPPUNIT_ASSERT(Handler.LockLooper());
	}

	void LockedLooper_LockLooper_ReturnsTrue()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		Looper.Lock();
		CPPUNIT_ASSERT(Handler.LockLooper());
	}

	// Deadlocks, this can't possibly work 
	// (LockLooper will wait instead of returning false)
	// TODO: investigate if it should be removed or fixed
	void LockedAnotherThread_LockLooper_ReturnsFalse()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		if (Looper.IsLocked())
			Looper.Unlock();

		TLockLooperInfo info(&Looper);
		thread_id tid = spawn_thread(LockLooperThreadFunc, "LockLooperHelperThread",
			B_NORMAL_PRIORITY, (void*)&info);
		resume_thread(tid);
		info.LockTest();

		CPPUNIT_ASSERT(!Handler.LockLooper());
		info.UnlockThread();
	}

	void NoLooper_LockLooperWithTimeout_ReturnsBadValue()
	{
		BHandler Handler;
		CPPUNIT_ASSERT_EQUAL(B_BAD_VALUE, Handler.LockLooperWithTimeout(10000));
	}

	void UnlockedLooper_LockLooperWithTimeout_ReturnsOk()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		if (Looper.IsLocked())
			Looper.Unlock();

		CPPUNIT_ASSERT_EQUAL(B_OK, Handler.LockLooperWithTimeout(10000));
	}

	void LockedLooper_LockLooperWithTimeout_ReturnsOk()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		Looper.Lock();
		CPPUNIT_ASSERT_EQUAL(B_OK, Handler.LockLooperWithTimeout(10000));
	}

	void LockedAnotherThread_LockLooperWithTimeout_ReturnsTimedOut()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		if (Looper.IsLocked())
			Looper.Unlock();

		TLockLooperInfo info(&Looper);
		thread_id tid = spawn_thread(LockLooperThreadFunc, "LockLooperHelperThread",
									 B_NORMAL_PRIORITY, (void*)&info);
		resume_thread(tid);
		info.LockTest();

		CPPUNIT_ASSERT_EQUAL(B_TIMED_OUT, Handler.LockLooperWithTimeout(10000));
		info.UnlockThread();
	}

	/**
		UnlockLooper()
		@case		handler has no looper
		@results	NONE
		@note		Original implementation apparently doesn't check to see if a
					looper actually exists before trying to call Unlock() on it.
					Disabled for TEST_R5.
	 */
	void NoLooper_UnlockLooper_Succeeds()
	{
	#ifndef TEST_R5
		DEBUGGER_ESCAPE;

		BHandler Handler;
		Handler.UnlockLooper();
	#endif
	}

	/**
		UnlockLooper()
		@case		handler has a looper which is initially unlocked
		@results	debug message "looper must be locked before proceeding" from
					BLooper::AssertLock()
	 */
	void InitiallyUnlocked_UnlockLooper_Succeeds()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		if (Looper.IsLocked())
			Looper.Unlock();
		Handler.UnlockLooper();
	}

	/**
		UnlockLooper()
		@case		handler has a looper which is initially locked
		@results	NONE
	 */
	void InitiallyLocked_UnlockLooper_Succeeds()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		if (!Looper.IsLocked())
			Looper.Lock();

		Handler.UnlockLooper();
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(HandlerLooperTest, getTestSuiteName());
