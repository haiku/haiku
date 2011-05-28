//------------------------------------------------------------------------------
//	BMessageRunnerTester.cpp
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
#include "BMessageRunnerTester.h"
#include "MessageRunnerTestHelpers.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------


using namespace std;


static const char *kTesterSignature
	= "application/x-vnd.obos-messagerunner-constructor-test";

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
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count)
	@case 1			target is invalid, message is valid, interval > 0,
					count > 0
	@results		InitCheck() should return B_OK. The message runner turns
					to unusable as soon as the first message had to be sent.
					GetInfo() should return B_OK.
 */
void TBMessageRunnerTester::BMessageRunnerA1()
{
	MessageRunnerTestApp app(kTesterSignature);
	BMessenger target;
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 5;
	BMessageRunner runner(target, &message, interval, count);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze(interval + 10000);
	check_message_runner_info(runner, B_BAD_VALUE);
	CHK(app.CountReplies() == 0);
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count)
	@case 2			target is valid, message is NULL, interval > 0, count > 0
	@results		InitCheck() should return B_BAD_VALUE.
					GetInfo() should return B_BAD_VALUE.
 */
void TBMessageRunnerTester::BMessageRunnerA2()
{
// R5: chrashes when passing a NULL message
#ifndef TEST_R5
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	bigtime_t interval = 100000;
	int32 count = 5;
	BMessageRunner runner(target, NULL, interval, count);
	CHK(runner.InitCheck() == B_BAD_VALUE);
	check_message_runner_info(runner, B_BAD_VALUE);
#endif
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count)
	@case 3			target is valid, message is valid, interval == 0, count > 0
	@results		R5: InitCheck() should return B_ERROR.
						GetInfo() should return B_BAD_VALUE.
					OBOS: InitCheck() should return B_OK.
						  GetInfo() should return B_OK.
						  A minimal time interval is used (50000).
 */
void TBMessageRunnerTester::BMessageRunnerA3()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 0;
	int32 count = 5;
	BMessageRunner runner(target, &message, interval, count);
#ifdef TEST_R5
	CHK(runner.InitCheck() == B_ERROR);
	check_message_runner_info(runner, B_BAD_VALUE);
#else
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze((count + 1) * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, count));
	CHK(app.CountReplies() == count);
#endif
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count)
	@case 4			target is valid, message is valid, interval < 0, count > 0
	@results		InitCheck() should return B_OK.
					GetInfo() should return B_OK.
					A minimal time interval is used (50000).
 */
void TBMessageRunnerTester::BMessageRunnerA4()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = -1;
	int32 count = 5;
	BMessageRunner runner(target, &message, interval, count);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze((count + 1) * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, count));
	CHK(app.CountReplies() == count);
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count)
	@case 5			target is valid, message is valid,
					interval == LONGLONG_MAX, count > 0
	@results		InitCheck() should return B_OK.
					GetInfo() should return B_OK.
					No message should be delivered.
 */
void TBMessageRunnerTester::BMessageRunnerA5()
{
// R5: doesn't behave very well. In worst case registrar time loop gets
// locked up and system wide message runners don't get messages anymore.
#ifndef TEST_R5
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = LONGLONG_MAX;
	int32 count = 5;
	BMessageRunner runner(target, &message, interval, count);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze(10000);
	CHK(looper->CheckMessages(startTime, interval, 0));
	CHK(app.CountReplies() == 0);
#endif
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count)
	@case 6			target is valid, message is valid, interval > 0, count == 0
	@results		InitCheck() should return B_ERROR.
					GetInfo() should return B_BAD_VALUE.
 */
void TBMessageRunnerTester::BMessageRunnerA6()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 0;
	BMessageRunner runner(target, &message, interval, count);
	CHK(runner.InitCheck() == B_ERROR);
	check_message_runner_info(runner, B_BAD_VALUE);
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count)
	@case 7			target is valid, message is valid, interval > 0, count < 0
	@results		InitCheck() should return B_OK.
					GetInfo() should return B_OK.
					Unlimited number of messages.
 */
void TBMessageRunnerTester::BMessageRunnerA7()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 5;
	BMessageRunner runner(target, &message, interval, count);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	int32 checkCount = 5;
	snooze(checkCount * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, checkCount));
	CHK(app.CountReplies() == checkCount);
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count)
	@case 8			target is valid, message is valid, interval > 0, count > 0
	@results		InitCheck() should return B_OK.
					GetInfo() should return B_OK.
					count messages are sent.
 */
void TBMessageRunnerTester::BMessageRunnerA8()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 5;
	BMessageRunner runner(target, &message, interval, count);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze((count + 1) * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, count));
	CHK(app.CountReplies() == count);
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count, BMessenger replyTo)
	@case 1			target is invalid, message is valid, interval > 0,
					count > 0
	@results		InitCheck() should return B_OK. The message runner turns
					to unusable as soon as the first message had to be sent.
					GetInfo() should return B_OK.
 */
void TBMessageRunnerTester::BMessageRunnerB1()
{
	MessageRunnerTestApp app(kTesterSignature);
	BMessenger target;
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 5;
	MessageRunnerTestHandler *handler = app.TestHandler();
	BMessenger replyTo(handler);
	BMessageRunner runner(target, &message, interval, count, replyTo);
	CHK(runner.InitCheck() == B_OK);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze(interval + 10000);
	check_message_runner_info(runner, B_BAD_VALUE);
	CHK(app.CountReplies() == 0);
	CHK(handler->CountReplies() == 0);
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count, BMessenger replyTo)
	@case 2			target is valid, message is NULL, interval > 0, count > 0
	@results		InitCheck() should return B_BAD_VALUE.
					GetInfo() should return B_BAD_VALUE.
 */
void TBMessageRunnerTester::BMessageRunnerB2()
{
// R5: chrashes when passing a NULL message
#ifndef TEST_R5
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	bigtime_t interval = 100000;
	int32 count = 5;
	MessageRunnerTestHandler *handler = app.TestHandler();
	BMessenger replyTo(handler);
	BMessageRunner runner(target, NULL, interval, count, replyTo);
	CHK(runner.InitCheck() == B_BAD_VALUE);
	check_message_runner_info(runner, B_BAD_VALUE);
#endif
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count, BMessenger replyTo)
	@case 3			target is valid, message is valid, interval == 0, count > 0
	@results		R5: InitCheck() should return B_ERROR.
						GetInfo() should return B_BAD_VALUE.
					OBOS: InitCheck() should return B_OK.
						  GetInfo() should return B_OK.
						  A minimal time interval is used (50000).
 */
void TBMessageRunnerTester::BMessageRunnerB3()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 0;
	int32 count = 5;
	MessageRunnerTestHandler *handler = app.TestHandler();
	BMessenger replyTo(handler);
	BMessageRunner runner(target, &message, interval, count, replyTo);
#ifdef TEST_R5
	CHK(runner.InitCheck() == B_ERROR);
	check_message_runner_info(runner, B_BAD_VALUE);
#else
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze((count + 1) * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, count));
	CHK(app.CountReplies() == 0);
	CHK(handler->CountReplies() == count);
#endif
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count, BMessenger replyTo)
	@case 4			target is valid, message is valid, interval < 0, count > 0
	@results		InitCheck() should return B_OK.
					GetInfo() should return B_OK.
					A minimal time interval is used (50000).
 */
void TBMessageRunnerTester::BMessageRunnerB4()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = -1;
	int32 count = 5;
	MessageRunnerTestHandler *handler = app.TestHandler();
	BMessenger replyTo(handler);
	BMessageRunner runner(target, &message, interval, count, replyTo);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze((count + 1) * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, count));
	CHK(app.CountReplies() == 0);
	CHK(handler->CountReplies() == count);
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count, BMessenger replyTo)
	@case 5			target is valid, message is valid,
					interval == LONGLONG_MAX, count > 0
	@results		InitCheck() should return B_OK.
					GetInfo() should return B_OK.
					No message should be delivered.
 */
void TBMessageRunnerTester::BMessageRunnerB5()
{
// R5: doesn't behave very well. In worst case registrar time loop gets
// locked up and system wide message runners don't get messages anymore.
#ifndef TEST_R5
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = LONGLONG_MAX;
	int32 count = 5;
	MessageRunnerTestHandler *handler = app.TestHandler();
	BMessenger replyTo(handler);
	BMessageRunner runner(target, &message, interval, count, replyTo);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze(10000);
	CHK(looper->CheckMessages(startTime, interval, 0));
	CHK(app.CountReplies() == 0);
	CHK(handler->CountReplies() == 0);
#endif
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count, BMessenger replyTo)
	@case 6			target is valid, message is valid, interval > 0, count == 0
	@results		InitCheck() should return B_ERROR.
					GetInfo() should return B_BAD_VALUE.
 */
void TBMessageRunnerTester::BMessageRunnerB6()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 0;
	MessageRunnerTestHandler *handler = app.TestHandler();
	BMessenger replyTo(handler);
	BMessageRunner runner(target, &message, interval, count, replyTo);
	CHK(runner.InitCheck() == B_ERROR);
	check_message_runner_info(runner, B_BAD_VALUE);
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count, BMessenger replyTo)
	@case 7			target is valid, message is valid, interval > 0, count < 0
	@results		InitCheck() should return B_OK.
					GetInfo() should return B_OK.
					Unlimited number of messages.
 */
void TBMessageRunnerTester::BMessageRunnerB7()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 5;
	MessageRunnerTestHandler *handler = app.TestHandler();
	BMessenger replyTo(handler);
	BMessageRunner runner(target, &message, interval, count, replyTo);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	int32 checkCount = 5;
	snooze(checkCount * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, checkCount));
	CHK(app.CountReplies() == 0);
	CHK(handler->CountReplies() == checkCount);
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count, BMessenger replyTo)
	@case 8			target is valid, message is valid, interval > 0, count > 0
	@results		InitCheck() should return B_OK.
					GetInfo() should return B_OK.
					count messages are sent.
 */
void TBMessageRunnerTester::BMessageRunnerB8()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 5;
	MessageRunnerTestHandler *handler = app.TestHandler();
	BMessenger replyTo(handler);
	BMessageRunner runner(target, &message, interval, count, replyTo);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze((count + 1) * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, count));
	CHK(app.CountReplies() == 0);
	CHK(handler->CountReplies() == count);
}

/*
	BMessageRunner(BMessenger target, const BMessage *message,
				   bigtime_t interval, int32 count, BMessenger replyTo)
	@case 9			target is valid, message is valid, interval > 0, count > 0,
					replyTo is invalid
	@results		InitCheck() should return B_OK.
					GetInfo() should return B_OK.
					count messages are sent. The replies go to the registrar!
 */
void TBMessageRunnerTester::BMessageRunnerB9()
{
	MessageRunnerTestApp app(kTesterSignature);
	MessageRunnerTestLooper *looper = app.TestLooper();
	BMessenger target(looper);
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 5;
	MessageRunnerTestHandler *handler = app.TestHandler();
	BMessenger replyTo(handler);
	BMessageRunner runner(target, &message, interval, count, replyTo);
	bigtime_t startTime = system_time();
	CHK(runner.InitCheck() == B_OK);
	interval = max(interval, kMinTimeInterval);
	check_message_runner_info(runner, B_OK, interval, count);
	snooze((count + 1) * interval + 10000);
	CHK(looper->CheckMessages(startTime, interval, count));
	CHK(app.CountReplies() == 0);
	CHK(handler->CountReplies() == count);
}


Test* TBMessageRunnerTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerA1);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerA2);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerA3);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerA4);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerA5);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerA6);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerA7);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerA8);

	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerB1);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerB2);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerB3);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerB4);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerB5);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerB6);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerB7);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerB8);
	ADD_TEST4(BMessageRunner, SuiteOfTests, TBMessageRunnerTester,
			  BMessageRunnerB9);

	return SuiteOfTests;
}

