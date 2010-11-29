/*
	$Id: ConcurrencyTest1.cpp 301 2002-07-18 05:32:00Z tylerdauwalder $
	
	This file implements a test class for testing BLocker functionality.
	It tests use cases "Locking 1", "Locking 2", "Unlocking", "Is Locked",
	"Locking Thread" and "Count Locks".
	
	*/


#include <ThreadedTestCaller.h>
#include "ConcurrencyTest1.h"
#include <cppunit/TestSuite.h>
#include <Locker.h>


// This constant indicates the number of times the thread should test the
// acquisition and release of the BLocker.

const int32 MAXLOOP = 10000;


/*
 *  Method:  ConcurrencyTest1::ConcurrencyTest1()
 *   Descr:  This is the only constructor for this test case.  It takes a
 *           test name and a flag to indicate whether to test a benaphore
 *           or semaphore type BLocker.
 */
		

	ConcurrencyTest1::ConcurrencyTest1(std::string name, bool benaphoreFlag) :
	LockerTestCase(name, benaphoreFlag), lockTestValue(false)
{
	}


/*
 *  Method:  ConcurrencyTest1::~ConcurrencyTest1()
 *   Descr:  This is the descriptor for this test case.
 */


	ConcurrencyTest1::~ConcurrencyTest1()
{
	}
	

/*
 *  Method:  ConcurrencyTest1::setUp()
 *   Descr:  This member is called before starting the actual test threads
 *           and is used to ensure that the class is initialized for the
 *           testing.  It just sets the "lockTestValue" flag to false.  This
 *           flag is used to show that there is mutual exclusion between the
 *           threads.
 */
	
void
	ConcurrencyTest1::setUp(void)
{
	lockTestValue = false;
	}


/*
 *  Method:  ConcurrencyTest1::suite()
 *   Descr:  This static member function returns a test suite for performing 
 *           all combinations of "ConcurrencyTest1".  The test suite contains
 *           two instances of the test.  One is performed on a benaphore,
 *           the other on a semaphore based BLocker.  Each individual test
 *           is created as a ThreadedTestCase (typedef'd as
 *           ConcurrencyTest1Caller) with three independent threads.
 */

CppUnit::Test *ConcurrencyTest1::suite(void)
{	
	typedef BThreadedTestCaller<ConcurrencyTest1>
		ConcurrencyTest1Caller;


	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite("ConcurrencyTest1");
	
	// Make a benaphore based test object, create a ThreadedTestCase for it and add
	// three threads to it.
	ConcurrencyTest1 *theTest = new ConcurrencyTest1("Benaphore", true);
	ConcurrencyTest1Caller *threadedTest1 = new ConcurrencyTest1Caller("BLocker::Concurrency Test #1 (benaphore)", theTest);
	threadedTest1->addThread("A", &ConcurrencyTest1::TestThread);
	threadedTest1->addThread("B", &ConcurrencyTest1::TestThread);
	threadedTest1->addThread("C", &ConcurrencyTest1::TestThread);
				
	// Make a semaphore based test object, create a ThreadedTestCase for it and add
	// three threads to it.					 		
	theTest = new ConcurrencyTest1("Semaphore", false);
	ConcurrencyTest1Caller *threadedTest2 = new ConcurrencyTest1Caller("BLocker::Concurrency Test #1 (semaphore)", theTest);
	threadedTest2->addThread("A", &ConcurrencyTest1::TestThread);
	threadedTest2->addThread("B", &ConcurrencyTest1::TestThread);
	threadedTest2->addThread("C", &ConcurrencyTest1::TestThread);
									 		
	testSuite->addTest(threadedTest1);	
	testSuite->addTest(threadedTest2);
	return(testSuite);
	}
	

/*
 *  Method:  ConcurrencyTest1::AcquireLock()
 *   Descr:  This member function is passed the number of times through the
 *           acquisition loop (lockAttempt) and whether or not this is
 *           the first acquisition of the lock within this iteration.
 *           Based on these values, it may do a LockWithTimeout() or just
 *           a plain Lock() on theLocker.  This is done to get coverage of
 *           both lock acquisition methods on the BLocker.
 */
	
bool ConcurrencyTest1::AcquireLock(int lockAttempt,
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
 *  Method:  ConcurrencyTest1::TestThread()
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

void ConcurrencyTest1::TestThread(void)
{
	int i;
	SafetyLock theSafetyLock(theLocker);
	
	for (i = 0; i < MAXLOOP; i++) {
		// Print out 10 sub test markers per thread
		if (i % (MAXLOOP / 10) == 0)
			NextSubTest();
		
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




