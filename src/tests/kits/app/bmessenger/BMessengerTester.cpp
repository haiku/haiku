//------------------------------------------------------------------------------
//	BMessengerTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Application.h>
#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>

#define CHK	CPPUNIT_ASSERT

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "BMessengerTester.h"
#include "AppRunner.h"
#include "Helpers.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// check_messenger
static
void
check_messenger(const BMessenger &messenger, bool valid, bool local,
				team_id team, BLooper *looper = NULL, BHandler *handler = NULL,
				team_id altTeam = -1)
{
	CHK(messenger.IsValid() == valid);
	CHK(messenger.IsTargetLocal() == local);
	BLooper *resultLooper = NULL;
	CHK(messenger.Target(&resultLooper) == handler);
	CHK(resultLooper == looper);
	if (altTeam >= 0)
		CHK(messenger.Team() == team || messenger.Team() == altTeam);
	else
{
if (messenger.Team() != team)
printf("team is %ld, but should be %ld\n", messenger.Team(), team);
		CHK(messenger.Team() == team);
}
}

static const char *kRunTestApp1Signature
	= "application/x-vnd.obos-app-run-testapp1";
//static const char *kBMessengerTestApp1Signature
//	= "application/x-vnd.obos-bmessenger-testapp1";


/*
	BMessenger()
	@case 1
	@results		IsValid() should return false.
					IsTargetLocal() should return false.
					Target() should return NULL and NULL for looper.
					Team() should return -1.
 */
void TBMessengerTester::BMessenger1()
{
	BMessenger messenger;
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *looper;
	CHK(messenger.Target(&looper) == NULL);
	CHK(looper == NULL);
	CHK(messenger.Team() == -1);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 1			handler is NULL, looper is NULL, result is NULL
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return -1.
 */
void TBMessengerTester::BMessenger2()
{
	BMessenger messenger((const BHandler*)NULL, NULL, NULL);
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *looper;
	CHK(messenger.Target(&looper) == NULL);
	CHK(looper == NULL);
	CHK(messenger.Team() == -1);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 2			handler is NULL, looper is NULL, result is not NULL
	@results		IsValid() and IsTargetLocal() should return false.
					Target() should return NULL and NULL for looper.
					Team() should return -1.
					result is set to B_BAD_VALUE.
 */
void TBMessengerTester::BMessenger3()
{
	status_t result = B_OK;
	BMessenger messenger((const BHandler*)NULL, NULL, &result);
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *looper;
	CHK(messenger.Target(&looper) == NULL);
	CHK(looper == NULL);
	CHK(messenger.Team() == -1);
	CHK(result == B_BAD_VALUE);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 3			handler is NULL, looper is not NULL, result is not NULL
	@results		IsValid() and IsTargetLocal() should return true.
					Target() should return NULL and the correct value for
					looper.
					Team() should return this team.
					result is set to B_OK.
 */
void TBMessengerTester::BMessenger4()
{
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	BMessenger messenger(NULL, looper, &result);
	CHK(messenger.IsValid() == true);
	CHK(messenger.IsTargetLocal() == true);
	BLooper *resultLooper;
	CHK(messenger.Target(&resultLooper) == NULL);
	CHK(resultLooper == looper);
	CHK(messenger.Team() == get_this_team());
	CHK(result == B_OK);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 4			handler is not NULL, looper is NULL, result is not NULL,
					handler doesn't belong to a looper
	@results		IsValid() and IsTargetLocal() should return false.
					Target() should return NULL and NULL for looper.
					Team() should return -1.
					result is set to B_MISMATCHED_VALUES.
 */
void TBMessengerTester::BMessenger5()
{
	status_t result = B_OK;
	BHandler *handler = new BHandler;
	HandlerDeleter deleter(handler);
	BMessenger messenger(handler, NULL, &result);
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *resultLooper;
	CHK(messenger.Target(&resultLooper) == NULL);
	CHK(resultLooper == NULL);
	CHK(messenger.Team() == -1);
	CHK(result == B_MISMATCHED_VALUES);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 5			handler is not NULL, looper is NULL, result is not NULL
					handler does belong to a looper
	@results		IsValid() and IsTargetLocal() should return true.
					Target() should return the correct handler and the
					handler->Looper() for looper.
					Team() should return this team.
					result is set to B_OK.
 */
void TBMessengerTester::BMessenger6()
{
	// create looper and handler
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	BHandler *handler = new BHandler;
	HandlerDeleter deleter(handler);
	CHK(looper->Lock());
	looper->AddHandler(handler);
	looper->Unlock();
	// create the messenger and do the checks
	BMessenger messenger(handler, NULL, &result);
	CHK(messenger.IsValid() == true);
	CHK(messenger.IsTargetLocal() == true);
	BLooper *resultLooper;
	CHK(messenger.Target(&resultLooper) == handler);
	CHK(resultLooper == looper);
	CHK(messenger.Team() == get_this_team());
//printf("result: %lx\n", result);
	CHK(result == B_OK);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 6			handler is not NULL, looper is not NULL, result is not NULL
					handler does belong to the looper
	@results		IsValid() and IsTargetLocal() should return true.
					Target() should return the correct handler and the correct value
					for looper.
					Team() should return this team.
					result is set to B_OK.
 */
void TBMessengerTester::BMessenger7()
{
	// create looper and handler
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	BHandler *handler = new BHandler;
	HandlerDeleter deleter(handler);
	CHK(looper->Lock());
	looper->AddHandler(handler);
	looper->Unlock();
	// create the messenger and do the checks
	BMessenger messenger(handler, looper, &result);
	CHK(messenger.IsValid() == true);
	CHK(messenger.IsTargetLocal() == true);
	BLooper *resultLooper;
	CHK(messenger.Target(&resultLooper) == handler);
	CHK(resultLooper == looper);
	CHK(messenger.Team() == get_this_team());
//printf("result: %lx\n", result);
	CHK(result == B_OK);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 7			handler is not NULL, looper is not NULL, result is not NULL
					handler does belong to a different looper
	@results		IsValid() and IsTargetLocal() should return false.
					Target() should return NULL and NULL for looper.
					Team() should return -1.
					result is set to B_MISMATCHED_VALUES.
 */
void TBMessengerTester::BMessenger8()
{
	// create loopers and handler
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	BLooper *looper2 = new BLooper;
	looper2->Run();
	LooperQuitter quitter2(looper2);
	BHandler *handler = new BHandler;
	HandlerDeleter deleter(handler);
	CHK(looper->Lock());
	looper->AddHandler(handler);
	looper->Unlock();
	// create the messenger and do the checks
	BMessenger messenger(handler, looper2, &result);
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *resultLooper;
	CHK(messenger.Target(&resultLooper) == NULL);
	CHK(resultLooper == NULL);
	CHK(messenger.Team() == -1);
//printf("result: %lx\n", result);
	CHK(result == B_MISMATCHED_VALUES);
}

/*
	BMessenger(const BMessenger &from)
	@case 1			from is uninitialized
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return -1.
 */
void TBMessengerTester::BMessenger9()
{
	BMessenger from;
	BMessenger messenger(from);
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *looper;
	CHK(messenger.Target(&looper) == NULL);
	CHK(looper == NULL);
	CHK(messenger.Team() == -1);
}


/*
	BMessenger(const BMessenger &from)
	@case 2			from is properly initialized to a local target
	@results		IsValid() and IsTargetLocal() should return true
					Target() should return the same values as for from.
					Team() should return this team.
 */
void TBMessengerTester::BMessenger10()
{
	// create looper and handler
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	BHandler *handler = new BHandler;
	HandlerDeleter deleter(handler);
	CHK(looper->Lock());
	looper->AddHandler(handler);
	looper->Unlock();
	// create the from messenger
	BMessenger from(handler, looper, &result);
	CHK(from.IsValid() == true);
	CHK(from.IsTargetLocal() == true);
	BLooper *resultLooper;
	CHK(from.Target(&resultLooper) == handler);
	CHK(resultLooper == looper);
	CHK(from.Team() == get_this_team());
	CHK(result == B_OK);
	// create the messenger and do the checks
	BMessenger messenger(from);
	CHK(messenger.IsValid() == true);
	CHK(messenger.IsTargetLocal() == true);
	resultLooper = NULL;
	CHK(messenger.Target(&resultLooper) == handler);
	CHK(resultLooper == looper);
	CHK(messenger.Team() == get_this_team());
}


/*
	BMessenger(const char *signature, team_id team, status_t *result)
	@case 1			signature is NULL, team is -1, result is (not) NULL
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return -1.
					(result should be set to B_BAD_TYPE.)
 */
void TBMessengerTester::BMessengerD1()
{
	BApplication app("application/x-vnd.obos-bmessenger-test");
	// run the remote app
	AppRunner runner(true);
	CHK(runner.Run("AppRunTestApp1") == B_OK);
	// create and check the messengers
	BMessenger messenger1(NULL, -1, NULL);
	check_messenger(messenger1, false, false, -1);
	status_t error;
	BMessenger messenger2(NULL, -1, &error);
	check_messenger(messenger2, false, false, -1);
	CHK(error == B_BAD_TYPE);
	// quit the remote app
	runner.WaitFor(true);
}

/*
	BMessenger(const char *signature, team_id team, status_t *result)
	@case 2			signature is not NULL, but identifies no running
					application, team is -1, result is (not) NULL
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return -1.
					(result should be set to B_BAD_VALUE.)
 */
void TBMessengerTester::BMessengerD2()
{
	// create and check the messengers
	BMessenger messenger1(kRunTestApp1Signature, -1, NULL);
	check_messenger(messenger1, false, false, -1);
	status_t error;
	BMessenger messenger2(kRunTestApp1Signature, -1, &error);
	check_messenger(messenger2, false, false, -1);
	CHK(error == B_BAD_VALUE);
}

/*
	BMessenger(const char *signature, team_id team, status_t *result)
	@case 3			signature is NULL, team is > 0, but identifies no running
					application, result is (not) NULL
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return -1.
					(result should be set to B_BAD_TEAM_ID.)
 */
void TBMessengerTester::BMessengerD3()
{
	BApplication app("application/x-vnd.obos-bmessenger-test");
	// run the remote app
	AppRunner runner(true);
	CHK(runner.Run("AppRunTestApp1") == B_OK);
	team_id team = runner.Team();
	// quit the remote app
	runner.WaitFor(true);
	snooze(10000);
	// create and check the messengers
	BMessenger messenger1(NULL, team, NULL);
	check_messenger(messenger1, false, false, -1);
	status_t error;
	BMessenger messenger2(NULL, team, &error);
	check_messenger(messenger2, false, false, -1);
	CHK(error == B_BAD_TEAM_ID);
}

/*
	BMessenger(const char *signature, team_id team, status_t *result)
	@case 4			signature is not NULL and identifies a running B_ARGV_ONLY
					application, team is -1, result is (not) NULL
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return the remote app's team ID.
					(result should be set to B_BAD_TYPE.)
 */
void TBMessengerTester::BMessengerD4()
{
	BApplication app("application/x-vnd.obos-bmessenger-test");
	// run the remote app
	AppRunner runner(true);
	CHK(runner.Run("AppRunTestApp1") == B_OK);
	// create and check the messengers
	BMessenger messenger1(kRunTestApp1Signature, -1, NULL);
	check_messenger(messenger1, false, false, runner.Team());
	status_t error;
	BMessenger messenger2(kRunTestApp1Signature, -1, &error);
	check_messenger(messenger2, false, false, runner.Team());
	CHK(error == B_BAD_TYPE);
	// quit the remote app
	runner.WaitFor(true);
}

/*
	BMessenger(const char *signature, team_id team, status_t *result)
	@case 5			signature is NULL,
					team is > 0 and identifies a running B_ARGV_ONLY
					application, result is (not) NULL
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return the remote app's team ID.
					(result should be set to B_BAD_TYPE.)
 */
void TBMessengerTester::BMessengerD5()
{
	BApplication app("application/x-vnd.obos-bmessenger-test");
	// run the remote app
	AppRunner runner(true);
	CHK(runner.Run("AppRunTestApp1") == B_OK);
	// create and check the messengers
	BMessenger messenger1(NULL, runner.Team(), NULL);
	check_messenger(messenger1, false, false, runner.Team());
	status_t error;
	BMessenger messenger2(NULL, runner.Team(), &error);
	check_messenger(messenger2, false, false, runner.Team());
	CHK(error == B_BAD_TYPE);
	// quit the remote app
	runner.WaitFor(true);
}

/*
	BMessenger(const char *signature, team_id team, status_t *result)
	@case 6			signature is not NULL and identifies a "normal" running
					application, team is -1, result is (not) NULL
	@results		IsValid() should return true
					IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return the team ID of the remote application.
					(result should be set to B_OK.)
 */
void TBMessengerTester::BMessengerD6()
{
	BApplication app("application/x-vnd.obos-bmessenger-test");
	// run the remote apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	CHK(runner1.Run("AppRunTestApp2") == B_OK);
	CHK(runner2.Run("AppRunTestApp2") == B_OK);
	// create and check the messengers
	BMessenger messenger1(kRunTestApp1Signature, -1, NULL);
	check_messenger(messenger1, true, false, runner1.Team(), NULL, NULL,
					runner2.Team());
	status_t error;
	BMessenger messenger2(kRunTestApp1Signature, -1, &error);
	check_messenger(messenger2, true, false, runner1.Team(), NULL, NULL,
					runner2.Team());
	CHK(error == B_OK);
	// quit the remote apps
	runner1.WaitFor(true);
	runner2.WaitFor(true);
}

/*
	BMessenger(const char *signature, team_id team, status_t *result)
	@case 7			signature is NULL,
					team is > 0 and identifies a "normal" running application,
					result is (not) NULL
	@results		IsValid() should return true
					IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return the team ID of the remote application (team).
					(result should be set to B_OK.)
 */
void TBMessengerTester::BMessengerD7()
{
	BApplication app("application/x-vnd.obos-bmessenger-test");
	// run the remote apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	CHK(runner1.Run("AppRunTestApp2") == B_OK);
	CHK(runner2.Run("AppRunTestApp2") == B_OK);
	// create and check the messengers
	BMessenger messenger1(NULL, runner1.Team(), NULL);
	check_messenger(messenger1, true, false, runner1.Team());
	status_t error;
	BMessenger messenger2(NULL, runner1.Team(), &error);
	check_messenger(messenger2, true, false, runner1.Team());
	CHK(error == B_OK);
	// quit the remote apps
	runner1.WaitFor(true);
	runner2.WaitFor(true);
	snooze(10000);
	// check the messengers again
	check_messenger(messenger1, false, false, runner1.Team());
	check_messenger(messenger2, false, false, runner1.Team());
}

/*
	BMessenger(const char *signature, team_id team, status_t *result)
	@case 8			signature is not NULL and team is > 0, but both identify
					different applications, result is (not) NULL
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return -1.
					(result should be set to B_MISMATCHED_VALUES.)
 */
void TBMessengerTester::BMessengerD8()
{
	BApplication app("application/x-vnd.obos-bmessenger-test");
	// run the remote apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	CHK(runner1.Run("BMessengerTestApp1") == B_OK);
	CHK(runner2.Run("AppRunTestApp2") == B_OK);
	// create and check the messengers
	BMessenger messenger1(kRunTestApp1Signature, runner1.Team(), NULL);
	check_messenger(messenger1, false, false, -1);
	status_t error;
	BMessenger messenger2(kRunTestApp1Signature, runner1.Team(), &error);
	check_messenger(messenger2, false, false, -1);
	CHK(error == B_MISMATCHED_VALUES);
	// quit the remote apps
	runner1.WaitFor(true);
	runner2.WaitFor(true);
}

/*
	BMessenger(const char *signature, team_id team, status_t *result)
	@case 9			signature is not NULL, team is > 0 and both identify the
					same application (more than one app with the given
					signature are running), result is (not) NULL
	@results		IsValid() should return true
					IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return the team ID of the remote application (team).
					(result should be set to B_OK.)
 */
void TBMessengerTester::BMessengerD9()
{
	BApplication app("application/x-vnd.obos-bmessenger-test");
	// run the remote apps
	AppRunner runner1(true);
	AppRunner runner2(true);
	CHK(runner1.Run("AppRunTestApp2") == B_OK);
	CHK(runner2.Run("AppRunTestApp2") == B_OK);
	// create and check the messengers
	BMessenger messenger1(kRunTestApp1Signature, runner1.Team(), NULL);
	check_messenger(messenger1, true, false, runner1.Team());
	status_t error;
	BMessenger messenger2(kRunTestApp1Signature, runner1.Team(), &error);
	check_messenger(messenger2, true, false, runner1.Team());
	CHK(error == B_OK);
	BMessenger messenger3(kRunTestApp1Signature, runner2.Team(), NULL);
	check_messenger(messenger3, true, false, runner2.Team());
	BMessenger messenger4(kRunTestApp1Signature, runner2.Team(), &error);
	check_messenger(messenger4, true, false, runner2.Team());
	CHK(error == B_OK);
	// quit the remote apps
	runner1.WaitFor(true);
	runner2.WaitFor(true);
}


Test* TBMessengerTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessenger1);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessenger2);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessenger3);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessenger4);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessenger5);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessenger6);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessenger7);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessenger8);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessenger9);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessenger10);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessengerD1);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessengerD2);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessengerD3);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessengerD4);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessengerD5);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessengerD6);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessengerD7);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessengerD8);
	ADD_TEST4(BMessenger, SuiteOfTests, TBMessengerTester, BMessengerD9);

	return SuiteOfTests;
}



