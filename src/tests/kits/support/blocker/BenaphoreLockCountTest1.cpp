/*
	$Id: BenaphoreLockCountTest1.cpp 301 2002-07-18 05:32:00Z tylerdauwalder $
	
	This file implements a test class for testing BLocker functionality.
	It tests use cases "Count Lock Requests" for a benaphore style BLocker.
	
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


#include "BenaphoreLockCountTest1.h"

#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>

#include <Locker.h>

#include <ThreadedTestCaller.h>


// This constant is used to determine the number of microseconds to
// sleep during major steps of the test.

const bigtime_t SNOOZE_TIME = 100000;


/*
 *  Method:  BenaphoreLockCountTest1::BenaphoreLockCountTest1()
 *   Descr:  This is the constructor for this test class.
 */
		

	BenaphoreLockCountTest1::BenaphoreLockCountTest1(std::string name) :
		LockerTestCase(name, true)
{
	}


/*
 *  Method:  BenaphoreLockCountTest1::~BenaphoreLockTestCountTest1()
 *   Descr:  This is the destructor for this test class.
 */


	BenaphoreLockCountTest1::~BenaphoreLockCountTest1()
{
	}


/*
 *  Method:  BenaphoreLockCountTest1::CheckLockRequests()
 *   Descr:  This member function checks the actual number of lock requests
 *           that the BLocker thinks are outstanding versus the number
 *           passed in.  If they match, true is returned.
 */
	
bool BenaphoreLockCountTest1::CheckLockRequests(int expected)
{
	int actual = theLocker->CountLockRequests();
	return(actual == expected);
}


/*
 *  Method:  BenaphoreLockCountTest1::TestThread1()
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

void BenaphoreLockCountTest1::TestThread1(void)
{
	SafetyLock theSafetyLock1(theLocker);
	SafetyLock theSafetyLock2(&thread2Lock);
	SafetyLock theSafetyLock3(&thread3Lock);
	
	CPPUNIT_ASSERT(thread2Lock.Lock());
	NextSubTest();
	CPPUNIT_ASSERT(thread3Lock.Lock());
	NextSubTest();

	CPPUNIT_ASSERT(CheckLockRequests(0));
	NextSubTest();
	CPPUNIT_ASSERT(theLocker->Lock());
	NextSubTest();
	
	CPPUNIT_ASSERT(CheckLockRequests(1));
	NextSubTest();
	
	thread2Lock.Unlock();
	NextSubTest();
	snooze(SNOOZE_TIME);
	NextSubTest();
	CPPUNIT_ASSERT(CheckLockRequests(3));
	NextSubTest();
	
	thread3Lock.Unlock();
	NextSubTest();
	snooze(SNOOZE_TIME);
	NextSubTest();
	CPPUNIT_ASSERT(CheckLockRequests(5));
	NextSubTest();
	
	theLocker->Unlock();
	NextSubTest();
	snooze(SNOOZE_TIME);
	NextSubTest();
	CPPUNIT_ASSERT(CheckLockRequests(2));
	NextSubTest();
	}


/*
 *  Method:  BenaphoreLockCountTest1::TestThread2()
 *   Descr:  This member function defines the actions of the second thread of
 *           the test.  First it sleeps for a short while and then blocks on
 *           the thread2Lock.  When the first thread releases it, this thread
 *           begins its testing.  It times out attempting to acquire the main
 *           lock and then blocks to acquire the lock.  Once that lock is
 *           acquired, the lock count is checked before finishing this thread.
 */

void BenaphoreLockCountTest1::TestThread2(void)
{
	SafetyLock theSafetyLock1(theLocker);
	
	snooze(SNOOZE_TIME / 10);
	NextSubTest();
	CPPUNIT_ASSERT(thread2Lock.Lock());
	NextSubTest();
	
	CPPUNIT_ASSERT(theLocker->LockWithTimeout(SNOOZE_TIME / 10) == B_TIMED_OUT);
	NextSubTest();
	CPPUNIT_ASSERT(theLocker->Lock());
	NextSubTest();
	int actual = theLocker->CountLockRequests();
	NextSubTest();
	CPPUNIT_ASSERT((actual == 3) || (actual == 4));
	NextSubTest();
	theLocker->Unlock();
	NextSubTest();
}


/*
 *  Method:  BenaphoreLockCountTest1::TestThread3()
 *   Descr:  This member function defines the actions of the second thread of
 *           the test.  First it sleeps for a short while and then blocks on
 *           the thread3Lock.  When the first thread releases it, this thread
 *           begins its testing.  It times out attempting to acquire the main
 *           lock and then blocks to acquire the lock.  Once that lock is
 *           acquired, the lock count is checked before finishing this thread.
 */

void BenaphoreLockCountTest1::TestThread3(void)
{
	SafetyLock theSafetyLock1(theLocker);
	
	snooze(SNOOZE_TIME / 10);
	NextSubTest();
	CPPUNIT_ASSERT(thread3Lock.Lock());
	NextSubTest();
	
	CPPUNIT_ASSERT(theLocker->LockWithTimeout(SNOOZE_TIME / 10) == B_TIMED_OUT);
	NextSubTest();
	CPPUNIT_ASSERT(theLocker->Lock());
	NextSubTest();
	int actual = theLocker->CountLockRequests();
	NextSubTest();
	CPPUNIT_ASSERT((actual == 3) || (actual == 4));
	NextSubTest();
	theLocker->Unlock();
	NextSubTest();
}


/*
 *  Method:  BenaphoreLockCountTest1::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           the "BenaphoreLockCountTest1" test.  The test caller
 *           is created as a ThreadedTestCaller (typedef'd as
 *           BenaphoreLockCountTest1Caller) with three independent threads.
 */

CppUnit::Test *BenaphoreLockCountTest1::suite(void)
{	
	BenaphoreLockCountTest1 *theTest = new BenaphoreLockCountTest1("");
	BThreadedTestCaller<BenaphoreLockCountTest1> *threadedTest =
		new BThreadedTestCaller<BenaphoreLockCountTest1>("BLocker::Benaphore Lock Count Test #1", theTest);
	threadedTest->addThread("A", &BenaphoreLockCountTest1::TestThread1);
	threadedTest->addThread("B", &BenaphoreLockCountTest1::TestThread2);
	threadedTest->addThread("C", &BenaphoreLockCountTest1::TestThread3);
	return(threadedTest);
}

