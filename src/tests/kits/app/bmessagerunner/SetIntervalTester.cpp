//------------------------------------------------------------------------------
//	SetIntervalTester.cpp
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
#include "SetIntervalTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------


using namespace std;


static const char *kTesterSignature
	= "application/x-vnd.obos-messagerunner-setinterval-test";

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
	status_t SetInterval(bigtime_t interval)
	@case 1			object is not properly initialized, interval > 0
	@results		Should return B_BAD_VALUE.
					InitCheck() should return B_ERROR.
					GetInfo() should return B_BAD_VALUE.
 */
void SetIntervalTester::SetInterval1()
{
	MessageRunnerTestApp app(kTesterSignature);
	BMessenger target;
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 0;
	BMessageRunner runner(target, &message, interval, count);
	CHK(runner.InitCheck() == B_ERROR);
	check_message_runner_info(runner, B_BAD_VALUE);
	bigtime_t newInterval = 100000;
	CHK(runner.SetInterval(newInterval) == B_BAD_VALUE);
	CHK(runner.InitCheck() == B_ERROR);
	check_message_runner_info(runner, B_BAD_VALUE);
}

/*
	status_t SetInterval(bigtime_t interval)
	@case 2			object was properly initialized, but has already delivered
					all its messages and thus became unusable, interval > 0
	@results		Should return B_BAD_VALUE.
					InitCheck() should return B_OK.
					GetInfo() should return B_BAD_VALUE.
 */
void SetIntervalTester::SetInterval2()
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
	// set new interval
	bigtime_t newInterval = 100000;
	CHK(runner.SetInterval(newInterval) == B_BAD_VALUE);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_BAD_VALUE);
}

/*
	status_t SetInterval(bigtime_t interval)
	@case 3			object is properly initialized and has still one message
					to deliver, interval > 0
	@results		Should return B_OK.
					InitCheck() should return B_OK.
					GetInfo() should return B_OK and the new interval.
					The timer is reset. The last message arives after the time
					specified by interval has passed.
 */
void SetIntervalTester::SetInterval3()
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
	// set new interval
	bigtime_t newInterval = 100000;
	CHK(runner.SetInterval(newInterval) == B_OK);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_OK, newInterval, 1);
	startTime = system_time();
	snooze(2 * newInterval + 10000);
	CHK(looper->CheckMessages(count - 1, startTime, newInterval, 1));
	CHK(app.CountReplies() == count);
}

/*
	status_t SetInterval(bigtime_t interval)
	@case 4			object is properly initialized and has still some messages
					to deliver, interval > 0
	@results		Should return B_OK.
					InitCheck() should return B_OK.
					GetInfo() should return B_OK and the new interval.
					The timer is reset. The messages start to arive after
					the time specified by interval.
 */
void SetIntervalTester::SetInterval4()
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
	// set new interval
	bigtime_t newInterval = 70000;
	CHK(runner.SetInterval(newInterval) == B_OK);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_OK, newInterval, count - 1);
	startTime = system_time();
	snooze(count * newInterval + 10000);
	CHK(looper->CheckMessages(1, startTime, newInterval, count - 1));
	CHK(app.CountReplies() == count);
}

/*
	status_t SetInterval(bigtime_t interval)
	@case 5			object is properly initialized and has still an unlimited
					number of messages to deliver, interval > 0
	@results		Should return B_OK.
					InitCheck() should return B_OK.
					GetInfo() should return B_OK and the new interval.
					The timer is reset. The messages start to arive after
					the time specified by interval.
 */
void SetIntervalTester::SetInterval5()
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
	// set new interval
	bigtime_t newInterval = 70000;
	CHK(runner.SetInterval(newInterval) == B_OK);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_OK, newInterval, -1);
	startTime = system_time();
	int32 checkCount = 5;
	snooze(checkCount * newInterval + 10000);
	CHK(looper->CheckMessages(1, startTime, newInterval, checkCount));
	CHK(app.CountReplies() == checkCount + 1);
}

/*
	status_t SetInterval(bigtime_t interval)
	@case 6			object is properly initialized and has still some messages
					to deliver, interval == 0
	@results		Should return B_OK.
					InitCheck() should return B_OK.
					R5: GetInfo() should return B_BAD_VALUE.
						All messages are delivered, but at weird times.
					Haiku: GetInfo() should return B_OK and the minimal
						  interval. The timer is reset. The messages start to
						  arive after the time specified by the minimal
						  interval.
 */
void SetIntervalTester::SetInterval6()
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
	// set new interval
	bigtime_t newInterval = 0;
	CHK(runner.SetInterval(newInterval) == B_OK);
	newInterval = max(newInterval, kMinTimeInterval);
	CHK(runner.InitCheck() == B_OK);
#ifdef TEST_R5
	check_message_runner_info(runner, B_BAD_VALUE);
	snooze(count * interval + 10000);
#else
	check_message_runner_info(runner, B_OK, newInterval, count - 1);
	startTime = system_time();
	snooze(count * newInterval + 10000);
	CHK(looper->CheckMessages(1, startTime, newInterval, count - 1));
#endif
	CHK(app.CountReplies() == count);
}

/*
	status_t SetInterval(bigtime_t interval)
	@case 7			object is properly initialized and has still some messages
					to deliver, interval < 0
	@results		Should return B_OK.
					InitCheck() should return B_OK.
					R5: GetInfo() should return B_BAD_VALUE.
						All messages are delivered, but at weird times.
					Haiku: GetInfo() should return B_OK and the minimal
						  interval. The timer is reset. The messages start to
						  arive after the time specified by the minimal
						  interval.
 */
void SetIntervalTester::SetInterval7()
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
	// set new interval
	bigtime_t newInterval = -1;
	CHK(runner.SetInterval(newInterval) == B_OK);
	newInterval = max(newInterval, kMinTimeInterval);
	CHK(runner.InitCheck() == B_OK);
#ifdef TEST_R5
	check_message_runner_info(runner, B_BAD_VALUE);
	snooze(count * interval + 10000);
#else
	check_message_runner_info(runner, B_OK, newInterval, count - 1);
	startTime = system_time();
	snooze(count * newInterval + 10000);
	CHK(looper->CheckMessages(1, startTime, newInterval, count - 1));
#endif
	CHK(app.CountReplies() == count);
}


Test* SetIntervalTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BMessageRunner, SuiteOfTests, SetIntervalTester, SetInterval1);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetIntervalTester, SetInterval2);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetIntervalTester, SetInterval3);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetIntervalTester, SetInterval4);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetIntervalTester, SetInterval5);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetIntervalTester, SetInterval6);
	ADD_TEST4(BMessageRunner, SuiteOfTests, SetIntervalTester, SetInterval7);

	return SuiteOfTests;
}

