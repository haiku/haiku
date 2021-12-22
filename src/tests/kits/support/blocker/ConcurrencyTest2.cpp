/*
	$Id: ConcurrencyTest2.cpp 301 2002-07-18 05:32:00Z tylerdauwalder $
	
	This file implements a test class for testing BLocker functionality.
	It tests use cases "Locking 1", "Locking 2", "Unlocking", "Is Locked",
	"Locking Thread" and "Count Locks".  It is essentially the same as Test1.cpp
	except it makes the first LockWithTimeout inside the threads timeout.  The
	reason for this is because the implementation of BLocker by Be and with Haiku
	is such that after one timeout occurs on a benaphore style BLocker, the lock
	effectively becomes a semaphore style BLocker.  This test tests that condition.
	
	*/


#include "ThreadedTestCaller.h"
#include "ConcurrencyTest2.h"
#include "cppunit/TestSuite.h"
#include "Locker.h"


// This constant indicates the number of times the thread should test the
// acquisition and release of the BLocker.

const int32 MAXLOOP = 10000;

// This constant is used to determine the number of microseconds to
// sleep during major steps of the test.

const bigtime_t SNOOZE_TIME = 200000;

/*
 *  Method:  ConcurrencyTest2::ConcurrencyTest2()
 *   Descr:  This is the only constructor for this test case.  It takes a
 *           test name and a flag to indicate whether to test a benaphore
 *           or semaphore type BLocker.
 */
		

	ConcurrencyTest2::ConcurrencyTest2(std::string name, bool benaphoreFlag) :
	LockerTestCase(name, benaphoreFlag), lockTestValue(false)
{
	}


/*
 *  Method:  ConcurrencyTest2::~ConcurrencyTest2()
 *   Descr:  This is the descriptor for this test case.
 */


	ConcurrencyTest2::~ConcurrencyTest2()
{
	}
	

/*
 *  Method:  ConcurrencyTest2::setUp()
 *   Descr:  This member is called before starting the actual test threads
 *           and is used to ensure that the class is initialized for the
 *           testing.  It just sets the "lockTestValue" flag to false.  This
 *           flag is used to show that there is mutual exclusion between the
 *           threads.
 */
	
void
	ConcurrencyTest2::setUp(void)
{
	lockTestValue = false;
	}


/*
 *  Method:  ConcurrencyTest2::suite()
 *   Descr:  This static member function returns a test suite for performing 
 *           all combinations of "ConcurrencyTest2".  The test suite contains
 *           two instances of the test.  One is performed on a benaphore,
 *           the other on a semaphore based BLocker.  Each individual test
 *           is created as a ThreadedTestCase (typedef'd as
 *           ConcurrencyTest2Caller) with three independent threads.
 */

CppUnit::Test *ConcurrencyTest2::suite(void)
{	
	typedef BThreadedTestCaller <ConcurrencyTest2 >
		ConcurrencyTest2Caller;
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite("ConcurrencyTest2");
	
	// Make a benaphore based test object, create a ThreadedTestCase for it and add
	// three threads to it.
	ConcurrencyTest2 *theTest = new ConcurrencyTest2("Benaphore", true);
	ConcurrencyTest2Caller *threadedTest1 = new ConcurrencyTest2Caller("BLocker::Concurrency Test #2 (benaphore)", theTest);
	threadedTest1->addThread("Acquire", &ConcurrencyTest2::AcquireThread);
	threadedTest1->addThread("Timeout1", &ConcurrencyTest2::TimeoutThread);
	threadedTest1->addThread("Timeout2", &ConcurrencyTest2::TimeoutThread);
				
	// Make a semaphore based test object, create a ThreadedTestCase for it and add
	// three threads to it.					 		
	theTest = new ConcurrencyTest2("Semaphore", false);
	ConcurrencyTest2Caller *threadedTest2 = new ConcurrencyTest2Caller("BLocker::Concurrency Test #2 (semaphore)", theTest);
	threadedTest2->addThread("Acquire", &ConcurrencyTest2::AcquireThread);
	threadedTest2->addThread("Timeout1", &ConcurrencyTest2::TimeoutThread);
	threadedTest2->addThread("Timeout2", &ConcurrencyTest2::TimeoutThread);
									 		
	testSuite->addTest(threadedTest1);	
	testSuite->addTest(threadedTest2);
	return(testSuite);
	}
	

/*
 *  Method:  ConcurrencyTest2::AcquireThread()
 *   Descr:  This member function acquires the lock, sleeps for SNOOZE_TIME,
 *           releases the lock and then launches into the lock loop test.
 */

void ConcurrencyTest2::AcquireThread(void)
{	
	SafetyLock theSafetyLock(theLocker);
	
	CPPUNIT_ASSERT(theLocker->Lock());
	NextSubTest();
	snooze(SNOOZE_TIME);					
	NextSubTest();
	theLocker->Unlock();
	NextSubTest();
	LockingLoop();
	NextSubTest();
	}

	
/*
 *  Method:  ConcurrencyTest2::AcquireLock()
 *   Descr:  This member function is passed the number of times through the
 *           acquisition loop (lockAttempt) and whether or not this is
 *           the first acquisition of the lock within this iteration.
 *           Based on these values, it may do a LockWithTimeout() or just
 *           a plain Lock() on theLocker.  This is done to get coverage of
 *           both lock acquisition methods on the BLocker.
 */
	
bool ConcurrencyTest2::AcquireLock(int lockAttempt,
                                                       bool firstAcquisition)
{
	bool timeoutLock;
	bool result;
	
	if (firstAcquisition) {
		timeoutLock = ((lockAttempt % 2) == 1);
	} else {
		timeoutLock = (((lockAttempt / 2) % 2) == 1);
	}
	if (timeoutLock) {
		result = (theLocker->LockWithTimeout(1000000) == B_OK);
	} else {
		result = theLocker->Lock();
	}
	return(result);
}


/*
 *  Method:  ConcurrencyTest2::TimeoutThread()
 *   Descr:  This member function sleeps for a short time and then attempts to
 *           acquire the lock for SNOOZE_TIME/10 seconds.  This acquisition
 *           should timeout.  Then the locking loop is started.
 */

void ConcurrencyTest2::TimeoutThread(void)
{
	SafetyLock theSafetyLock(theLocker);
	
	snooze(SNOOZE_TIME/2);
	NextSubTest();
	CPPUNIT_ASSERT(theLocker->LockWithTimeout(SNOOZE_TIME/10) == B_TIMED_OUT);
	NextSubTest();
	LockingLoop();
	NextSubTest();
}
	

/*
 *  Method:  ConcurrencyTest2::TestThread()
 *   Descr:  This method is the core of the test.  Each of the three threads
 *           run this method to perform the concurrency test.  First, the
 *           SafetyLock class (see LockerTestCase.h) is used to make sure that
 *           the lock is released if an assertion happens.  Then, each thread
 *           iterates MAXLOOP times through the main loop where the following
 *           actions are performed:
 *              - CheckLock() is used to show that the thread does not have
 *                the lock.
 *              - The thread acquires the lock.
 *              - The thread confirms that mutual exclusion is OK by testing
 *                lockTestValue.
 *              - The thread confirms the lock is held once by the thread.
 *              - The thread acquires the lock again.
 *              - The thread confirms the lock is held twice now by the thread.
 *              - The thread releases the lock once.
 *              - The thread confirms the lock is held once now.
 *              - The thread confirms that mutual exclusion is still OK by
 *                testing lockTestValue.
 *              - The thread releases the lock again.
 *              - The thread confirms that the lock is no longer held.
 */

void ConcurrencyTest2::LockingLoop(void)
{
	int i;
	SafetyLock theSafetyLock(theLocker);
	
	for (i = 0; i < MAXLOOP; i++) {	
		CheckLock(0);
		CPPUNIT_ASSERT(AcquireLock(i, true));
		
		CPPUNIT_ASSERT(!lockTestValue);
		lockTestValue = true;
		CheckLock(1);
		
		CPPUNIT_ASSERT(AcquireLock(i, false));
		CheckLock(2);
		
		theLocker->Unlock();
		CheckLock(1);
		
		CPPUNIT_ASSERT(lockTestValue);
		lockTestValue = false;
		theLocker->Unlock();
		CheckLock(0);
	}
}



