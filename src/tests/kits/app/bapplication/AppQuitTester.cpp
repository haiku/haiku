//------------------------------------------------------------------------------
//	AppQuitTester.cpp
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
#include "AppQuitTester.h"
#include "PipedAppRunner.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// check_output
template<typename AppRunnerC>
static
void
check_output(AppRunnerC &runner, const char *expectedOutput)
{
	BString buffer;
	CHK(runner.GetOutput(&buffer) == B_OK);
if (buffer != expectedOutput)
printf("result is `%s', but should be `%s'\n", buffer.String(),
expectedOutput);
	CHK(buffer == expectedOutput);
}

/*
	void Quit()
	@case 1			not running application
	@results		Should delete the application object.
*/
void AppQuitTester::QuitTest1()
{
	BApplication app("application/x-vnd.obos-app-quit-test");
	const char *output =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::~BApplication()\n";
	// run the apps
	AppRunner runner;
	CHK(runner.Run("AppQuitTestApp1") == B_OK);
	runner.WaitFor(false);
	// get the output and compare the result
	check_output(runner, output);
}

/*
	void Quit()
	@case 2			running application, call from looper thread
	@results		Run() should return. Should not delete the application
					object.
*/
void AppQuitTester::QuitTest2()
{
	BApplication app("application/x-vnd.obos-app-quit-test");
	const char *output =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::Run() done: 1\n"
		"BApplication::~BApplication()\n";
	// run the apps
	AppRunner runner;
	CHK(runner.Run("AppQuitTestApp2") == B_OK);
	runner.WaitFor(false);
	// get the output and compare the result
	check_output(runner, output);
}

/*
	void Quit()
	@case 3			running application, call from other thread
	@results		Run() should return. Should not delete the application
					object.
*/
void AppQuitTester::QuitTest3()
{
	BApplication app("application/x-vnd.obos-app-quit-test");
	const char *output =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::Run() done: 1\n"
		"BApplication::~BApplication()\n";
	// run the apps
	AppRunner runner;
	CHK(runner.Run("AppQuitTestApp3") == B_OK);
	runner.WaitFor(false);
	// get the output and compare the result
	check_output(runner, output);
}

/*
	void Quit()
	@case 4			running application, call from other thread, but don't
					lock before
	@results		Should print error message, but proceed anyway.
					Run() should return.
					Should not delete the application object.
*/
void AppQuitTester::QuitTest4()
{
	BApplication app("application/x-vnd.obos-app-quit-test");
	const char *output =
		"error: 0\n"
		"InitCheck(): 0\n"
		"BApplication::ReadyToRun()\n"
		"BApplication::Run() done: 1\n"
		"BApplication::~BApplication()\n";
	// run the apps
	PipedAppRunner runner;
	CHK(runner.Run("AppQuitTestApp4") == B_OK);
	runner.WaitFor();
	// get the output and compare the result
	BString buffer;
	CHK(runner.GetOutput(&buffer) == B_OK);
	// Remove the error message, as it contains the team ID, which we don't
	// know.
	int32 errorIndex = buffer.FindFirst(
		"ERROR - you must Lock the application object before calling Quit()");
	CHK(errorIndex >= 0);
	int32 errorEnd = buffer.FindFirst('\n', errorIndex);
	CHK(errorEnd >= 0);
	buffer.Remove(errorIndex, errorEnd - errorIndex + 1);
if (buffer != output)
printf("result is `%s', but should be `%s'\n", buffer.String(),
output);
	CHK(buffer == output);
}


Test* AppQuitTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BApplication, SuiteOfTests, AppQuitTester, QuitTest1);
	ADD_TEST4(BApplication, SuiteOfTests, AppQuitTester, QuitTest2);
	ADD_TEST4(BApplication, SuiteOfTests, AppQuitTester, QuitTest3);
	ADD_TEST4(BApplication, SuiteOfTests, AppQuitTester, QuitTest4);

	return SuiteOfTests;
}



