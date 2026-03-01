/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		tylerdauwalder
 */


#include <Autolock.h>
#include <Looper.h>
#include <OS.h>

#include <TestSuiteAddon.h>
#include <ThreadedTestCase.h>
#include <ThreadedTestCaller.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>


static const bigtime_t SNOOZE_TIME = 250000;


class AutolockLockerTest : public BThreadedTestCase {
public:
								AutolockLockerTest(std::string name);
	virtual						~AutolockLockerTest();

			void				Lock_Locker_MatchesThread();
			void				Construct_AutolockPtr_LocksLocker();
			void				Construct_AutolockRef_LocksLocker();

	static	CppUnit::Test*		suite();

private:
			BLocker*			fLocker;
};


AutolockLockerTest::AutolockLockerTest(std::string name)
	:
	BThreadedTestCase(name),
	fLocker(new BLocker)
{
}


AutolockLockerTest::~AutolockLockerTest()
{
	delete fLocker;
}


/**
 * This method performs the tests on the Autolock.  It first acquires the
 * lock and sleeps for a short time.  It deletes the lock rather than Unlock()
 * it in order to test the other two threads.  Then, it constructs a new
 * Locker and Autolock and checks that both the Autolock and the Locker are
 * both locked.  Then, the Autolock is released by deleting it.  The Locker
 * is checked to see that it is now released.  This is then repeated for an Autolock
 * constructed by passing a reference to the Locker.
 */
void
AutolockLockerTest::Lock_Locker_MatchesThread()
{
	CPPUNIT_ASSERT(fLocker->Lock());
	CPPUNIT_ASSERT(fLocker->LockingThread() == find_thread(NULL));
	snooze(SNOOZE_TIME);

	// Deleting the locker while others might be waiting is a destructive test
	delete fLocker;

	fLocker = new BLocker;

	{
		BAutolock autolock(fLocker);
		CPPUNIT_ASSERT(fLocker->IsLocked());
		CPPUNIT_ASSERT(fLocker->LockingThread() == find_thread(NULL));
		CPPUNIT_ASSERT(autolock.IsLocked());
	}
	CPPUNIT_ASSERT(fLocker->LockingThread() != find_thread(NULL));

	{
		BAutolock autolock(*fLocker);
		CPPUNIT_ASSERT(fLocker->IsLocked());
		CPPUNIT_ASSERT(fLocker->LockingThread() == find_thread(NULL));
		CPPUNIT_ASSERT(autolock.IsLocked());
	}
	CPPUNIT_ASSERT(fLocker->LockingThread() != find_thread(NULL));
}


/**
 * This method performs the tests on the Autolock.  It first sleeps for a short
 * time and then tries to acquire the lock with an Autolock.  It passes a pointer
 * to the lock to the Autolock.  It expects the acquisition to fail and IsLocked()
 * is tested to be sure.
 */
void
AutolockLockerTest::Construct_AutolockPtr_LocksLocker()
{
	snooze(SNOOZE_TIME / 10);
	BAutolock autolock(fLocker);
	CPPUNIT_ASSERT(!autolock.IsLocked());
}


/**
 * This method performs the tests on the Autolock.  It first sleeps for a short
 * time and then tries to acquire the lock with an Autolock.  It passes a reference
 * to the lock to the Autolock.  It expects the acquisition to fail and IsLocked()
 * is tested to be sure.
 */
void
AutolockLockerTest::Construct_AutolockRef_LocksLocker()
{
	snooze(SNOOZE_TIME / 10);
	BAutolock autolock(*fLocker);
	CPPUNIT_ASSERT(!autolock.IsLocked());
}


CppUnit::Test*
AutolockLockerTest::suite()
{
	typedef BThreadedTestCaller<AutolockLockerTest> AutolockLockerTestCaller;

	AutolockLockerTest* theTest = new AutolockLockerTest("");
	AutolockLockerTestCaller* threadedTest
		= new AutolockLockerTestCaller("BAutolock::Locker Test", theTest);

	threadedTest->addThread("A", &AutolockLockerTest::Lock_Locker_MatchesThread);
	threadedTest->addThread("B", &AutolockLockerTest::Construct_AutolockPtr_LocksLocker);
	threadedTest->addThread("C", &AutolockLockerTest::Construct_AutolockRef_LocksLocker);

	return threadedTest;
}


class AutolockLooperTest : public BThreadedTestCase {
public:
								AutolockLooperTest(std::string name);
	virtual						~AutolockLooperTest();

			void				Construct_AutolockPtr_LocksLooper();

	static	CppUnit::Test*		suite();

private:
			BLooper*			fLooper;
};


AutolockLooperTest::AutolockLooperTest(std::string name)
	:
	BThreadedTestCase(name),
	fLooper(new BLooper)
{
	fLooper->Run();
}


AutolockLooperTest::~AutolockLooperTest()
{
	if (fLooper != NULL) {
		fLooper->Lock();
		fLooper->Quit();
	}
}


/**
 * This method performs the tests on the Autolock.  It constructs a new
 * Autolock and checks that both the Autolock and the Looper are
 * both locked.  Then, the Autolock is released by deleting it.  The Looper
 * is checked to see that it is now released.
 */
void
AutolockLooperTest::Construct_AutolockPtr_LocksLooper()
{
	BAutolock* autolock = new BAutolock(fLooper);

	CPPUNIT_ASSERT(fLooper->IsLocked());
	CPPUNIT_ASSERT(fLooper->LockingThread() == find_thread(NULL));
	CPPUNIT_ASSERT(autolock->IsLocked());

	delete autolock;
	CPPUNIT_ASSERT(fLooper->LockingThread() != find_thread(NULL));
}


CppUnit::Test*
AutolockLooperTest::suite()
{
	typedef BThreadedTestCaller<AutolockLooperTest> AutolockLooperTestCaller;

	AutolockLooperTest* theTest = new AutolockLooperTest("");
	AutolockLooperTestCaller* threadedTest
		= new AutolockLooperTestCaller("BAutolock::Looper Test", theTest);

	threadedTest->addThread("A", &AutolockLooperTest::Construct_AutolockPtr_LocksLooper);

	return threadedTest;
}


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(AutolockLockerTest, getTestSuiteName());
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(AutolockLooperTest, getTestSuiteName());
