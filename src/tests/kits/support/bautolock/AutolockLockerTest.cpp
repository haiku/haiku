/*
	$Id: AutolockLockerTest.cpp 332 2002-07-19 06:45:28Z tylerdauwalder $
	
	This file tests all use cases of the BAutolock when used with a BLocker.
	BLooper based tests are done seperately.
	
	*/


#include "ThreadedTestCaller.h"
#include "AutolockLockerTest.h"
#include <Autolock.h>
#include <OS.h>


const bigtime_t SNOOZE_TIME = 250000;


/*
 *  Method: AutolockLockerTest::AutolockLockerTest()
 *   Descr: This method is the only constructor for the AutolockLockerTest
 *          class.
 */
		

	AutolockLockerTest::AutolockLockerTest(std::string name) : 
		BThreadedTestCase(name), theLocker(new BLocker)
{
	}


/*
 *  Method: AutolockLockerTest::~AutolockLockerTest()
 *   Descr: This method is the destructor for the AutolockLockerTest class.
 *          It only deallocates the autolocker and locker.
 */


	AutolockLockerTest::~AutolockLockerTest()
{
	delete theLocker;
	theLocker = NULL;
	}
		

/*
 *  Method:  AutolockLockerTest::TestThread1()
 *   Descr:  This method performs the tests on the Autolock.  It first acquires the
 *           lock and sleeps for a short time.  It deletes the lock rather than Unlock()
 *           it in order to test the other two threads.  Then, it constructs a new
 *           Locker and Autolock and checks that both the Autolock and the Locker are
 *           both locked.  Then, the Autolock is released by deleting it.  The Locker
 *           is checked to see that it is now released.  This is then repeated for an Autolock
 *           constructed by passing a reference to the Locker.
 */
	

	void AutolockLockerTest::TestThread1(void)
{
	BAutolock *theAutolock;
	
	NextSubTest();
	CPPUNIT_ASSERT(theLocker->Lock());
	CPPUNIT_ASSERT(theLocker->LockingThread() == find_thread(NULL));
	snooze(SNOOZE_TIME);
	delete theLocker;
	
	NextSubTest();
	theLocker =  new BLocker;
	theAutolock = new BAutolock(theLocker);

	CPPUNIT_ASSERT(theLocker->IsLocked());
	CPPUNIT_ASSERT(theLocker->LockingThread() == find_thread(NULL));
	CPPUNIT_ASSERT(theAutolock->IsLocked());
	
	NextSubTest();
	delete theAutolock;
	theAutolock = NULL;
	CPPUNIT_ASSERT(theLocker->LockingThread() != find_thread(NULL));
	
	NextSubTest();
	theAutolock = new BAutolock(*theLocker);
	CPPUNIT_ASSERT(theLocker->IsLocked());
	CPPUNIT_ASSERT(theLocker->LockingThread() == find_thread(NULL));
	CPPUNIT_ASSERT(theAutolock->IsLocked());
	
	NextSubTest();
	delete theAutolock;
	theAutolock = NULL;
	CPPUNIT_ASSERT(theLocker->LockingThread() != find_thread(NULL));
}


/*
 *  Method:  AutolockLockerTest::TestThread2()
 *   Descr:  This method performs the tests on the Autolock.  It first sleeps for a short
 *           time and then tries to acquire the lock with an Autolock.  It passes a pointer
 *           to the lock to the Autolock.  It expects the acquisition to fail and IsLocked()
 *           is tested to be sure.
 */
	

	void AutolockLockerTest::TestThread2(void)
{
	NextSubTest();
	snooze(SNOOZE_TIME / 10);
	BAutolock theAutolock(theLocker);
	CPPUNIT_ASSERT(!theAutolock.IsLocked());
}


/*
 *  Method:  AutolockLockerTest::TestThread3()
 *   Descr:  This method performs the tests on the Autolock.  It first sleeps for a short
 *           time and then tries to acquire the lock with an Autolock.  It passes a reference
 *           to the lock to the Autolock.  It expects the acquisition to fail and IsLocked()
 *           is tested to be sure.
 */
	

	void AutolockLockerTest::TestThread3(void)
{
	NextSubTest();
	snooze(SNOOZE_TIME / 10);
	BAutolock theAutolock(*theLocker);
	CPPUNIT_ASSERT(!theAutolock.IsLocked());
}


/*
 *  Method:  AutolockLockerTest::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           the "AutolockLockerTest" test.  The test caller
 *           is created as a ThreadedTestCaller (typedef'd as
 *           BenaphoreLockCountTest1Caller) with three independent threads.
 */


CppUnit::Test *AutolockLockerTest::suite(void)
{	
	typedef BThreadedTestCaller <AutolockLockerTest >
		AutolockLockerTestCaller;

	AutolockLockerTest *theTest = new AutolockLockerTest("");
	AutolockLockerTestCaller *threadedTest = new AutolockLockerTestCaller("BAutolock::Locker Test", theTest);
	threadedTest->addThread("A", &AutolockLockerTest::TestThread1);
	threadedTest->addThread("B", &AutolockLockerTest::TestThread2);
	threadedTest->addThread("C", &AutolockLockerTest::TestThread3);
	return(threadedTest);
}



