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


static const bigtime_t SNOOZE_TIME = 200000;


/**
 * \brief Class for testing BLocker functionality.
 * It tests use cases "Destruction", "Locking 3" and "Locking 4".
 */
class LockerDestructionTest : public BThreadedTestCase {
public:
								LockerDestructionTest(std::string name, bool isBenaphore);
	virtual						~LockerDestructionTest();

			void				SimpleWaiterThread();
			void				SimpleDeleterThread();

			void				TimeoutWaiterThread();
			void				TimeoutDeleterThread();

	static	CppUnit::Test*		suite();

private:
			BLocker*			fLocker;
};


LockerDestructionTest::LockerDestructionTest(std::string name, bool isBenaphore)
	:
	BThreadedTestCase(name),
	fLocker(new BLocker(isBenaphore))
{
}


LockerDestructionTest::~LockerDestructionTest()
{
	if (fLocker != NULL)
		delete fLocker;
}


/**
 * \brief Simple destruction tests
 *
 * The test works like the following:
 * 	- the main thread acquires the lock
 * 	- the second thread sleeps
 * 	- the second thread then attempts to acquire the lock
 * 	- the first thread releases the lock
 * 	- at this time, the new thread acquires the lock and goes to sleep
 * 	- the first thread attempts to acquire the lock
 * 	- the second thread deletes the lock
 * 	- the first thread is woken up indicating that the lock wasn't acquired.
 */


/**
 * This method immediately acquires the lock, sleeps
 * for SNOOZE_TIME and then releases the lock.  It sleeps
 * again for SNOOZE_TIME and then tries to re-acquire the
 * lock.  By this time, the other thread should have
 * deleted the lock.  This acquisition should fail.
 */
void
LockerDestructionTest::SimpleWaiterThread()
{
	CPPUNIT_ASSERT(fLocker->Lock());
	snooze(SNOOZE_TIME);
	fLocker->Unlock();
	snooze(SNOOZE_TIME);
	CPPUNIT_ASSERT(!fLocker->Lock());
}


/**
 * This method sleeps for SNOOZE_TIME and then acquires the lock.
 * It sleeps again for 2*SNOOZE_TIME and then deletes the lock.
 * This should wake up the other thread.
 */
void
LockerDestructionTest::SimpleDeleterThread()
{
	BLocker* tmpLock;
	snooze(SNOOZE_TIME);
	CPPUNIT_ASSERT(fLocker->Lock());
	snooze(SNOOZE_TIME);
	snooze(SNOOZE_TIME);
	tmpLock = fLocker;
	fLocker = NULL;
	delete tmpLock;
}


/**
 * \brief Timeout destruction tests
 *
 * The test works like the following:
 *  - the main thread acquires the lock
 *  - it creates a new thread and sleeps
 *  - the new thread attempts to acquire the lock but times out
 *  - the new thread then attempts to acquire the lock again
 *  - before the new thread times out a second time, the first thread releases
 *    the lock
 *  - at this time, the new thread acquires the lock and goes to sleep
 *  - the first thread attempts to acquire the lock
 *  - the second thread deletes the lock
 *  - the first thread is woken up indicating that the lock wasn't acquired.
 */


/**
 * This method immediately acquires the lock, sleeps
 * for SNOOZE_TIME and then releases the lock.  It sleeps
 * again for SNOOZE_TIME and then tries to re-acquire the
 * lock.  By this time, the other thread should have
 * deleted the lock.  This acquisition should fail.
 */
void
LockerDestructionTest::TimeoutWaiterThread()
{
	CPPUNIT_ASSERT(fLocker->LockWithTimeout(SNOOZE_TIME) == B_OK);
	snooze(SNOOZE_TIME);
	fLocker->Unlock();
	snooze(SNOOZE_TIME);
	// Should wake up with B_BAD_SEM_ID or B_BAD_VALUE since lock is deleted
	CPPUNIT_ASSERT(fLocker->LockWithTimeout(SNOOZE_TIME * 10) == B_BAD_SEM_ID);
}


/**
 * This method sleeps for SNOOZE_TIME/10 and then attempts to acquire
 * the lock for SNOOZE_TIME/10 seconds.  This acquisition will timeout
 * because the other thread is holding the lock.  Then it acquires the
 * lock by using a larger timeout. It sleeps again for 2*SNOOZE_TIME and
 * then deletes the lock.  This should wake up the other thread.
 */
void
LockerDestructionTest::TimeoutDeleterThread()
{
	BLocker* tmpLock;
	snooze(SNOOZE_TIME / 10);
	CPPUNIT_ASSERT(fLocker->LockWithTimeout(SNOOZE_TIME / 10) == B_TIMED_OUT);
	CPPUNIT_ASSERT(fLocker->LockWithTimeout(SNOOZE_TIME * 10) == B_OK);
	snooze(SNOOZE_TIME);
	snooze(SNOOZE_TIME);
	tmpLock = fLocker;
	fLocker = NULL;
	delete tmpLock;
}


CppUnit::Test*
LockerDestructionTest::suite()
{
	typedef BThreadedTestCaller<LockerDestructionTest> LockerDestructionTestCaller;
	CppUnit::TestSuite* testSuite = new CppUnit::TestSuite("LockerDestructionTest");

	// Benaphore
	LockerDestructionTest* simpleBenaphore = new LockerDestructionTest("SimpleBenaphore", true);
	LockerDestructionTestCaller* simpleBenaphoreCaller
		= new LockerDestructionTestCaller("BLocker::Destruction Test #1 (benaphore)", simpleBenaphore);
	simpleBenaphoreCaller->addThread("Waiter", &LockerDestructionTest::SimpleWaiterThread);
	simpleBenaphoreCaller->addThread("Deleter", &LockerDestructionTest::SimpleDeleterThread);
	testSuite->addTest(simpleBenaphoreCaller);

	// Semaphore
	LockerDestructionTest* simpleSemaphore = new LockerDestructionTest("SimpleSemaphore", false);
	LockerDestructionTestCaller* simpleSemaphoreCaller
		= new LockerDestructionTestCaller("BLocker::Destruction Test #1 (semaphore)", simpleSemaphore);
	simpleSemaphoreCaller->addThread("Waiter", &LockerDestructionTest::SimpleWaiterThread);
	simpleSemaphoreCaller->addThread("Deleter", &LockerDestructionTest::SimpleDeleterThread);
	testSuite->addTest(simpleSemaphoreCaller);

	// Benaphore
	LockerDestructionTest* timeoutBenaphore = new LockerDestructionTest("TimeoutBenaphore", true);
	LockerDestructionTestCaller* timeoutBenaphoreCaller
		= new LockerDestructionTestCaller("BLocker::Destruction Test #2 (benaphore)", timeoutBenaphore);
	timeoutBenaphoreCaller->addThread("Waiter", &LockerDestructionTest::TimeoutWaiterThread);
	timeoutBenaphoreCaller->addThread("Deleter", &LockerDestructionTest::TimeoutDeleterThread);
	testSuite->addTest(timeoutBenaphoreCaller);

	// Semaphore
	LockerDestructionTest* timeoutSemaphore = new LockerDestructionTest("TimeoutSemaphore", false);
	LockerDestructionTestCaller* timeoutSemaphoreCaller
		= new LockerDestructionTestCaller("BLocker::Destruction Test #2 (semaphore)", timeoutSemaphore);
	timeoutSemaphoreCaller->addThread("Waiter", &LockerDestructionTest::TimeoutWaiterThread);
	timeoutSemaphoreCaller->addThread("Deleter", &LockerDestructionTest::TimeoutDeleterThread);
	testSuite->addTest(timeoutSemaphoreCaller);

	return testSuite;
}


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(LockerDestructionTest, getTestSuiteName());
