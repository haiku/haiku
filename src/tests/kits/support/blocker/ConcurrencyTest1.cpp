/*
	$Id: ConcurrencyTest1.cpp,v 1.1 2002/07/09 12:24:58 ejakowatz Exp $
	
	This file implements a test class for testing BLocker functionality.
	It tests use cases "Locking 1", "Locking 2", "Unlocking", "Is Locked",
	"Locking Thread" and "Count Locks".
	
	*/


#include "ConcurrencyTest1.h"
#include "TestSuite.h"
#include "ThreadedTestCaller.h"
#include <be/support/Locker.h>
#include "Locker.h"


// This constant indicates the number of times the thread should test the
// acquisition and release of the BLocker.

const int32 MAXLOOP = 10000;


/*
 *  Method:  ConcurrencyTest1<Locker>::ConcurrencyTest1()
 *   Descr:  This is the only constructor for this test case.  It takes a
 *           test name and a flag to indicate whether to test a benaphore
 *           or semaphore type BLocker.
 */
		
template<class Locker>
	ConcurrencyTest1<Locker>::ConcurrencyTest1(std::string name, bool benaphoreFlag) :
	LockerTestCase<Locker>(name, benaphoreFlag), lockTestValue(false)
{
	}


/*
 *  Method:  ConcurrencyTest1<Locker>::~ConcurrencyTest1()
 *   Descr:  This is the descriptor for this test case.
 */

template<class Locker>
	ConcurrencyTest1<Locker>::~ConcurrencyTest1()
{
	}
	

/*
 *  Method:  ConcurrencyTest1<Locker>::setUp()
 *   Descr:  This member is called before starting the actual test threads
 *           and is used to ensure that the class is initialized for the
 *           testing.  It just sets the "lockTestValue" flag to false.  This
 *           flag is used to show that there is mutual exclusion between the
 *           threads.
 */
	
template<class Locker> void
	ConcurrencyTest1<Locker>::setUp(void)
{
	lockTestValue = false;
	}


/*
 *  Method:  ConcurrencyTest1<Locker>::suite()
 *   Descr:  This static member function returns a test suite for performing 
 *           all combinations of "ConcurrencyTest1".  The test suite contains
 *           two instances of the test.  One is performed on a benaphore,
 *           the other on a semaphore based BLocker.  Each individual test
 *           is created as a ThreadedTestCase (typedef'd as
 *           ConcurrencyTest1Caller) with three independent threads.
 */

template<class Locker> Test *ConcurrencyTest1<Locker>::suite(void)
{	
	TestSuite *testSuite = new TestSuite("ConcurrencyTest1");
	
	// Make a benaphore based test object, create a ThreadedTestCase for it and add
	// three threads to it.
	ConcurrencyTest1<Locker> *theTest = new ConcurrencyTest1<Locker>("Benaphore", true);
	ConcurrencyTest1Caller *threadedTest1 = new ConcurrencyTest1Caller("", theTest);
	threadedTest1->addThread(":Thread1", &ConcurrencyTest1<Locker>::TestThread);
	threadedTest1->addThread(":Thread2", &ConcurrencyTest1<Locker>::TestThread);
	threadedTest1->addThread(":Thread3", &ConcurrencyTest1<Locker>::TestThread);
				
	// Make a semaphore based test object, create a ThreadedTestCase for it and add
	// three threads to it.					 		
	theTest = new ConcurrencyTest1<Locker>("Semaphore", false);
	ConcurrencyTest1Caller *threadedTest2 = new ConcurrencyTest1Caller("", theTest);
	threadedTest2->addThread(":Thread1", &ConcurrencyTest1<Locker>::TestThread);
	threadedTest2->addThread(":Thread2", &ConcurrencyTest1<Locker>::TestThread);
	threadedTest2->addThread(":Thread3", &ConcurrencyTest1<Locker>::TestThread);
									 		
	testSuite->addTest(threadedTest1);	
	testSuite->addTest(threadedTest2);
	return(testSuite);
	}
	

/*
 *  Method:  ConcurrencyTest1<Locker>::AcquireLock()
 *   Descr:  This member function is passed the number of times through the
 *           acquisition loop (lockAttempt) and whether or not this is
 *           the first acquisition of the lock within this iteration.
 *           Based on these values, it may do a LockWithTimeout() or just
 *           a plain Lock() on theLocker.  This is done to get coverage of
 *           both lock acquisition methods on the BLocker.
 */
	
template<class Locker> bool ConcurrencyTest1<Locker>::AcquireLock(int lockAttempt,
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
 *  Method:  ConcurrencyTest1<Locker>::TestThread()
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

template<class Locker> void ConcurrencyTest1<Locker>::TestThread(void)
{
	int i;
	SafetyLock<Locker> theSafetyLock(theLocker);
	
	for (i = 0; i < MAXLOOP; i++) {
		CheckLock(0);
		assert(AcquireLock(i, true));
		
		assert(!lockTestValue);
		lockTestValue = true;
		CheckLock(1);
		
		assert(AcquireLock(i, false));
		CheckLock(2);
		
		theLocker->Unlock();
		CheckLock(1);
		
		assert(lockTestValue);
		lockTestValue = false;
		theLocker->Unlock();
		CheckLock(0);
	}
}

template class ConcurrencyTest1<BLocker>;
template class ConcurrencyTest1<OpenBeOS::BLocker>;