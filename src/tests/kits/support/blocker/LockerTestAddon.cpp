/*
	$Id: LockerTestAddon.cpp,v 1.1 2002/07/09 12:24:58 ejakowatz Exp $
	
	This file declares the addonTestName string and addonTestFunc
	function for the BLocker tests.  These symbols will be used
	when the addon is loaded.
	
	*/
	

#include "ConstructionTest1.h"
#include "ConcurrencyTest1.h"
#include "ConcurrencyTest2.h"
#include "DestructionTest1.h"
#include "DestructionTest2.h"
#include "BenaphoreLockCountTest1.h"
#include "SemaphoreLockCountTest1.h"
#include <Locker.h>
#include "Locker.h"
#include "TestAddon.h"
#include "TestSuite.h"


/*
 *  Function:  addonTestFunc()
 *     Descr:  This function is called by the test application to
 *             get a pointer to the test to run.  The BLocker test
 *             is a test suite.  A series of tests are added to
 *             the suite.  Each test appears twice, once for
 *             the Be implementation of BLocker, once for the
 *             OpenBeOS implementation.
 */

Test *addonTestFunc(void)
{
	TestSuite *testSuite = new TestSuite("Blocker");
	
	testSuite->addTest(ConstructionTest1<BLocker>::suite());
	testSuite->addTest(ConcurrencyTest1<BLocker>::suite());
	testSuite->addTest(ConcurrencyTest2<BLocker>::suite());
	testSuite->addTest(DestructionTest1<BLocker>::suite());
	testSuite->addTest(DestructionTest2<BLocker>::suite());
	testSuite->addTest(BenaphoreLockCountTest1<BLocker>::suite());
	testSuite->addTest(SemaphoreLockCountTest1<BLocker>::suite());
	
	testSuite->addTest(ConstructionTest1<OpenBeOS::BLocker>::suite());
	testSuite->addTest(ConcurrencyTest1<OpenBeOS::BLocker>::suite());
	testSuite->addTest(ConcurrencyTest2<OpenBeOS::BLocker>::suite());
	testSuite->addTest(DestructionTest1<OpenBeOS::BLocker>::suite());
	testSuite->addTest(DestructionTest2<OpenBeOS::BLocker>::suite());
	testSuite->addTest(BenaphoreLockCountTest1<OpenBeOS::BLocker>::suite());
	testSuite->addTest(SemaphoreLockCountTest1<OpenBeOS::BLocker>::suite());
	
	return(testSuite);
}
