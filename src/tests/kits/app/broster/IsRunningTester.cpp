//------------------------------------------------------------------------------
//	IsRunningTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <OS.h>
#include <Handler.h>
#include <Looper.h>
#include <Roster.h>
#include <String.h>

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "AppRunner.h"
#include "IsRunningTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

/*
	bool IsRunning(const char *signature) const
	@case 1			signature is NULL
	@results		Should return false.
*/
void IsRunningTester::IsRunningTestA1()
{
// R5: crashes when passing a NULL signature
#ifndef TEST_R5
	BRoster roster;
	CHK(roster.IsRunning((const char*)NULL) == false);
#endif
}

/*
	bool IsRunning(const char *signature) const
	@case 2			signature is not NULL, but no app with this signature is
					running
	@results		Should return false.
*/
void IsRunningTester::IsRunningTestA2()
{
	BRoster roster;
	CHK(roster.IsRunning("application/x-vnd.obos-app-run-testapp1") == false);
}

/*
	bool IsRunning(const char *signature) const
	@case 3			signature is not NULL and an (two) app(s) with this
					signature is (are) running; quit one; quit the second one
	@results		Should return true; true; false.
*/
void IsRunningTester::IsRunningTestA3()
{
	// run the remote apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	CHK(runner1.Run("AppRunTestApp1") == B_OK);
	CHK(runner2.Run("AppRunTestApp1") == B_OK);
	// create the BRoster and perform the tests
	BRoster roster;
	CHK(roster.IsRunning("application/x-vnd.obos-app-run-testapp1") == true);
	// quit app 1
	runner1.WaitFor(true);
	CHK(roster.IsRunning("application/x-vnd.obos-app-run-testapp1") == true);
	// quit app 2
	runner2.WaitFor(true);
	CHK(roster.IsRunning("application/x-vnd.obos-app-run-testapp1") == false);
}

/*
	bool IsRunning(entry_ref *ref) const
	@case 1			ref is NULL
	@results		Should return false.
*/
void IsRunningTester::IsRunningTestB1()
{
// R5: crashes when passing a NULL ref
#ifndef TEST_R5
	BRoster roster;
	CHK(roster.IsRunning((entry_ref*)NULL) == false);
#endif
}

/*
	bool IsRunning(entry_ref *ref) const
	@case 2			ref is not NULL, but no app with this ref is running
	@results		Should return false.
*/
void IsRunningTester::IsRunningTestB2()
{
	BRoster roster;
	entry_ref ref;
	CHK(find_test_app("AppRunTestApp1", &ref) == B_OK);
	CHK(roster.IsRunning(&ref) == false);
}

/*
	bool IsRunning(entry_ref *ref) const
	@case 3			ref is not NULL and an (two) app(s) with this ref is (are)
					running; quit one; quit the second one
	@results		Should return true; true; false.
*/
void IsRunningTester::IsRunningTestB3()
{
	entry_ref ref;
	CHK(find_test_app("AppRunTestApp1", &ref) == B_OK);
	// run the remote apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	CHK(runner1.Run("AppRunTestApp1") == B_OK);
	CHK(runner2.Run("AppRunTestApp1") == B_OK);
	// create the BRoster and perform the tests
	BRoster roster;
	CHK(roster.IsRunning(&ref) == true);
	// quit app 1
	runner1.WaitFor(true);
	CHK(roster.IsRunning(&ref) == true);
	// quit app 2
	runner2.WaitFor(true);
	CHK(roster.IsRunning(&ref) == false);
}


Test* IsRunningTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BRoster, SuiteOfTests, IsRunningTester, IsRunningTestA1);
	ADD_TEST4(BRoster, SuiteOfTests, IsRunningTester, IsRunningTestA2);
	ADD_TEST4(BRoster, SuiteOfTests, IsRunningTester, IsRunningTestA3);

	ADD_TEST4(BRoster, SuiteOfTests, IsRunningTester, IsRunningTestB1);
	ADD_TEST4(BRoster, SuiteOfTests, IsRunningTester, IsRunningTestB2);
	ADD_TEST4(BRoster, SuiteOfTests, IsRunningTester, IsRunningTestB3);

	return SuiteOfTests;
}

