/*
	$Id: SemaphoreLockCountTest1.cpp 301 2002-07-18 05:32:00Z tylerdauwalder $
	
	This file implements a test class for testing BLocker functionality.
	It tests use cases "Count Lock Requests" for a semaphore style BLocker.
	
	The test works by:
		- checking the lock requests
		- acquiring the lock
		- checking the lock requests
		- staring a thread which times out acquiring the lock and then blocks
		  again waiting for the lock
		- checking the lock requests
		- start a second thread which times out acquiring the lock and then blocks
		  again waiting for the lock
		- checking the lock requests
		- release the lock
		- each blocked thread acquires the lock, checks the lock requests and releases
		  the lock before terminating
		- the main thread checks the lock requests one last time
		
	*/


#include "ThreadedTestCaller.h"
#include "SemaphoreLockCountTest1.h"
#include "cppunit/TestSuite.h"
#include <Locker.h>


// This constant is used to determine the number of microseconds to
// sleep during major steps of the test.

const bigtime_t SNOOZE_TIME = 100000;


/*
 *  Method:  SemaphoreLockCountTest1::SemaphoreLockCountTest1()
 *   Descr:  This is the constructor for this test class.
 */
		

	SemaphoreLockCountTest1::SemaphoreLockCountTest1(std::string name) :
		LockerTestCase(name, false)
{
	}


/*
 *  Method:  SemaphoreLockCountTest1::~SemaphoreLockCountTest1()
 *   Descr:  This is the destructor for this test class.
 */


	SemaphoreLockCountTest1::~SemaphoreLockCountTest1()
{
	}


/*
 *  Method:  SemaphoreLockCountTest1::CheckLockRequests()
 *   Descr:  This member function checks the actual number of lock requests
 *           that the BLocker thinks are outstanding versus the number
 *           passed in.  If they match, true is returned.
 */
	
bool SemaphoreLockCountTest1::CheckLockRequests(int expected)
{
	int actual = theLocker->CountLockRequests();
	return(actual == expected);
}


/*
 *  Method:  SemaphoreLockCountTest1::TestThread1()
 *   Descr:  This member function performs the main portion of the test.
 *           It first acquires thread2Lock and thread3Lock.  This ensures
 *           that thread2 and thread3 will block until this thread wants
 *           them to start running.  It then checks the lock count, acquires
 *           the lock and checks the lock count again.  It unlocks each
 *           of the other two threads in turn and rechecks the lock count.
 *           Finally, it releases the lock and sleeps for a short while
 *           for the other two threads to finish.  At the end, it checks
 *           the lock count on final time.
 */

void SemaphoreLockCountTest1::TestThread1(void)
{
	SafetyLock theSafetyLock1(theLocker);
	SafetyLock theSafetyLock2(&thread2Lock);
	SafetyLock theSafetyLock3(&thread3Lock);
	
	NextSubTest();
	CPPUNIT_ASSERT(thread2Lock.Lock());
	CPPUNIT_ASSERT(thread3Lock.Lock());

	NextSubTest();
	CPPUNIT_ASSERT(CheckLockRequests(1));
	CPPUNIT_ASSERT(theLocker->Lock());
	
	NextSubTest();
	CPPUNIT_ASSERT(CheckLockRequests(2));
	
	NextSubTest();
	thread2Lock.Unlock();
	snooze(SNOOZE_TIME);
	CPPUNIT_ASSERT(CheckLockRequests(4));
	
	NextSubTest();
	thread3Lock.Unlock();
	snooze(SNOOZE_TIME);
	CPPUNIT_ASSERT(CheckLockRequests(6));
	
	NextSubTest();
	theLocker->Unlock();
	snooze(SNOOZE_TIME);
	CPPUNIT_ASSERT(CheckLockRequests(3));
	}


/*
 *  Method:  SemaphoreLockCountTest1::TestThread2()
 *   Descr:  This member function defines the actions of the second thread of
 *           the test.  First it sleeps for a short while and then blocks on
 *           the thread2Lock.  When the first thread releases it, this thread
 *           begins its testing.  It times out attempting to acquire the main
 *           lock and then blocks to acquire the lock.  Once that lock is
 *           acquired, the lock count is checked before finishing this thread.
 */

void SemaphoreLockCountTest1::TestThread2(void)
{
	SafetyLock theSafetyLock1(theLocker);
	
	NextSubTest();
	snooze(SNOOZE_TIME / 10);
	CPPUNIT_ASSERT(thread2Lock.Lock());
	
	NextSubTest();
	CPPUNIT_ASSERT(theLocker->LockWithTimeout(SNOOZE_TIME / 10) == B_TIMED_OUT);
	CPPUNIT_ASSERT(theLocker->Lock());
	int actual = theLocker->CountLockRequests();
	CPPUNIT_ASSERT((actual == 4) || (actual == 5));
	theLocker->Unlock();
}


/*
 *  Method:  SemaphoreLockCountTest1::TestThread3()
 *   Descr:  This member function defines the actions of the second thread of
 *           the test.  First it sleeps for a short while and then blocks on
 *           the thread3Lock.  When the first thread releases it, this thread
 *           begins its testing.  It times out attempting to acquire the main
 *           lock and then blocks to acquire the lock.  Once that lock is
 *           acquired, the lock count is checked before finishing this thread.
 */

void SemaphoreLockCountTest1::TestThread3(void)
{
	SafetyLock theSafetyLock1(theLocker);
	
	NextSubTest();
	snooze(SNOOZE_TIME / 10);
	CPPUNIT_ASSERT(thread3Lock.Lock());
	
	NextSubTest();
	CPPUNIT_ASSERT(theLocker->LockWithTimeout(SNOOZE_TIME / 10) == B_TIMED_OUT);
	CPPUNIT_ASSERT(theLocker->Lock());
	int actual = theLocker->CountLockRequests();
	CPPUNIT_ASSERT((actual == 4) || (actual == 5));
	theLocker->Unlock();
}


/*
 *  Method:  SemaphoreLockCountTest1::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           the "SemaphoreLockCountTest1" test.  The test caller
 *           is created as a ThreadedTestCaller (typedef'd as
 *           SemaphoreLockCountTest1Caller) with three independent threads.
 */

CppUnit::Test *SemaphoreLockCountTest1::suite(void)
{	
	typedef BThreadedTestCaller <SemaphoreLockCountTest1 >
		SemaphoreLockCountTest1Caller;
		
	SemaphoreLockCountTest1 *theTest = new SemaphoreLockCountTest1("");
	SemaphoreLockCountTest1Caller *threadedTest = new SemaphoreLockCountTest1Caller("BLocker::Semaphore Lock Count Test", theTest);
	threadedTest->addThread("A", &SemaphoreLockCountTest1::TestThread1);
	threadedTest->addThread("B", &SemaphoreLockCountTest1::TestThread2);
	threadedTest->addThread("C", &SemaphoreLockCountTest1::TestThread3);
	return(threadedTest);
}

