//------------------------------------------------------------------------------
//	main.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LocalCommon.h"
#include "BArchivableTester.h"
#include "ValidateInstantiationTester.h"
#include "InstantiateObjectTester.h"
#include "FindInstantiationFuncTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
//	Function:	addonTestFunc()
//	Descr:		This function is called by the test application to
//				get a pointer to the test to run.
//------------------------------------------------------------------------------

Test* addonTestFunc()
{
	TestSuite* tests = new TestSuite("BArchivable");

	tests->addTest(TBArchivableTestCase::Suite());
	tests->addTest(TValidateInstantiationTest::Suite());
	tests->addTest(TInstantiateObjectTester::Suite());
	tests->addTest(TFindInstantiationFuncTester::Suite());

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

