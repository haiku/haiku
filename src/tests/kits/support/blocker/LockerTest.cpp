/*
	$Id: LockerTest.cpp 301 2002-07-18 05:32:00Z tylerdauwalder $
	
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
#include "LockerTest.h"
#include "cppunit/Test.h"
#include "cppunit/TestSuite.h"


/*
 *  Function:  addonTestFunc()
 *     Descr:  This function is called by the test application to
 *             get a pointer to the test to run.  The BLocker test
 *             is a test suite.  A series of tests are added to
 *             the suite.  Each test appears twice, once for
 *             the Be implementation of BLocker, once for the
 *             Haiku implementation.
 */

CppUnit::Test* LockerTestSuite()
{
	CppUnit::TestSuite *testSuite = new CppUnit::TestSuite();
	
	testSuite->addTest(ConstructionTest1::suite());
	testSuite->addTest(ConcurrencyTest1::suite());
	testSuite->addTest(ConcurrencyTest2::suite());
	testSuite->addTest(DestructionTest1::suite());
	testSuite->addTest(DestructionTest2::suite());
	testSuite->addTest(BenaphoreLockCountTest1::suite());
	testSuite->addTest(SemaphoreLockCountTest1::suite());
	
	return testSuite;
}




