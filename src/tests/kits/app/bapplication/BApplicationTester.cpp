//------------------------------------------------------------------------------
//	BApplicationTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <be/app/Message.h>
#include <be/kernel/OS.h>

#include <Application.h>
#include <Handler.h>
#include <Looper.h>
#include <String.h>

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "AppRunner.h"
#include "BApplicationTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// test_app
static
void
test_app(const char *app, const char *expectedResult)
{
	// construct an absolute path to the app
	BString appPath(BTestShell::GlobalTestDir());
	appPath.CharacterEscape(" \t\n!\"'`$&()?*+{}[]<>|", '\\');
	appPath += "/kits/app/";
	appPath += app;
#ifdef TEST_R5
	appPath += "_r5";
#endif
	// run the app
	AppRunner runner;
	CHK(runner.Run(appPath.String()) == B_OK);
	while (!runner.HasQuitted())
		snooze(10000);
	// read the output and compare the result
	char buffer[1024];
	ssize_t read = runner.ReadOutput(buffer, sizeof(buffer) - 1);
	CHK(read >= 0);
	buffer[read] = '\0';
if (strcmp(buffer, expectedResult))
printf("result is `%s', but should be `%s'\n", buffer, expectedResult);
	CHK(!strcmp(buffer, expectedResult));
}


/*
	BApplication(const char *signature)
	@case 1			signature is NULL
	@results		Should print error message and quit.
 */
void TBApplicationTester::BApplication1()
{
	test_app("BApplicationTestApp1",
			 "bad signature ((null)), must begin with \"application/\" and "
			 "can't conflict with existing registered mime types inside "
			 "the \"application\" media type.\n");
}

/*
	BApplication(const char *signature)
	@case 2			signature is no valid MIME string
	@results		Should print error message and quit.
 */
void TBApplicationTester::BApplication2()
{
	test_app("BApplicationTestApp2",
			 "bad signature (no valid MIME string), must begin with "
			 "\"application/\" and can't conflict with existing registered "
			 "mime types inside the \"application\" media type.\n");
}

/*
	BApplication(const char *signature)
	@case 3			signature is a valid MIME string, but doesn't have the
					"application" supertype
	@results		Should print error message and quit.
 */
void TBApplicationTester::BApplication3()
{
	test_app("BApplicationTestApp3",
			 "bad signature (image/gif), must begin with \"application/\" and "
			 "can't conflict with existing registered mime types inside "
			 "the \"application\" media type.\n");
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
	test_app("BApplicationTestApp4",
			 "Signature in rsrc doesn't match constructor arg. "
			 "(application/x-vnd.obos-bapplication-testapp4,"
			 "application/x-vnd.obos-bapplication-testapp4-or-not)\n"
			 "InitCheck(): 0\n");

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
	test_app("BApplicationTestApp5", "InitCheck(): 0\n");
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



