/*
	$Id: LockerTestCase.cpp 301 2002-07-18 05:32:00Z tylerdauwalder $
	
	This file implements a base class for testing BLocker functionality.
	
	*/


#include "LockerTestCase.h"
#include <Locker.h>


/*
 *  Method: LockerTestCase::LockerTestCase()
 *   Descr: This method is the only constructore for the LockerTestCase
 *          class.  It takes a test name and a flag to indicate whether
 *          the locker should be a benaphore or a semaphore.
 */
		

	LockerTestCase::LockerTestCase(std::string name, bool isBenaphore) : 
		BThreadedTestCase(name), theLocker(new BLocker(isBenaphore))
{
	}


/*
 *  Method: LockerTestCase::~LockerTestCase()
 *   Descr: This method is the destructor for the LockerTestCase class.
 *          It only deallocates the locker allocated in the constructor.
 */


	LockerTestCase::~LockerTestCase()
{
	delete theLocker;
	theLocker = NULL;
	}
		

/*
 *  Method:  LockerTestCase::CheckLock()
 *   Descr:  This method confirms that the lock is currently in a sane
 *           state.  If the lock is not sane, then an assertion is
 *           raised.  The caller provides the number of times the
 *           thread has successfully acquired the lock.  If the caller
 *           indicates that the lock has been acquired one or more times
 *           by the current thread, then the function confirms that.  If
 *           the caller indicates the lock has not been acquired 
 *           (expectedCount = 0), then it checks to make sure that the
 *           lock is not held by the current thread.  If it is, it
 *           raises an assertion.
 */
	
void LockerTestCase::CheckLock(int expectedCount)
{
	bool isLocked = theLocker->IsLocked();
	thread_id actualThread = theLocker->LockingThread();
	thread_id expectedThread = find_thread(NULL);
	int32 actualCount = theLocker->CountLocks();
	
	if (expectedCount > 0) {
		CPPUNIT_ASSERT(isLocked);
		CPPUNIT_ASSERT(expectedThread == actualThread);
		CPPUNIT_ASSERT(expectedCount == actualCount);
	} else {
		CPPUNIT_ASSERT(!((isLocked) && (actualThread == expectedThread)));
	}
	return;
}
	



