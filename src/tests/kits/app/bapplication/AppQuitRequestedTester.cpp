//------------------------------------------------------------------------------
//	AppQuitRequestedTester.cpp
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
#include "AppRunner.h"
#include "AppQuitRequestedTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// check_output
static
void
check_output(AppRunner &runner, const char *expectedOutput)
{
	BString buffer;
	CHK(runner.GetOutput(&buffer) == B_OK);
if (buffer != expectedOutput)
printf("result is `%s', but should be `%s'\n", buffer.String(),
expectedOutput);
	CHK(buffer == expectedOutput);
}

/*
	bool QuitRequested()
	@case 1			return false the first time, true the second time it is
					invoked
	@results		Should not quit after the first time, but after the
					second one.
*/
void AppQuitRequestedTester::QuitRequestedTest1()
{
	BApplication app("application/x-vnd.obos-app-quit-requested-test");
	const char *output =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::QuitRequested()\n"
		"BApplication::Run() done: 1\n"
		"BApplication::~BApplication()\n";
	// run the app
	AppRunner runner;
	CHK(runner.Run("AppQuitRequestedTestApp1") == B_OK);
	runner.WaitFor(false);
	// get the output and compare the result
	check_output(runner, output);
}


Test* AppQuitRequestedTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BApplication, SuiteOfTests, AppQuitRequestedTester,
			  QuitRequestedTest1);

	return SuiteOfTests;
}



