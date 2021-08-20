//------------------------------------------------------------------------------
//	SetCountTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <algorithm>
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Application.h>
#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <OS.h>

// Project Includes ------------------------------------------------------------
#include <TestShell.h>
#include <TestUtils.h>
#include <cppunit/TestAssert.h>

// Local Includes --------------------------------------------------------------
#include "MessageRunnerTestHelpers.h"
#include "SetCountTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------


using namespace std;


static const char *kTesterSignature
	= "application/x-vnd.obos-messagerunner-setcount-test";

static const bigtime_t kMinTimeInterval = 50000;

// check_message_runner_info
static
void
check_message_runner_info(const BMessageRunner &runner, status_t error,
						  bigtime_t interval = 0, int32 count = 0)
{
	bigtime_t runnerInterval = 0;
	int32 runnerCount = 0;
	CHK(runner.GetInfo(&runnerInterval, &runnerCount) == error);
	if (error == B_OK) {
		CHK(runnerInterval == interval);
		CHK(runnerCount == count);
	}
}

/*
	status_t SetCount(int32 count)
	@case 1			object is not properly initialized, count > 0
	@results		Should return B_BAD_VALUE.
					InitCheck() should return B_ERROR.
					GetInfo() should return B_BAD_VALUE.
 */
void SetCountTester::SetCount1()
{
	MessageRunnerTestApp app(kTesterSignature);
	BMessenger target;
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 0;
	BMessageRunner runner(target, &message, interval, count);
	CHK(runner.InitCheck() == B_ERROR);
	check_message_runner_info(runner, B_BAD_VALUE);
	int32 newCount = 100000;
	CHK(runner.SetCount(newCount) == B_BAD_VALUE);
	CHK(runner.InitCheck() == B_ERROR);
	check_message_runner_info(runner, B_BAD_VALUE);
}

/*
	status_t SetCount(int32 count)
	@case 2			object was properly initialized, but has already delivered
					all its messages and thus became unusable, count > 0
	@results		Should return B_BAD_VALUE.
					InitCheck() should return B_OK.
					GetInfo() should return B_BAD_VALUE.
 */
void SetCountTester::SetCount2()
{
	MessageRunnerTestApp app(kTesterSignature);
	BMessenger target;
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 50000;
	int32 count = 1;
	BMessageRunner runner(target, &message, interval, count);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze(count * interval + 10000);
	// set new count
	int32 newCount = 100000;
	CHK(runner.SetCount(newCount) == B_BAD_VALUE);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_BAD_VALUE);
}

/*
	status_t SetCount(int32 count)
	@case 3			object is properly initialized and has still one message
					to deliver, count > 0
	@results		Should return B_OK.
					InitCheck() should return B_OK.
					GetInfo() should return B_OK and the new count.
					The timer is NOT reset. count messages should arrive.
 */
void SetCountTester::SetCount3()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 50000;
	int32 count = 5;
	BMessageRunner runner(target, &message, interval, count);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze((count - 1) * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, count - 1));
	CHK(app.CountReplies() == count - 1);
	// set new count
	bigtime_t newCount = 3;
	CHK(runner.SetCount(newCount) == B_OK);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_OK, interval, newCount);
//	startTime += system_time();
	startTime += (count - 1) * interval;
	snooze((newCount + 1) * interval + 10000);
	CHK(looper->CheckMessages(count - 1, startTime, interval, newCount));
	CHK(app.CountReplies() == count - 1 + newCount);
}

/*
	status_t SetCount(int32 count)
	@case 4			object is properly initialized and has still some messages
					to deliver, count > 0
	@results		Should return B_OK.
					InitCheck() should return B_OK.
					GetInfo() should return B_OK and the new count.
					The timer is NOT reset. count messages should arrive.
 */
void SetCountTester::SetCount4()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 50000;
	int32 count = 5;
	BMessageRunner runner(target, &message, interval, count);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze(1 * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, 1));
	CHK(app.CountReplies() == 1);
	// set new count
	bigtime_t newCount = 3;
	CHK(runner.SetCount(newCount) == B_OK);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_OK, interval, newCount);
	startTime += 1 * interval;
	snooze((newCount + 1) * interval + 10000);
	CHK(looper->CheckMessages(1, startTime, interval, newCount));
	CHK(app.CountReplies() == 1 + newCount);
}

/*
	status_t SetCount(int32 count)
	@case 5			object is properly initialized and has still an unlimited
					number of messages to deliver, count > 0
	@results		Should return B_OK.
					InitCheck() should return B_OK.
					GetInfo() should return B_OK and the new count.
					The timer is NOT reset. count messages should arrive.
 */
void SetCountTester::SetCount5()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 50000;
	int32 count = -1;
	BMessageRunner runner(target, &message, interval, count);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze(1 * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, 1));
	CHK(app.CountReplies() == 1);
	// set new count
	bigtime_t newCount = 3;
	CHK(runner.SetCount(newCount) == B_OK);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_OK, interval, newCount);
	startTime += 1 * interval;
	snooze((newCount + 1) * interval + 10000);
	CHK(looper->CheckMessages(1, startTime, interval, newCount));
	CHK(app.CountReplies() == 1 + newCount);
}

/*
	status_t SetCount(int32 count)
	@case 6			object is properly initialized and has still some messages
					to deliver, count == 0
	@results		Should return B_OK.
					InitCheck() should return B_OK.
					R5: GetInfo() should return B_OK and count 0!
						The timer is NOT reset and a message arives after the
						time specified by the interval!
					Haiku: GetInfo() should return B_BAD_VALUE.
 */
void SetCountTester::SetCount6()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 70000;
	int32 count = 5;
	BMessageRunner runner(target, &message, interval, count);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze(1 * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, 1));
	CHK(app.CountReplies() == 1);
	// set new count
	bigtime_t newCount = 0;
	CHK(runner.SetCount(newCount) == B_OK);
	CHK(runner.InitCheck() == B_OK);
	startTime += 1 * interval;
#ifdef TEST_R5
	check_message_runner_info(runner, B_OK, interval, newCount);
	snooze((newCount + 1) * interval + 10000);
	CHK(looper->CheckMessages(1, startTime, interval, 1));
	CHK(app.CountReplies() == 1 + 1);
#else
	check_message_runner_info(runner, B_BAD_VALUE);
	snooze(1 * interval + 10000);
	CHK(looper->CheckMessages(1, startTime, interval, 0));
	CHK(app.CountReplies() == 1);
#endif
}

/*
	status_t SetCount(int32 count)
	@case 7			object is properly initialized and has still some messages
					to deliver, count < 0
	@results		Should return B_OK.
					InitCheck() should return B_OK.
					GetInfo() should return B_OK and the new count.
					The timer is NOT reset. Unlimited number of messages.
 */
void SetCountTester::SetCount7()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 70000;
	int32 count = 5;
	BMessageRunner runner(target, &message, interval, count);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze(1 * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, 1));
	CHK(app.CountReplies() == 1);
	// set new count
	bigtime_t newCount = -1;
	CHK(runner.SetCount(newCount) == B_OK);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_OK, interval, newCount);
	startTime += 1 * interval;
	int32 checkCount = 5;
	snooze(checkCount * interval + 10000);
	CHK(looper->CheckMessages(1, startTime, interval, checkCount));
	CHK(app.CountReplies() == 1 + checkCount);
}


Test* SetCountTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BMessageRunner, SuiteOfTests, SetCountTester, SetCount1);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetCountTester, SetCount2);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetCountTester, SetCount3);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetCountTester, SetCount4);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetCountTester, SetCount5);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetCountTester, SetCount6);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetCountTester, SetCount7);

	return SuiteOfTests;
}

