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
	// run the app
	AppRunner runner;
	CHK(runner.Run(appPath.String()) == B_OK);
	snooze(100000);
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


Test* TBApplicationTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BApplication, SuiteOfTests, TBApplicationTester, BApplication1);

	return SuiteOfTests;
}



