//------------------------------------------------------------------------------
//	BApplicationTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <OS.h>
#include <Application.h>
#include <Handler.h>
#include <Looper.h>
#include <String.h>

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "PipedAppRunner.h"
#include "BApplicationTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// test_app
static
void
test_app(const char *app, const char *expectedResult)
{
	// run the app
	PipedAppRunner runner;
	CHK(runner.Run(app) == B_OK);
	runner.WaitFor();
	// get the output and compare the result
	BString buffer;
	CHK(runner.GetOutput(&buffer) == B_OK);
if (buffer != expectedResult)
printf("result is `%s', but should be `%s'\n", buffer.String(),
expectedResult);
	CHK(buffer == expectedResult);
}


/*
	BApplication(const char *signature)
	@case 1			signature is NULL
	@results		Should print error message and quit.
 */
void TBApplicationTester::BApplication1()
{
	const char *output1 =
		"bad signature ((null)), must begin with \"application/\" and "
		"can't conflict with existing registered mime types inside "
		"the \"application\" media type.\n";
	const char *output2 =
		"bad signature ((null)), must begin with \"application/\" and "
		"can't conflict with existing registered mime types inside "
		"the \"application\" media type.\n"
		"error: 80000005\n"
		"InitCheck(): 80000005\n";
	test_app("BApplicationTestApp1", output1);
	test_app("BApplicationTestApp1a", output1);
	test_app("BApplicationTestApp1b", output2);
}

/*
	BApplication(const char *signature)
	@case 2			signature is no valid MIME string
	@results		Should print error message and quit.
 */
void TBApplicationTester::BApplication2()
{
	const char *output1 =
		"bad signature (no valid MIME string), must begin with "
		"\"application/\" and can't conflict with existing registered "
		"mime types inside the \"application\" media type.\n";
	const char *output2 =
		"bad signature (no valid MIME string), must begin with "
		"\"application/\" and can't conflict with existing registered "
		"mime types inside the \"application\" media type.\n"
		"error: 80000005\n"
		"InitCheck(): 80000005\n";
	test_app("BApplicationTestApp2", output1);
	test_app("BApplicationTestApp2a", output1);
	test_app("BApplicationTestApp2b", output2);
}

/*
	BApplication(const char *signature)
	@case 3			signature is a valid MIME string, but doesn't have the
					"application" supertype
	@results		Should print error message and quit.
 */
void TBApplicationTester::BApplication3()
{
	const char *output1 =
		"bad signature (image/gif), must begin with \"application/\" and "
		"can't conflict with existing registered mime types inside "
		"the \"application\" media type.\n";
	const char *output2 =
		"bad signature (image/gif), must begin with \"application/\" and "
		"can't conflict with existing registered mime types inside "
		"the \"application\" media type.\n"
		"error: 80000005\n"
		"InitCheck(): 80000005\n";
	test_app("BApplicationTestApp3", output1);
	test_app("BApplicationTestApp3a", output1);
	test_app("BApplicationTestApp3b", output2);
}

/*
	BApplication(const char *signature)
	@case 4			signature is a valid MIME string with "application"
					supertype, but a different one than in the app
					attributes/resources
	@results		Should print warning message and continue.
					InitCheck() should return B_OK.
 */
void TBApplicationTester::BApplication4()
{
	const char *output1 =
		"Signature in rsrc doesn't match constructor arg. "
		"(application/x-vnd.obos-bapplication-testapp4,"
#ifndef TEST_R5
		" "
#endif
		"application/x-vnd.obos-bapplication-testapp4-or-not)\n"
		"InitCheck(): 0\n";
	const char *output2 =
		"Signature in rsrc doesn't match constructor arg. "
		"(application/x-vnd.obos-bapplication-testapp4,"
#ifndef TEST_R5
		" "
#endif
		"application/x-vnd.obos-bapplication-testapp4-or-not)\n"
		"error: 0\n"
		"InitCheck(): 0\n";
	test_app("BApplicationTestApp4", output1);
	test_app("BApplicationTestApp4a", output1);
	test_app("BApplicationTestApp4b", output2);
}

/*
	BApplication(const char *signature)
	@case 5			signature is a valid MIME string with "application"
					supertype, and the same as in the app attributes/resources
	@results		Shouldn't print anything at all and continue.
					InitCheck() should return B_OK.
 */
void TBApplicationTester::BApplication5()
{
	const char *output1 = "InitCheck(): 0\n";
	const char *output2 =
		"error: 0\n"
		"InitCheck(): 0\n";
	test_app("BApplicationTestApp5", output1);
	test_app("BApplicationTestApp5a", output1);
	test_app("BApplicationTestApp5b", output2);
}


Test* TBApplicationTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BApplication, SuiteOfTests, TBApplicationTester, BApplication1);
	ADD_TEST4(BApplication, SuiteOfTests, TBApplicationTester, BApplication2);
	ADD_TEST4(BApplication, SuiteOfTests, TBApplicationTester, BApplication3);
	ADD_TEST4(BApplication, SuiteOfTests, TBApplicationTester, BApplication4);
	ADD_TEST4(BApplication, SuiteOfTests, TBApplicationTester, BApplication5);

	return SuiteOfTests;
}



