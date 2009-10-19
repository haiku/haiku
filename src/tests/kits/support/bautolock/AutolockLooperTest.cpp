/*
	$Id: AutolockLooperTest.cpp 332 2002-07-19 06:45:28Z tylerdauwalder $
	
	This file tests all use cases of the BAutolock when used with a BLooper.
	BLocker based tests are done seperately.
	
	*/


#include "ThreadedTestCaller.h"
#include "AutolockLooperTest.h"
#include <Autolock.h>
#include <Looper.h>
#include <OS.h>


const bigtime_t SNOOZE_TIME = 250000;


/*
 *  Method: AutolockLooperTest::AutolockLooperTest()
 *   Descr: This method is the only constructor for the AutolockLooperTest
 *          class.
 */
		

	AutolockLooperTest::AutolockLooperTest(std::string name) : 
		BThreadedTestCase(name), theLooper(new BLooper)
{
	theLooper->Run();
	}


/*
 *  Method: AutolockLooperTest::~AutolockLooperTest()
 *   Descr: This method is the destructor for the AutolockLooperTest class.
 *          It only deallocates the autoLooper and Looper.
 */


	AutolockLooperTest::~AutolockLooperTest()
{
	if (theLooper != NULL)
		theLooper->Lock();
		theLooper->Quit();
	}
		

/*
 *  Method:  AutolockLooperTest::TestThread1()
 *   Descr:  This method performs the tests on the Autolock.  It constructs a new
 *           Autolock and checks that both the Autolock and the Looper are
 *           both locked.  Then, the Autolock is released by deleting it.  The Looper
 *           is checked to see that it is now released.
 */
	

	void AutolockLooperTest::TestThread1(void)
{
	BAutolock *theAutolock = new BAutolock(theLooper);

	NextSubTest();
	CPPUNIT_ASSERT(theLooper->IsLocked());
	CPPUNIT_ASSERT(theLooper->LockingThread() == find_thread(NULL));
	CPPUNIT_ASSERT(theAutolock->IsLocked());
	
	NextSubTest();
	delete theAutolock;
	theAutolock = NULL;
	CPPUNIT_ASSERT(theLooper->LockingThread() != find_thread(NULL));
}


/*
 *  Method:  AutolockLooperTest::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           the "AutolockLooperTest" test.  The test caller
 *           is created as a ThreadedTestCaller (typedef'd as
 *           BenaphoreLockCountTest1Caller) with three independent threads.
 */


CppUnit::Test *AutolockLooperTest::suite(void)
{	
	typedef BThreadedTestCaller <AutolockLooperTest >
		AutolockLooperTestCaller;

	AutolockLooperTest *theTest = new AutolockLooperTest("");
	AutolockLooperTestCaller *threadedTest = new AutolockLooperTestCaller("BAutolock::Looper Test", theTest);
	threadedTest->addThread("A", &AutolockLooperTest::TestThread1);
	return(threadedTest);
}




