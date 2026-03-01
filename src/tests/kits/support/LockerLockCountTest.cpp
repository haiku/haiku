/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		tylerdauwalder
 */

/**
 * This file implements a test class for testing BLocker functionality.
 * It tests use cases "Count Lock Requests" for a benaphore style BLocker.
 *
 * The test works by:
 * 	- checking the lock requests
 * 	- acquiring the lock
 * 	- checking the lock requests
 * 	- staring a thread which times out acquiring the lock and then blocks
 * 	  again waiting for the lock
 * 	- checking the lock requests
 * 	- start a second thread which times out acquiring the lock and then blocks
 * 	  again waiting for the lock
 * 	- checking the lock requests
 * 	- release the lock
 * 	- each blocked thread acquires the lock, checks the lock requests and releases
 * 	  the lock before terminating
 * 	- the main thread checks the lock requests one last time
 */


#include <Locker.h>

#include <TestSuiteAddon.h>
#include <ThreadedTestCaller.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>

#include <string.h>


static const bigtime_t SNOOZE_TIME = 100000;


/**
 * \brief Utility class to ensure a BLocker is released on destruction.
 */
class SafetyLock {
public:
	SafetyLock(BLocker* lock) : fLocker(lock) {}
	~SafetyLock() { if (fLocker != NULL) fLocker->Unlock(); }

private:
	BLocker* fLocker;
};


/**
 * \brief Tests BLocker's CountLockRequests() functionality.
 */
class LockerLockCountTest : public BThreadedTestCase {
public:
								LockerLockCountTest(std::string name, bool isBenaphore);
	virtual						~LockerLockCountTest();

			void				TestThread1();
			void				TestThread2();
			void				TestThread3();

	static	CppUnit::Test*		suite();

private:
			bool				CheckLockRequests(int expected);

			BLocker*			fLocker;
			BLocker				fThread2Lock;
			BLocker				fThread3Lock;
			bool				fIsBenaphore;
};


LockerLockCountTest::LockerLockCountTest(std::string name, bool isBenaphore)
	:
	BThreadedTestCase(name),
	fLocker(new BLocker(isBenaphore)),
	fThread2Lock("thread2Lock"),
	fThread3Lock("thread3Lock"),
	fIsBenaphore(isBenaphore)
{
}


LockerLockCountTest::~LockerLockCountTest()
{
	delete fLocker;
}


bool
LockerLockCountTest::CheckLockRequests(int expected)
{
	return fLocker->CountLockRequests() == expected;
}


/**
 * \brief Main portion of the test
 *
 * This member function performs the main portion of the test.
 * It first acquires thread2Lock and thread3Lock.  This ensures
 * that thread2 and thread3 will block until this thread wants
 * them to start running.  It then checks the lock count, acquires
 * the lock and checks the lock count again.  It unlocks each
 * of the other two threads in turn and rechecks the lock count.
 * Finally, it releases the lock and sleeps for a short while
 * for the other two threads to finish.  At the end, it checks
 * the lock count on final time.
 */
void
LockerLockCountTest::TestThread1()
{
	SafetyLock theSafetyLock1(fLocker);
	SafetyLock theSafetyLock2(&fThread2Lock);
	SafetyLock theSafetyLock3(&fThread3Lock);

	CPPUNIT_ASSERT(fThread2Lock.Lock());
	CPPUNIT_ASSERT(fThread3Lock.Lock());

	// Initial request count varies by benaphore/semaphore
	CPPUNIT_ASSERT(CheckLockRequests(fIsBenaphore ? 0 : 1));
	CPPUNIT_ASSERT(fLocker->Lock());
	CPPUNIT_ASSERT(CheckLockRequests(fIsBenaphore ? 1 : 2));

	fThread2Lock.Unlock();
	snooze(SNOOZE_TIME);
	CPPUNIT_ASSERT(CheckLockRequests(fIsBenaphore ? 3 : 4));

	fThread3Lock.Unlock();
	snooze(SNOOZE_TIME);
	CPPUNIT_ASSERT(CheckLockRequests(fIsBenaphore ? 5 : 6));

	fLocker->Unlock();
	snooze(SNOOZE_TIME);
	CPPUNIT_ASSERT(CheckLockRequests(fIsBenaphore ? 2 : 3));
}


/**
 * \brief Second thread of the test
 *
 * This member function defines the actions of the second thread of
 * the test.  First it sleeps for a short while and then blocks on
 * the thread2Lock.  When the first thread releases it, this thread
 * begins its testing.  It times out attempting to acquire the main
 * lock and then blocks to acquire the lock.  Once that lock is
 * acquired, the lock count is checked before finishing this thread.
 */
void
LockerLockCountTest::TestThread2()
{
	SafetyLock theSafetyLock1(fLocker);

	snooze(SNOOZE_TIME / 10);
	CPPUNIT_ASSERT(fThread2Lock.Lock());

	CPPUNIT_ASSERT(fLocker->LockWithTimeout(SNOOZE_TIME / 10) == B_TIMED_OUT);
	CPPUNIT_ASSERT(fLocker->Lock());

	int actual = fLocker->CountLockRequests();
	if (fIsBenaphore)
		CPPUNIT_ASSERT(actual == 3 || actual == 4);
	else
		CPPUNIT_ASSERT(actual == 4 || actual == 5);

	fLocker->Unlock();
}


/**
 * \brief Third thread of the test
 *
 * This member function defines the actions of the third thread of
 * the test.  First it sleeps for a short while and then blocks on
 * the thread3Lock.  When the first thread releases it, this thread
 * begins its testing.  It times out attempting to acquire the main
 * lock and then blocks to acquire the lock.  Once that lock is
 * acquired, the lock count is checked before finishing this thread.
 */
void
LockerLockCountTest::TestThread3()
{
	SafetyLock theSafetyLock1(fLocker);

	snooze(SNOOZE_TIME / 10);
	CPPUNIT_ASSERT(fThread3Lock.Lock());

	CPPUNIT_ASSERT(fLocker->LockWithTimeout(SNOOZE_TIME / 10) == B_TIMED_OUT);
	CPPUNIT_ASSERT(fLocker->Lock());

	int actual = fLocker->CountLockRequests();
	if (fIsBenaphore)
		CPPUNIT_ASSERT(actual == 3 || actual == 4);
	else
		CPPUNIT_ASSERT(actual == 4 || actual == 5);

	fLocker->Unlock();
}


CppUnit::Test*
LockerLockCountTest::suite()
{
	typedef BThreadedTestCaller<LockerLockCountTest> LockerLockCountTestCaller;
	CppUnit::TestSuite* testSuite = new CppUnit::TestSuite("LockerLockCountTest");

	// Benaphore test
	LockerLockCountTest* benaphoreTest = new LockerLockCountTest("Benaphore", true);
	LockerLockCountTestCaller* caller1
		= new LockerLockCountTestCaller("BLocker::Benaphore Lock Count Test", benaphoreTest);
	caller1->addThread("A", &LockerLockCountTest::TestThread1);
	caller1->addThread("B", &LockerLockCountTest::TestThread2);
	caller1->addThread("C", &LockerLockCountTest::TestThread3);
	testSuite->addTest(caller1);

	// Semaphore test
	LockerLockCountTest* semaphoreTest = new LockerLockCountTest("Semaphore", false);
	LockerLockCountTestCaller* caller2
		= new LockerLockCountTestCaller("BLocker::Semaphore Lock Count Test", semaphoreTest);
	caller2->addThread("A", &LockerLockCountTest::TestThread1);
	caller2->addThread("B", &LockerLockCountTest::TestThread2);
	caller2->addThread("C", &LockerLockCountTest::TestThread3);
	testSuite->addTest(caller2);

	return testSuite;
}


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(LockerLockCountTest, getTestSuiteName());
