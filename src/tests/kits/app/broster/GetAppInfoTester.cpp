//------------------------------------------------------------------------------
//	GetAppInfoTester.cpp
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
#include "GetAppInfoTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// check_app_info
static
bool
check_app_info(app_info &info, AppRunner &runner, const char *signature,
			   uint32 flags)
{
	team_id team = runner.Team();
	// get the thread
	thread_id thread = -1;
	int32 cookie = 0;
	thread_info threadInfo;
	while (get_next_thread_info(team, &cookie, &threadInfo) == B_OK) {
		if (thread < 0 || threadInfo.thread < thread)
			thread = threadInfo.thread;
	}
	// get port and ref
	port_id port = runner.AppLooperPort();
	entry_ref ref;
	runner.GetRef(&ref);
	// compare
//printf("check_app_info(): "
//"  thread:    %ld vs %ld\n"
//"  team:      %ld vs %ld\n"
//"  port:      %ld vs %ld\n"
//"  flags:     %lx  vs %lx\n"
//"  signature: `%s' vs `%s'\n", info.thread, thread, info.team, team,
//info.port, port, info.flags, flags, info.signature, signature);
//printf("  ref:       (%ld, %lld, `%s') vs (%ld, %lld, `%s')\n",
//info.ref.device, info.ref.directory, info.ref.name,
//ref.device, ref.directory, ref.name);
	return (info.thread == thread && info.team == team && info.port == port
			&& info.flags == flags && info.ref == ref
			&& !strncmp(info.signature, signature, B_MIME_TYPE_LENGTH));
}

/*
	status_t GetAppInfo(const char *signature, app_info *info) const
	@case 1			signature is NULL or info is NULL
	@results		Should return B_BAD_VALUE.
*/
void GetAppInfoTester::GetAppInfoTestA1()
{
	BRoster roster;
	app_info info;
	CHK(roster.GetAppInfo((const char*)NULL, NULL) == B_BAD_VALUE);
	CHK(roster.GetAppInfo((const char*)NULL, &info) == B_BAD_VALUE);
// R5: crashes when passing a NULL app_info
#ifndef TEST_R5
	CHK(roster.GetAppInfo("application/x-vnd.obos-app-run-testapp1",
						  NULL) == B_BAD_VALUE);
#endif
}

/*
	status_t GetAppInfo(const char *signature, app_info *info) const
	@case 2			signature/info are not NULL, but no app with this
					signature is running
	@results		Should return B_ERROR.
*/
void GetAppInfoTester::GetAppInfoTestA2()
{
	BRoster roster;
	app_info info;
	CHK(roster.GetAppInfo("application/x-vnd.obos-app-run-testapp1", &info)
		== B_ERROR);
}

/*
	status_t GetAppInfo(const char *signature, app_info *info) const
	@case 3			signature/info are not NULL and an (two) app(s) with this
					signature is (are) running; quit one; quit the second one
	@results		Should
					- fill the app info with the data of one of the apps and
					  return B_OK;
					- fill the app info with the data of the second apps and
					  return B_OK;
					- return B_ERROR.
*/
void GetAppInfoTester::GetAppInfoTestA3()
{
	const char *signature = "application/x-vnd.obos-app-run-testapp1";
	uint32 flags = B_MULTIPLE_LAUNCH | B_ARGV_ONLY;
	// run the remote apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	CHK(runner1.Run("AppRunTestApp1") == B_OK);
	CHK(runner2.Run("AppRunTestApp1") == B_OK);
	// create the BRoster and perform the tests
	BRoster roster;
	app_info info1;
	CHK(roster.GetAppInfo(signature, &info1) == B_OK);
	CHK(check_app_info(info1, runner1, signature, flags)
		|| check_app_info(info1, runner2, signature, flags));
	// quit app 1
	runner1.WaitFor(true);
	app_info info2;
	CHK(roster.GetAppInfo(signature, &info2) == B_OK);
	CHK(check_app_info(info2, runner2, signature, flags));
	// quit app 2
	runner2.WaitFor(true);
	CHK(roster.GetAppInfo(signature, &info1) == B_ERROR);
}

/*
	status_t GetAppInfo(entry_ref *ref, app_info *info) const
	@case 1			ref is NULL or info is NULL
	@results		Should return B_BAD_VALUE.
*/
void GetAppInfoTester::GetAppInfoTestB1()
{
	BRoster roster;
	entry_ref ref;
	CHK(find_test_app("AppRunTestApp1", &ref) == B_OK);
	app_info info;
	CHK(roster.GetAppInfo((entry_ref*)NULL, NULL) == B_BAD_VALUE);
	CHK(roster.GetAppInfo((entry_ref*)NULL, &info) == B_BAD_VALUE);
// R5: crashes when passing a NULL app_info
#ifndef TEST_R5
	CHK(roster.GetAppInfo(&ref, NULL) == B_BAD_VALUE);
#endif
}

/*
	status_t GetAppInfo(entry_ref *ref, app_info *info) const
	@case 2			ref/info are not NULL, but no app with this ref is running
	@results		Should return B_ERROR.
*/
void GetAppInfoTester::GetAppInfoTestB2()
{
	BRoster roster;
	entry_ref ref;
	CHK(find_test_app("AppRunTestApp1", &ref) == B_OK);
	app_info info;
	CHK(roster.GetAppInfo(&ref, &info) == B_ERROR);
}

/*
	status_t GetAppInfo(entry_ref *ref, app_info *info) const
	@case 3			ref/info are not NULL and an (two) app(s) with this ref
					is (are) running; quit one; quit the second one
	@results		Should
					- fill the app info with the data of one of the apps and
					  return B_OK;
					- fill the app info with the data of the second apps and
					  return B_OK;
					- return B_ERROR.
*/
void GetAppInfoTester::GetAppInfoTestB3()
{
	const char *signature = "application/x-vnd.obos-app-run-testapp1";
	uint32 flags = B_MULTIPLE_LAUNCH | B_ARGV_ONLY;
	entry_ref ref;
	CHK(find_test_app("AppRunTestApp1", &ref) == B_OK);
	// run the remote apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	CHK(runner1.Run("AppRunTestApp1") == B_OK);
	CHK(runner2.Run("AppRunTestApp1") == B_OK);
	// create the BRoster and perform the tests
	BRoster roster;
	app_info info1;
	CHK(roster.GetAppInfo(&ref, &info1) == B_OK);
	CHK(check_app_info(info1, runner1, signature, flags)
		|| check_app_info(info1, runner2, signature, flags));
	// quit app 1
	runner1.WaitFor(true);
	app_info info2;
	CHK(roster.GetAppInfo(&ref, &info2) == B_OK);
	CHK(check_app_info(info2, runner2, signature, flags));
	// quit app 2
	runner2.WaitFor(true);
	CHK(roster.GetAppInfo(&ref, &info1) == B_ERROR);
}

/*
	status_t GetRunningAppInfo(team_id team, app_info *info) const
	@case 1			info is NULL
	@results		Should return B_BAD_VALUE.
*/
void GetAppInfoTester::GetRunningAppInfoTest1()
{
// R5: crashes when passing a NULL app_info
#ifndef TEST_R5
	BRoster roster;
	// invalid team ID
	CHK(roster.GetRunningAppInfo(-1, NULL) == B_BAD_VALUE);
	// valid team ID
	AppRunner runner(true);
	CHK(runner.Run("AppRunTestApp1") == B_OK);
	CHK(roster.GetRunningAppInfo(runner.Team(), NULL) == B_BAD_VALUE);
	runner.WaitFor(true);
#endif
}

/*
	status_t GetRunningAppInfo(team_id team, app_info *info) const
	@case 2			info is not NULL, but no app with the team ID is running
	@results		Should return B_BAD_TEAM_ID.
*/
void GetAppInfoTester::GetRunningAppInfoTest2()
{
	BRoster roster;
	// invalid team ID
	app_info info;
#ifdef TEST_R5
	CHK(roster.GetRunningAppInfo(-1, &info) == B_ERROR);
#else
	CHK(roster.GetRunningAppInfo(-1, &info) == B_BAD_TEAM_ID);
#endif
	CHK(roster.GetRunningAppInfo(-2, &info) == B_BAD_TEAM_ID);
	// originally valid team ID -- app terminates before call
	AppRunner runner(true);
	CHK(runner.Run("AppRunTestApp1") == B_OK);
	team_id team = runner.Team();
	runner.WaitFor(true);
	CHK(roster.GetRunningAppInfo(team, &info) == B_BAD_TEAM_ID);
}

/*
	status_t GetRunningAppInfo(team_id team, app_info *info) const
	@case 3			info is not NULL, and an app with the team ID is running
	@results		Should fill the app info and return B_OK.
*/
void GetAppInfoTester::GetRunningAppInfoTest3()
{
	const char *signature = "application/x-vnd.obos-app-run-testapp1";
	uint32 flags = B_MULTIPLE_LAUNCH | B_ARGV_ONLY;
	// run the app
	AppRunner runner(true);
	CHK(runner.Run("AppRunTestApp1") == B_OK);
	// get and check the info
	BRoster roster;
	app_info info;
	CHK(roster.GetRunningAppInfo(runner.Team(), &info) == B_OK);
	CHK(check_app_info(info, runner, signature, flags));
	// quit the app
	runner.WaitFor(true);
}


Test* GetAppInfoTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BRoster, SuiteOfTests, GetAppInfoTester, GetAppInfoTestA1);
	ADD_TEST4(BRoster, SuiteOfTests, GetAppInfoTester, GetAppInfoTestA2);
	ADD_TEST4(BRoster, SuiteOfTests, GetAppInfoTester, GetAppInfoTestA3);

	ADD_TEST4(BRoster, SuiteOfTests, GetAppInfoTester, GetAppInfoTestB1);
	ADD_TEST4(BRoster, SuiteOfTests, GetAppInfoTester, GetAppInfoTestB2);
	ADD_TEST4(BRoster, SuiteOfTests, GetAppInfoTester, GetAppInfoTestB3);

	ADD_TEST4(BRoster, SuiteOfTests, GetAppInfoTester,
			  GetRunningAppInfoTest1);
	ADD_TEST4(BRoster, SuiteOfTests, GetAppInfoTester,
			  GetRunningAppInfoTest2);
	ADD_TEST4(BRoster, SuiteOfTests, GetAppInfoTester,
			  GetRunningAppInfoTest3);

	return SuiteOfTests;
}

