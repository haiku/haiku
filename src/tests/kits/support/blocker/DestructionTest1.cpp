/*
	$Id: DestructionTest1.cpp,v 1.1 2002/07/09 12:24:58 ejakowatz Exp $
	
	This file implements a test class for testing BLocker functionality.
	It tests use cases "Destruction" and "Locking 3".
	
	The test works like the following:
		- the main thread acquires the lock
		- the second thread sleeps
		- the second thread then attempts to acquire the lock
		- the first thread releases the lock
		- at this time, the new thread acquires the lock and goes to sleep
		- the first thread attempts to acquire the lock
		- the second thread deletes the lock
		- the first thread is woken up indicating that the lock wasn't acquired.
		
	*/


#include "DestructionTest1.h"
#include "TestSuite.h"
#include "ThreadedTestCaller.h"
#include <be/support/Locker.h>
#include "Locker.h"

// This constant is used to determine the number of microseconds to
// sleep during major steps of the test.

const bigtime_t SNOOZE_TIME = 200000;


/*
 *  Method:  DestructionTest1<Locker>::DestructionTest1()
 *   Descr:  This is the only constructor for this test class.
 */
		
template<class Locker>
	DestructionTest1<Locker>::DestructionTest1(std::string name,
											   bool isBenaphore) :
		LockerTestCase<Locker>(name, isBenaphore)
{
	}


/*
 *  Method:  DestructionTest1<Locker>::~DestructionTest1()
 *   Descr:  This is the only destructor for this test class.
 */

template<class Locker>
	DestructionTest1<Locker>::~DestructionTest1()
{
	}
	

/*
 *  Method:  DestructionTest1<Locker>::TestThread1()
 *   Descr:  This method immediately acquires the lock, sleeps
 *           for SNOOZE_TIME and then releases the lock.  It sleeps
 *           again for SNOOZE_TIME and then tries to re-acquire the
 *           lock.  By this time, the other thread should have
 *           deleted the lock.  This acquisition should fail.
 */

template<class Locker> void DestructionTest1<Locker>::TestThread1(void)
{
	assert(theLocker->Lock());	
	snooze(SNOOZE_TIME);
	theLocker->Unlock();
	snooze(SNOOZE_TIME);
	assert(!theLocker->Lock());
	}


/*
 *  Method:  DestructionTest1<Locker>::TestThread2()
 *   Descr:  This method sleeps for SNOOZE_TIME and then acquires the lock.
 *           It sleeps again for 2*SNOOZE_TIME and then deletes the lock.
 *           This should wake up the other thread.
 */

template<class Locker> void DestructionTest1<Locker>::TestThread2(void)
{
	Locker *tmpLock;
	
	snooze(SNOOZE_TIME);	
	assert(theLocker->Lock());
	snooze(SNOOZE_TIME);	
	snooze(SNOOZE_TIME);
	tmpLock = theLocker;
	theLocker = NULL;
	delete tmpLock;
}


/*
 *  Method:  DestructionTest1<Locker>::suite()
 *   Descr:  This static member function returns a test suite for performing 
 *           all combinations of "DestructionTest1".  The test suite contains
 *           two instances of the test.  One is performed on a benaphore,
 *           the other on a semaphore based BLocker.  Each individual test
 *           is created as a ThreadedTestCase (typedef'd as
 *           DestructionTest1Caller) with two independent threads.
 */

template<class Locker> Test *DestructionTest1<Locker>::suite(void)
{	
	TestSuite *testSuite = new TestSuite("DestructionTest1");
	
	// Make a benaphore based test object, create a ThreadedTestCase for it and add
	// two threads to it.
	DestructionTest1<Locker> *theTest = new DestructionTest1<Locker>("Benaphore", true);
	DestructionTest1Caller *threadedTest1 = new DestructionTest1Caller("", theTest);
	threadedTest1->addThread(":Thread1", &DestructionTest1<Locker>::TestThread1);
	threadedTest1->addThread(":Thread2", &DestructionTest1<Locker>::TestThread2);
				
	// Make a semaphore based test object, create a ThreadedTestCase for it and add
	// three threads to it.					 		
	theTest = new DestructionTest1<Locker>("Semaphore", false);
	DestructionTest1Caller *threadedTest2 = new DestructionTest1Caller("", theTest);
	threadedTest2->addThread(":Thread1", &DestructionTest1<Locker>::TestThread1);
	threadedTest2->addThread(":Thread2", &DestructionTest1<Locker>::TestThread2);
									 		
	testSuite->addTest(threadedTest1);	
	testSuite->addTest(threadedTest2);
	return(testSuite);
	}

template class DestructionTest1<BLocker>;
template class DestructionTest1<OpenBeOS::BLocker>;