//------------------------------------------------------------------------------
//	main.cpp
//
//	Entry points for testing BMessenger
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "../common.h"
#include "BMessengerTester.h"
#include "TargetTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


/*
 *  Function:  addonTestFunc()
 *     Descr:  This function is called by the test application to
 *             get a pointer to the test to run.  The BMessageQueue test
 *             is a test suite.  A series of tests are added to
 *             the suite.
 */

Test* addonTestFunc(void)
{
	TestSuite* tests = new TestSuite("BHandler");

	tests->addTest(TBMessengerTester::Suite());
	tests->addTest(TargetTester::Suite());

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
