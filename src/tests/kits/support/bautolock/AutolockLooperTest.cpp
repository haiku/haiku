/*
	$Id: AutolockLooperTest.cpp,v 1.1 2002/07/09 12:24:57 ejakowatz Exp $
	
	This file tests all use cases of the BAutolock when used with a BLooper.
	BLocker based tests are done seperately.
	
	*/


#include "AutolockLooperTest.h"
#include <be/support/Autolock.h>
#include "Autolock.h"
#include <OS.h>


const bigtime_t SNOOZE_TIME = 250000;


/*
 *  Method: AutolockLooperTest<Autolock, Looper>::AutolockLooperTest()
 *   Descr: This method is the only constructor for the AutolockLooperTest
 *          class.
 */
		
template<class Autolock, class Looper>
	AutolockLooperTest<Autolock, Looper>::AutolockLooperTest(std::string name) : 
		TestCase(name), theLooper(new Looper)
{
	theLooper->Run();
	}


/*
 *  Method: AutolockLooperTest<Autolock, Looper>::~AutolockLooperTest()
 *   Descr: This method is the destructor for the AutolockLooperTest class.
 *          It only deallocates the autoLooper and Looper.
 */

template<class Autolock, class Looper>
	AutolockLooperTest<Autolock, Looper>::~AutolockLooperTest()
{
	if (theLooper != NULL)
		theLooper->Lock();
		theLooper->Quit();
	}
		

/*
 *  Method:  AutolockLooperTest<Autolock, Looper>::TestThread1()
 *   Descr:  This method performs the tests on the Autolock.  It constructs a new
 *           Autolock and checks that both the Autolock and the Looper are
 *           both locked.  Then, the Autolock is released by deleting it.  The Looper
 *           is checked to see that it is now released.
 */
	
template<class Autolock, class Looper>
	void AutolockLooperTest<Autolock, Looper>::TestThread1(void)
{
	Autolock *theAutolock = new Autolock(theLooper);

	assert(theLooper->IsLocked());
	assert(theLooper->LockingThread() == find_thread(NULL));
	assert(theAutolock->IsLocked());
	
	delete theAutolock;
	theAutolock = NULL;
	assert(theLooper->LockingThread() != find_thread(NULL));
}


/*
 *  Method:  AutolockLooperTest<Autolock, Looper>::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           the "AutolockLooperTest" test.  The test caller
 *           is created as a ThreadedTestCaller (typedef'd as
 *           BenaphoreLockCountTest1Caller) with three independent threads.
 */

template<class Autolock, class Looper>
	Test *AutolockLooperTest<Autolock, Looper>::suite(void)
{	
	AutolockLooperTest<Autolock, Looper> *theTest = new AutolockLooperTest<Autolock, Looper>("");
	AutolockLooperTestCaller *threadedTest = new AutolockLooperTestCaller("", theTest);
	threadedTest->addThread(":Thread1", &AutolockLooperTest<Autolock, Looper>::TestThread1);
	return(threadedTest);
	}
	
	
template class AutolockLooperTest<BAutolock, BLooper>;
template class AutolockLooperTest<OpenBeOS::BAutolock, BLooper>;