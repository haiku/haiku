/*
	$Id: AutolockLockerTest.cpp,v 1.1 2002/07/09 12:24:57 ejakowatz Exp $
	
	This file tests all use cases of the BAutolock when used with a BLocker.
	BLooper based tests are done seperately.
	
	*/


#include "AutolockLockerTest.h"
#include <be/support/Autolock.h>
#include "Autolock.h"
#include <OS.h>


const bigtime_t SNOOZE_TIME = 250000;


/*
 *  Method: AutolockLockerTest<Autolock, Locker>::AutolockLockerTest()
 *   Descr: This method is the only constructor for the AutolockLockerTest
 *          class.
 */
		
template<class Autolock, class Locker>
	AutolockLockerTest<Autolock, Locker>::AutolockLockerTest(std::string name) : 
		TestCase(name), theLocker(new Locker)
{
	}


/*
 *  Method: AutolockLockerTest<Autolock, Locker>::~AutolockLockerTest()
 *   Descr: This method is the destructor for the AutolockLockerTest class.
 *          It only deallocates the autolocker and locker.
 */

template<class Autolock, class Locker>
	AutolockLockerTest<Autolock, Locker>::~AutolockLockerTest()
{
	delete theLocker;
	theLocker = NULL;
	}
		

/*
 *  Method:  AutolockLockerTest<Autolock, Locker>::TestThread1()
 *   Descr:  This method performs the tests on the Autolock.  It first acquires the
 *           lock and sleeps for a short time.  It deletes the lock rather than Unlock()
 *           it in order to test the other two threads.  Then, it constructs a new
 *           Locker and Autolock and checks that both the Autolock and the Locker are
 *           both locked.  Then, the Autolock is released by deleting it.  The Locker
 *           is checked to see that it is now released.  This is then repeated for an Autolock
 *           constructed by passing a reference to the Locker.
 */
	
template<class Autolock, class Locker>
	void AutolockLockerTest<Autolock, Locker>::TestThread1(void)
{
	Autolock *theAutolock;
	
	assert(theLocker->Lock());
	assert(theLocker->LockingThread() == find_thread(NULL));
	snooze(SNOOZE_TIME);
	delete theLocker;
	
	theLocker =  new Locker;
	theAutolock = new Autolock(theLocker);

	assert(theLocker->IsLocked());
	assert(theLocker->LockingThread() == find_thread(NULL));
	assert(theAutolock->IsLocked());
	
	delete theAutolock;
	theAutolock = NULL;
	assert(theLocker->LockingThread() != find_thread(NULL));
	
	theAutolock = new Autolock(*theLocker);
	assert(theLocker->IsLocked());
	assert(theLocker->LockingThread() == find_thread(NULL));
	assert(theAutolock->IsLocked());
	
	delete theAutolock;
	theAutolock = NULL;
	assert(theLocker->LockingThread() != find_thread(NULL));
}


/*
 *  Method:  AutolockLockerTest<Autolock, Locker>::TestThread2()
 *   Descr:  This method performs the tests on the Autolock.  It first sleeps for a short
 *           time and then tries to acquire the lock with an Autolock.  It passes a pointer
 *           to the lock to the Autolock.  It expects the acquisition to fail and IsLocked()
 *           is tested to be sure.
 */
	
template<class Autolock, class Locker>
	void AutolockLockerTest<Autolock, Locker>::TestThread2(void)
{
	snooze(SNOOZE_TIME / 10);
	Autolock theAutolock(theLocker);
	assert(!theAutolock.IsLocked());
}


/*
 *  Method:  AutolockLockerTest<Autolock, Locker>::TestThread3()
 *   Descr:  This method performs the tests on the Autolock.  It first sleeps for a short
 *           time and then tries to acquire the lock with an Autolock.  It passes a reference
 *           to the lock to the Autolock.  It expects the acquisition to fail and IsLocked()
 *           is tested to be sure.
 */
	
template<class Autolock, class Locker>
	void AutolockLockerTest<Autolock, Locker>::TestThread3(void)
{
	snooze(SNOOZE_TIME / 10);
	Autolock theAutolock(*theLocker);
	assert(!theAutolock.IsLocked());
}


/*
 *  Method:  AutolockLockerTest<Autolock, Locker>::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           the "AutolockLockerTest" test.  The test caller
 *           is created as a ThreadedTestCaller (typedef'd as
 *           BenaphoreLockCountTest1Caller) with three independent threads.
 */

template<class Autolock, class Locker>
	Test *AutolockLockerTest<Autolock, Locker>::suite(void)
{	
	AutolockLockerTest<Autolock, Locker> *theTest = new AutolockLockerTest<Autolock, Locker>("");
	AutolockLockerTestCaller *threadedTest = new AutolockLockerTestCaller("", theTest);
	threadedTest->addThread(":Thread1", &AutolockLockerTest<Autolock, Locker>::TestThread1);
	threadedTest->addThread(":Thread2", &AutolockLockerTest<Autolock, Locker>::TestThread2);
	threadedTest->addThread(":Thread3", &AutolockLockerTest<Autolock, Locker>::TestThread3);
	return(threadedTest);
	}
	
	
template class AutolockLockerTest<BAutolock, BLocker>;
template class AutolockLockerTest<OpenBeOS::BAutolock, OpenBeOS::BLocker>;