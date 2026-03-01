/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		tylerdauwalder
 */


#include <Locker.h>

#include <TestSuiteAddon.h>
#include <ThreadedTestCaller.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>

#include <string.h>



static const int32 MAX_LOOP = 10000;
static const bigtime_t SNOOZE_TIME = 200000;


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
 * \brief Test class for testing BLocker functionality.
 *
 * It tests use cases "Locking 1", "Locking 2", "Unlocking", "Is Locked",
 * "Locking Thread" and "Count Locks".
 */
class LockerConcurrencyTest : public BThreadedTestCase {
public:
								LockerConcurrencyTest(std::string name, bool benaphoreFlag);
	virtual						~LockerConcurrencyTest();

	virtual void				setUp();

			void				SimpleLockingLoop();
			void				AcquireThread();
			void				TimeoutThread();

	static	CppUnit::Test*		suite();

private:
			void				CheckLock(int expectedCount);
			bool				AcquireLock(int lockAttempt, bool firstAcquisition);

			BLocker*			fLocker;
			bool				fLockTestValue;
};


LockerConcurrencyTest::LockerConcurrencyTest(std::string name, bool benaphoreFlag)
	:
	BThreadedTestCase(name),
	fLocker(new BLocker(benaphoreFlag)),
	fLockTestValue(false)
{
}


LockerConcurrencyTest::~LockerConcurrencyTest()
{
	delete fLocker;
}


void
LockerConcurrencyTest::CheckLock(int expectedCount)
{
	CPPUNIT_ASSERT(fLocker->CountLockRequests() == expectedCount);
}


void
LockerConcurrencyTest::setUp()
{
	fLockTestValue = false;
}


bool
LockerConcurrencyTest::AcquireLock(int lockAttempt, bool firstAcquisition)
{
	bool timeoutLock;
	if (firstAcquisition)
		timeoutLock = ((lockAttempt % 2) == 1);
	else
		timeoutLock = (((lockAttempt / 2) % 2) == 1);

	if (timeoutLock)
		return fLocker->LockWithTimeout(1000000) == B_OK;

	return fLocker->Lock();
}


/**
 * This method is the core of the test.  Each of the three threads
 * run this method to perform the concurrency test.  First, the
 * SafetyLock class (see LockerTestCase.h) is used to make sure that
 * the lock is released if an assertion happens.  Then, each thread
 * iterates MAXLOOP times through the main loop where the following
 * actions are performed:
 *  - CheckLock() is used to show that the thread does not have
 *    the lock.
 *  - The thread acquires the lock.
 *  - The thread confirms that mutual exclusion is OK by testing
 *    lockTestValue.
 *  - The thread confirms the lock is held once by the thread.
 *  - The thread acquires the lock again.
 *  - The thread confirms the lock is held twice now by the thread.
 *  - The thread releases the lock once.
 *  - The thread confirms the lock is held once now.
 *  - The thread confirms that mutual exclusion is still OK by
 *    testing lockTestValue.
 *  - The thread releases the lock again.
 *  - The thread confirms that the lock is no longer held.
 */
void
LockerConcurrencyTest::SimpleLockingLoop()
{
	SafetyLock theSafetyLock(fLocker);

	for (int i = 0; i < MAX_LOOP; i++) {
		CheckLock(0);
		CPPUNIT_ASSERT(AcquireLock(i, true));

		CPPUNIT_ASSERT(!fLockTestValue);
		fLockTestValue = true;
		CheckLock(1);

		CPPUNIT_ASSERT(AcquireLock(i, false));
		CheckLock(2);

		fLocker->Unlock();
		CheckLock(1);

		CPPUNIT_ASSERT(fLockTestValue);
		fLockTestValue = false;
		fLocker->Unlock();
		CheckLock(0);
	}
}


/**
 * This member function acquires the lock, sleeps for SNOOZE_TIME,
 * releases the lock and then launches into the lock loop test.
 */
void
LockerConcurrencyTest::AcquireThread()
{
	SafetyLock theSafetyLock(fLocker);
	CPPUNIT_ASSERT(fLocker->Lock());
	snooze(SNOOZE_TIME);
	fLocker->Unlock();

	SimpleLockingLoop();
}


/**
 * This member function sleeps for a short time and then attempts to
 * acquire the lock for SNOOZE_TIME/10 seconds.  This acquisition
 * should timeout.  Then the locking loop is started.
 */
void
LockerConcurrencyTest::TimeoutThread()
{
	SafetyLock theSafetyLock(fLocker);
	snooze(SNOOZE_TIME / 2);
	CPPUNIT_ASSERT(fLocker->LockWithTimeout(SNOOZE_TIME / 10) == B_TIMED_OUT);

	SimpleLockingLoop();
}


CppUnit::Test*
LockerConcurrencyTest::suite()
{
	typedef BThreadedTestCaller<LockerConcurrencyTest> LockerConcurrencyTestCaller;
	CppUnit::TestSuite* testSuite = new CppUnit::TestSuite("LockerConcurrencyTest");

	// Benaphore
	LockerConcurrencyTest* simpleBenaphore = new LockerConcurrencyTest("SimpleBenaphore", true);
	LockerConcurrencyTestCaller* simpleBenaphoreCaller
		= new LockerConcurrencyTestCaller("BLocker::Concurrency Test #1 (benaphore)", simpleBenaphore);
	simpleBenaphoreCaller->addThread("A", &LockerConcurrencyTest::SimpleLockingLoop);
	simpleBenaphoreCaller->addThread("B", &LockerConcurrencyTest::SimpleLockingLoop);
	simpleBenaphoreCaller->addThread("C", &LockerConcurrencyTest::SimpleLockingLoop);
	testSuite->addTest(simpleBenaphoreCaller);

	// Semaphore
	LockerConcurrencyTest* simpleSemaphore = new LockerConcurrencyTest("SimpleSemaphore", false);
	LockerConcurrencyTestCaller* simpleSemaphoreCaller
		= new LockerConcurrencyTestCaller("BLocker::Concurrency Test #1 (semaphore)", simpleSemaphore);
	simpleSemaphoreCaller->addThread("A", &LockerConcurrencyTest::SimpleLockingLoop);
	simpleSemaphoreCaller->addThread("B", &LockerConcurrencyTest::SimpleLockingLoop);
	simpleSemaphoreCaller->addThread("C", &LockerConcurrencyTest::SimpleLockingLoop);
	testSuite->addTest(simpleSemaphoreCaller);

	// Benaphore
	LockerConcurrencyTest* timeoutBenaphore = new LockerConcurrencyTest("TimeoutBenaphore", true);
	LockerConcurrencyTestCaller* timeoutBenaphoreCaller
		= new LockerConcurrencyTestCaller("BLocker::Concurrency Test #2 (benaphore)", timeoutBenaphore);
	timeoutBenaphoreCaller->addThread("Acquire", &LockerConcurrencyTest::AcquireThread);
	timeoutBenaphoreCaller->addThread("Timeout1", &LockerConcurrencyTest::TimeoutThread);
	timeoutBenaphoreCaller->addThread("Timeout2", &LockerConcurrencyTest::TimeoutThread);
	testSuite->addTest(timeoutBenaphoreCaller);

	// Semaphore
	LockerConcurrencyTest* timeoutSemaphore = new LockerConcurrencyTest("TimeoutSemaphore", false);
	LockerConcurrencyTestCaller* timeoutSemaphoreCaller
		= new LockerConcurrencyTestCaller("BLocker::Concurrency Test #2 (semaphore)", timeoutSemaphore);
	timeoutSemaphoreCaller->addThread("Acquire", &LockerConcurrencyTest::AcquireThread);
	timeoutSemaphoreCaller->addThread("Timeout1", &LockerConcurrencyTest::TimeoutThread);
	timeoutSemaphoreCaller->addThread("Timeout2", &LockerConcurrencyTest::TimeoutThread);
	testSuite->addTest(timeoutSemaphoreCaller);

	return testSuite;
}


//CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(LockerConcurrencyTest, getTestSuiteName());
