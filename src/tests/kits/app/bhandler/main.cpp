//------------------------------------------------------------------------------
//	main.cpp
//
//	Entry points for testing BHandler
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"
#include "BHandlerTester.h"
#include "IsWatchedTest.h"
#include "LooperTest.h"
#include "SetNextHandlerTest.h"
#include "NextHandlerTest.h"
#include "AddFilterTest.h"
#include "RemoveFilterTest.h"
#include "SetFilterListTest.h"
#include "LockLooperTest.h"
#include "LockLooperWithTimeoutTest.h"
#include "UnlockLooperTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


/*
 *  Function:  addonTestFunc()
 *     Descr:  This function is called by the test application to
 *             get a pointer to the test to run.  The BMessageQueue test
 *             is a test suite.  A series of tests are added to
 *             the suite.  Each test appears twice, once for
 *             the Be implementation of BMessageQueue, once for the
 *             OpenBeOS implementation.
 */

Test* addonTestFunc(void)
{
	TestSuite* tests = new TestSuite("BHandler");

	tests->addTest(TBHandlerTester::Suite());
	tests->addTest(TIsWatchedTest::Suite());
	tests->addTest(TLooperTest::Suite());
	tests->addTest(TSetNextHandlerTest::Suite());
	tests->addTest(TNextHandlerTest::Suite());
	tests->addTest(TAddFilterTest::Suite());
	tests->addTest(TRemoveFilterTest::Suite());
	tests->addTest(TSetFilterListTest::Suite());
	tests->addTest(TLockLooperTest::Suite());
	tests->addTest(TLockLooperWithTimeoutTest::Suite());
	tests->addTest(TUnlockLooperTest::Suite());

	return tests;
}

int main()
{
	Test* tests = addonTestFunc();

	TextTestResult Result;
	tests->run(&Result);
	cout << Result << endl;

	delete tests;

	return !Result.wasSuccessful();
}

/*
 * $Log $
 *
 * $Id  $
 *
 */

