//------------------------------------------------------------------------------
//	GetInfoTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
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
#include "GetInfoTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

#ifndef TEST_R5
static const char *kTesterSignature
	= "application/x-vnd.obos-messagerunner-getinfo-test";
#endif

static const bigtime_t kMinTimeInterval = 50000;

/*
	status_t GetInfo(bigtime_t *interval, int32 *count) const
	@case 1			object is properly initialized, interval or count are NULL
	@results		Should return B_OK.
					InitCheck() should return B_OK.
 */
void GetInfoTester::GetInfo1()
{
// R5: crashes when passing a NULL parameter.
#ifndef TEST_R5
	MessageRunnerTestApp app(kTesterSignature);
	BMessenger target;
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 5;
	BMessageRunner runner(target, &message, interval, count);
	CHK(runner.InitCheck() == B_OK);
	bigtime_t readInterval = 0;
	int32 readCount = 0;
	CHK(runner.GetInfo(&readInterval, NULL) == B_OK);
	CHK(readInterval == interval);
	CHK(runner.GetInfo(NULL, &readCount) == B_OK);
	CHK(readCount == count);
	CHK(runner.GetInfo(NULL, NULL) == B_OK);
#endif
}

/*
	status_t GetInfo(bigtime_t *interval, int32 *count) const
	@case 2			object is not properly initialized, interval or count are
					NULL
	@results		Should return B_BAD_VALUE.
					InitCheck() should return B_ERROR.
 */
void GetInfoTester::GetInfo2()
{
// R5: crashes when passing a NULL parameter.
#ifndef TEST_R5
	MessageRunnerTestApp app(kTesterSignature);
	BMessenger target;
	BMessage message(MSG_RUNNER_MESSAGE);
	bigtime_t interval = 100000;
	int32 count = 0;
	BMessageRunner runner(target, &message, interval, count);
	CHK(runner.InitCheck() == B_ERROR);
	bigtime_t readInterval = 0;
	int32 readCount = 0;
	CHK(runner.GetInfo(&readInterval, NULL) == B_BAD_VALUE);
	CHK(runner.GetInfo(NULL, &readCount) == B_BAD_VALUE);
	CHK(runner.GetInfo(NULL, NULL) == B_BAD_VALUE);
#endif
}


Test* GetInfoTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BMessageRunner, SuiteOfTests, GetInfoTester, GetInfo1);
	ADD_TEST4(BMessageRunner, SuiteOfTests, GetInfoTester, GetInfo2);

	return SuiteOfTests;
}

