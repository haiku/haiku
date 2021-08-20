/*
	$Id: AutolockTestAddon.cpp 10 2002-07-09 12:24:59Z ejakowatz $
	
	This file declares the addonTestName string and addonTestFunc
	function for the BLocker tests.  These symbols will be used
	when the addon is loaded.
	
	*/
	

#include "AutolockLockerTest.h"
#include "AutolockLooperTest.h"
#include <Autolock.h>
#include "Autolock.h"
#include "TestAddon.h"
#include "TestSuite.h"


/*
 *  Function:  addonTestFunc()
 *     Descr:  This function is called by the test application to
 *             get a pointer to the test to run.  The BLocker test
 *             is a test suite.  A series of tests are added to
 *             the suite.  Each test appears twice, once for
 *             the Be implementation of BLocker, once for the
 *             Haiku implementation.
 */

Test *addonTestFunc(void)
{
	TestSuite *testSuite = new TestSuite("BAutolock");
	
	testSuite->addTest(AutolockLockerTest<BAutolock, BLocker>::suite());
	testSuite->addTest(AutolockLooperTest<BAutolock, BLooper>::suite());
	
	testSuite->addTest(
		AutolockLockerTest<OpenBeOS::BAutolock, OpenBeOS::BLocker>::suite());
	testSuite->addTest(
		AutolockLooperTest<OpenBeOS::BAutolock, BLooper>::suite());
	
	return(testSuite);
}
