/*
	$Id: ConcurrencyTest2.cpp,v 1.1 2002/07/09 12:24:58 ejakowatz Exp $
	
	This file implements a test class for testing BLocker functionality.
	It tests use cases "Locking 1", "Locking 2", "Unlocking", "Is Locked",
	"Locking Thread" and "Count Locks".  It is essentially the same as Test1.cpp
	except it makes the first LockWithTimeout inside the threads timeout.  The
	reason for this is because the implementation of BLocker by Be and with OpenBeOS
	is such that after one timeout occurs on a benaphore style BLocker, the lock
	effectively becomes a semaphore style BLocker.  This test tests that condition.
	
	*/


#include "ConcurrencyTest2.h"
#include "TestSuite.h"
#include "ThreadedTestCaller.h"
#include <be/support/Locker.h>
#include "Locker.h"


// This constant indicates the number of times the thread should test the
// acquisition and release of the BLocker.

const int32 MAXLOOP = 10000;

// This constant is used to determine the number of microseconds to
// sleep during major steps of the test.

const bigtime_t SNOOZE_TIME = 200000;

/*
 *  Method:  ConcurrencyTest2<Locker>::ConcurrencyTest2()
 *   Descr:  This is the only constructor for this test case.  It takes a
 *           test name and a flag to indicate whether to test a benaphore
 *           or semaphore type BLocker.
 */
		
template<class Locker>
	ConcurrencyTest2<Locker>::ConcurrencyTest2(std::string name, bool benaphoreFlag) :
	LockerTestCase<Locker>(name, benaphoreFlag), lockTestValue(false)
{
	}


/*
 *  Method:  ConcurrencyTest2<Locker>::~ConcurrencyTest2()
 *   Descr:  This is the descriptor for this test case.
 */

template<class Locker>
	ConcurrencyTest2<Locker>::~ConcurrencyTest2()
{
	}
	

/*
 *  Method:  ConcurrencyTest2<Locker>::setUp()
 *   Descr:  This member is called before starting the actual test threads
 *           and is used to ensure that the class is initialized for the
 *           testing.  It just sets the "lockTestValue" flag to false.  This
 *           flag is used to show that there is mutual exclusion between the
 *           threads.
 */
	
template<class Locker> void
	ConcurrencyTest2<Locker>::setUp(void)
{
	lockTestValue = false;
	}


/*
 *  Method:  ConcurrencyTest2<Locker>::suite()
 *   Descr:  This static member function returns a test suite for performing 
 *           all combinations of "ConcurrencyTest2".  The test suite contains
 *           two instances of the test.  One is performed on a benaphore,
 *           the other on a semaphore based BLocker.  Each individual test
 *           is created as a ThreadedTestCase (typedef'd as
 *           ConcurrencyTest2Caller) with three independent threads.
 */

template<class Locker> Test *ConcurrencyTest2<Locker>::suite(void)
{	
	TestSuite *testSuite = new TestSuite("ConcurrencyTest2");
	
	// Make a benaphore based test object, create a ThreadedTestCase for it and add
	// three threads to it.
	ConcurrencyTest2<Locker> *theTest = new ConcurrencyTest2<Locker>("Benaphore", true);
	ConcurrencyTest2Caller *threadedTest1 = new ConcurrencyTest2Caller("", theTest);
	threadedTest1->addThread(":Thread1", &ConcurrencyTest2<Locker>::AcquireThread);
	threadedTest1->addThread(":Thread2", &ConcurrencyTest2<Locker>::TimeoutThread);
	threadedTest1->addThread(":Thread3", &ConcurrencyTest2<Locker>::TimeoutThread);
				
	// Make a semaphore based test object, create a ThreadedTestCase for it and add
	// three threads to it.					 		
	theTest = new ConcurrencyTest2<Locker>("Semaphore", false);
	ConcurrencyTest2Caller *threadedTest2 = new ConcurrencyTest2Caller("", theTest);
	threadedTest2->addThread(":Thread1", &ConcurrencyTest2<Locker>::AcquireThread);
	threadedTest2->addThread(":Thread2", &ConcurrencyTest2<Locker>::TimeoutThread);
	threadedTest2->addThread(":Thread3", &ConcurrencyTest2<Locker>::TimeoutThread);
									 		
	testSuite->addTest(threadedTest1);	
	testSuite->addTest(threadedTest2);
	return(testSuite);
	}
	

/*
 *  Method:  ConcurrencyTest2<Locker>::AcquireThread()
 *   Descr:  This member function acquires the lock, sleeps for SNOOZE_TIME,
 *           releases the lock and then launches into the lock loop test.
 */

template<class Locker> void ConcurrencyTest2<Locker>::AcquireThread(void)
{	
	SafetyLock<Locker> theSafetyLock(theLocker);
	
	assert(theLocker->Lock());
	snooze(SNOOZE_TIME);					
	theLocker->Unlock();
	LockingLoop();
	}

	
/*
 *  Method:  ConcurrencyTest2<Locker>::AcquireLock()
 *   Descr:  This member function is passed the number of times through the
 *           acquisition loop (lockAttempt) and whether or not this is
 *           the first acquisition of the lock within this iteration.
 *           Based on these values, it may do a LockWithTimeout() or just
 *           a plain Lock() on theLocker.  This is done to get coverage of
 *           both lock acquisition methods on the BLocker.
 */
	
template<class Locker> bool ConcurrencyTest2<Locker>::AcquireLock(int lockAttempt,
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
 *  Method:  ConcurrencyTest2<Locker>::TimeoutThread()
 *   Descr:  This member function sleeps for a short time and then attempts to
 *           acquire the lock for SNOOZE_TIME/10 seconds.  This acquisition
 *           should timeout.  Then the locking loop is started.
 */

template<class Locker> void ConcurrencyTest2<Locker>::TimeoutThread(void)
{
	SafetyLock<Locker> theSafetyLock(theLocker);
	
	snooze(SNOOZE_TIME/2);
	assert(theLocker->LockWithTimeout(SNOOZE_TIME/10) == B_TIMED_OUT);
	LockingLoop();
}
	

/*
 *  Method:  ConcurrencyTest2<Locker>::TestThread()
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

template<class Locker> void ConcurrencyTest2<Locker>::LockingLoop(void)
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


template class ConcurrencyTest2<BLocker>;
template class ConcurrencyTest2<OpenBeOS::BLocker>;