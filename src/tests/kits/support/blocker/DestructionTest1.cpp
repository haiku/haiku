/*
	$Id: DestructionTest1.cpp 301 2002-07-18 05:32:00Z tylerdauwalder $
	
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


#include "ThreadedTestCaller.h"
#include "DestructionTest1.h"
#include "cppunit/TestSuite.h"
#include <Locker.h>

// This constant is used to determine the number of microseconds to
// sleep during major steps of the test.

const bigtime_t SNOOZE_TIME = 200000;


/*
 *  Method:  DestructionTest1::DestructionTest1()
 *   Descr:  This is the only constructor for this test class.
 */
		

	DestructionTest1::DestructionTest1(std::string name,
											   bool isBenaphore) :
		LockerTestCase(name, isBenaphore)
{
	}


/*
 *  Method:  DestructionTest1::~DestructionTest1()
 *   Descr:  This is the only destructor for this test class.
 */


	DestructionTest1::~DestructionTest1()
{
	}
	

/*
 *  Method:  DestructionTest1::TestThread1()
 *   Descr:  This method immediately acquires the lock, sleeps
 *           for SNOOZE_TIME and then releases the lock.  It sleeps
 *           again for SNOOZE_TIME and then tries to re-acquire the
 *           lock.  By this time, the other thread should have
 *           deleted the lock.  This acquisition should fail.
 */

void DestructionTest1::TestThread1(void)
{
	CPPUNIT_ASSERT(theLocker->Lock());	
	NextSubTest();
	snooze(SNOOZE_TIME);
	NextSubTest();
	theLocker->Unlock();
	NextSubTest();
	snooze(SNOOZE_TIME);
	NextSubTest();
	CPPUNIT_ASSERT(!theLocker->Lock());
	NextSubTest();
	}


/*
 *  Method:  DestructionTest1::TestThread2()
 *   Descr:  This method sleeps for SNOOZE_TIME and then acquires the lock.
 *           It sleeps again for 2*SNOOZE_TIME and then deletes the lock.
 *           This should wake up the other thread.
 */

void DestructionTest1::TestThread2(void)
{
	BLocker *tmpLock;
	
	snooze(SNOOZE_TIME);
	NextSubTest();
	CPPUNIT_ASSERT(theLocker->Lock());
	NextSubTest();
	snooze(SNOOZE_TIME);	
	NextSubTest();
	snooze(SNOOZE_TIME);
	NextSubTest();
	tmpLock = theLocker;
	NextSubTest();
	theLocker = NULL;
	NextSubTest();
	delete tmpLock;
}


/*
 *  Method:  DestructionTest1::suite()
 *   Descr:  This static member function returns a test suite for performing 
 *           all combinations of "DestructionTest1".  The test suite contains
 *           two instances of the test.  One is performed on a benaphore,
 *           the other on a semaphore based BLocker.  Each individual test
 *           is created as a ThreadedTestCase (typedef'd as
 *           DestructionTest1Caller) with two independent threads.
 */

CppUnit::Test *DestructionTest1::suite(void)
{	
	typedef BThreadedTestCaller <DestructionTest1 >
		DestructionTest1Caller;
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite("DestructionTest1");
	
	// Make a benaphore based test object, create a ThreadedTestCase for it and add
	// two threads to it.
	DestructionTest1 *theTest = new DestructionTest1("Benaphore", true);
	DestructionTest1Caller *threadedTest1 = new DestructionTest1Caller("BLocker::Destruction Test #1 (benaphore)", theTest);
	threadedTest1->addThread("A", &DestructionTest1::TestThread1);
	threadedTest1->addThread("B", &DestructionTest1::TestThread2);
				
	// Make a semaphore based test object, create a ThreadedTestCase for it and add
	// three threads to it.					 		
	theTest = new DestructionTest1("Semaphore", false);
	DestructionTest1Caller *threadedTest2 = new DestructionTest1Caller("BLocker::Destruction Test #1 (semaphore)", theTest);
	threadedTest2->addThread("A", &DestructionTest1::TestThread1);
	threadedTest2->addThread("B", &DestructionTest1::TestThread2);
									 		
	testSuite->addTest(threadedTest1);	
	testSuite->addTest(threadedTest2);
	return(testSuite);
	}

