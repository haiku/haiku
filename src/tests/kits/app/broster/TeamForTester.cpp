//------------------------------------------------------------------------------
//	TeamForTester.cpp
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
#include "TeamForTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

/*
	team_id TeamFor(const char *signature) const
	@case 1			signature is NULL
	@results		Should return B_BAD_VALUE.
*/
void TeamForTester::TeamForTestA1()
{
// R5: crashes when passing a NULL signature
#ifndef TEST_R5
	BRoster roster;
	CHK(roster.TeamFor((const char*)NULL) == B_BAD_VALUE);
#endif
}

/*
	team_id TeamFor(const char *signature) const
	@case 2			signature is not NULL, but no app with this signature is
					running
	@results		Should return B_ERROR.
*/
void TeamForTester::TeamForTestA2()
{
	BRoster roster;
	CHK(roster.TeamFor("application/x-vnd.obos-app-run-testapp1") == B_ERROR);
}

/*
	team_id TeamFor(const char *signature) const
	@case 3			signature is not NULL and an (two) app(s) with this
					signature is (are) running; quit one; quit the second one
	@results		Should return the ID of one of the teams;
					the ID of the second team; B_ERROR.
*/
void TeamForTester::TeamForTestA3()
{
	// run the remote apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	CHK(runner1.Run("AppRunTestApp1") == B_OK);
	CHK(runner2.Run("AppRunTestApp1") == B_OK);
	// create the BRoster and perform the tests
	BRoster roster;
	team_id team = roster.TeamFor("application/x-vnd.obos-app-run-testapp1");
	CHK(team == runner1.Team() || team == runner2.Team());
	// quit app 1
	runner1.WaitFor(true);
	CHK(roster.TeamFor("application/x-vnd.obos-app-run-testapp1")
		== runner2.Team());
	// quit app 2
	runner2.WaitFor(true);
	CHK(roster.TeamFor("application/x-vnd.obos-app-run-testapp1") == B_ERROR);
}

/*
	team_id TeamFor(entry_ref *ref) const
	@case 1			ref is NULL
	@results		Should return B_BAD_VALUE.
*/
void TeamForTester::TeamForTestB1()
{
// R5: crashes when passing a NULL ref
#ifndef TEST_R5
	BRoster roster;
	CHK(roster.TeamFor((entry_ref*)NULL) == B_BAD_VALUE);
#endif
}

/*
	team_id TeamFor(entry_ref *ref) const
	@case 2			ref is not NULL, but no app with this ref is running
	@results		Should return B_ERROR.
*/
void TeamForTester::TeamForTestB2()
{
	BRoster roster;
	entry_ref ref;
	CHK(find_test_app("AppRunTestApp1", &ref) == B_OK);
	CHK(roster.TeamFor(&ref) == B_ERROR);
}

/*
	team_id TeamFor(entry_ref *ref) const
	@case 3			ref is not NULL and an (two) app(s) with this ref is (are)
					running; quit one; quit the second one
	@results		Should return the ID of one of the teams;
					the ID of the second team; B_ERROR.
*/
void TeamForTester::TeamForTestB3()
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
	team_id team = roster.TeamFor(&ref);
	CHK(team == runner1.Team() || team == runner2.Team());
	// quit app 1
	runner1.WaitFor(true);
	CHK(roster.TeamFor(&ref) == runner2.Team());
	// quit app 2
	runner2.WaitFor(true);
	CHK(roster.TeamFor(&ref) == B_ERROR);
}


Test* TeamForTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BRoster, SuiteOfTests, TeamForTester, TeamForTestA1);
	ADD_TEST4(BRoster, SuiteOfTests, TeamForTester, TeamForTestA2);
	ADD_TEST4(BRoster, SuiteOfTests, TeamForTester, TeamForTestA3);

	ADD_TEST4(BRoster, SuiteOfTests, TeamForTester, TeamForTestB1);
	ADD_TEST4(BRoster, SuiteOfTests, TeamForTester, TeamForTestB2);
	ADD_TEST4(BRoster, SuiteOfTests, TeamForTester, TeamForTestB3);

	return SuiteOfTests;
}

