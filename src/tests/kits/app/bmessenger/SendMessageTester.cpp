//------------------------------------------------------------------------------
//	SendMessageTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <OS.h>
#include <Handler.h>
#include <Looper.h>
#include <Messenger.h>

// Project Includes ------------------------------------------------------------
#include <TestUtils.h>
#include <ThreadedTestCaller.h>
#include <cppunit/TestSuite.h>

// Local Includes --------------------------------------------------------------
#include "Helpers.h"
#include "SendMessageTester.h"
#include "SMInvoker.h"
#include "SMLooper.h"
#include "SMReplyTarget.h"
#include "SMTarget.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

// target flavors
enum target_kind {
	TARGET_UNINITIALIZED,
	TARGET_LOCAL_PREFERRED,
	TARGET_LOCAL_SPECIFIC,
	TARGET_REMOTE_PREFERRED,
	TARGET_REMOTE_SPECIFIC,
};

// tester class
class SMTester {
public:
	SMTester(target_kind targetKind)
		: fTargetKind(targetKind)
	{
	}

	~SMTester()
	{
	}

	void Run(SMInvoker &invoker, bigtime_t targetUnblock,
			 bigtime_t targetReply, status_t result, bool deliverySuccess,
			 bool replySuccess, bigtime_t duration)
	{
//printf("SMTester::Run(%lld, %lld, %lx, %d, %d, %lld)\n", targetUnblock,
//targetReply, result, deliverySuccess, replySuccess, duration);
		enum { JITTER = 10000 };
		enum { SETUP_LATENCY = 100000 };
		enum { DELIVERY_LATENCY = 10000 };
		enum { TARGET_HEAD_START = 1000 };
		if (targetUnblock == 0)
			targetUnblock = -TARGET_HEAD_START;
		// create the target
		SMTarget *target = NULL;
		switch (fTargetKind) {
			case TARGET_UNINITIALIZED:
				target = new SMTarget;
				break;
			case TARGET_LOCAL_PREFERRED:
				target = new LocalSMTarget(true);
				break;
			case TARGET_LOCAL_SPECIFIC:
				target = new LocalSMTarget(false);
				break;
			case TARGET_REMOTE_PREFERRED:
				target = new RemoteSMTarget(true);
				break;
			case TARGET_REMOTE_SPECIFIC:
				target = new RemoteSMTarget(false);
				break;
		}
		AutoDeleter<SMTarget> deleter(target);
		// create the reply target
		SMReplyTarget replyTarget;
		// init the target and send the message
		BHandler *replyHandler = replyTarget.Handler();
		BMessenger replyMessenger = replyTarget.Messenger();
		bigtime_t startTime = system_time() + SETUP_LATENCY;
		target->Init(startTime + targetUnblock, targetReply);
		BMessenger targetMessenger = target->Messenger();
		snooze_until(startTime, B_SYSTEM_TIMEBASE);
		status_t actualResult = invoker.Invoke(targetMessenger, replyHandler,
											   replyMessenger);
		bigtime_t actualDuration = system_time() - startTime;
//printf("duration: %lld vs %lld\n", actualDuration, duration);
		// We need to wait for the reply, if reply mode is asynchronous.
		snooze_until(startTime + targetUnblock + targetReply
					 + 2 * DELIVERY_LATENCY, B_SYSTEM_TIMEBASE);
		bool actualReplySuccess = invoker.ReplySuccess();
		if (!invoker.DirectReply())
			actualReplySuccess = replyTarget.ReplySuccess();
		// check the results
if (actualResult != result)
printf("result: %lx vs %lx\n", actualResult, result);
		CHK(actualResult == result);
		CHK(target->DeliverySuccess() == deliverySuccess);
		CHK(actualReplySuccess == replySuccess);
		CHK(actualDuration > duration - JITTER
			&& actualDuration < duration + JITTER);
	}

private:
	target_kind	fTargetKind;
};


//------------------------------------------------------------------------------

// constructor
SendMessageTester::SendMessageTester()
	: BThreadedTestCase(),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// constructor
SendMessageTester::SendMessageTester(std::string name)
	: BThreadedTestCase(name),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// destructor
SendMessageTester::~SendMessageTester()
{
	if (fLooper) {
		fLooper->Lock();
		if (fHandler) {
			fLooper->RemoveHandler(fHandler);
			delete fHandler;
		}
		fLooper->Quit();
	}
}

// TestUninitialized
void
SendMessageTester::TestUninitialized()
{
	SMTester tester(TARGET_UNINITIALIZED);
	// status_t SendMessage(uint32 command, BHandler *replyTo) const
	NextSubTest();
	{
		SMInvoker1 invoker1(false);
		SMInvoker1 invoker2(true);
		tester.Run(invoker1, 0, 0, B_BAD_PORT_ID, false, false, 0);
		tester.Run(invoker2, 0, 0, B_BAD_PORT_ID, false, false, 0);
	}
	// status_t SendMessage(BMessage *message, BHandler *replyTo,
	//						bigtime_t timeout) const
	NextSubTest();
	{
		SMInvoker2 invoker1(true, false, B_INFINITE_TIMEOUT);
		SMInvoker2 invoker2(true, true, B_INFINITE_TIMEOUT);
		tester.Run(invoker1, 0, 0, B_BAD_PORT_ID, false, false, 0);
		tester.Run(invoker2, 0, 0, B_BAD_PORT_ID, false, false, 0);
	}
	// status_t SendMessage(BMessage *message, BMessenger replyTo,
	//						bigtime_t timeout) const
	NextSubTest();
	{
		SMInvoker3 invoker1(true, false, B_INFINITE_TIMEOUT);
		SMInvoker3 invoker2(true, true, B_INFINITE_TIMEOUT);
		tester.Run(invoker1, 0, 0, B_BAD_PORT_ID, false, false, 0);
		tester.Run(invoker2, 0, 0, B_BAD_PORT_ID, false, false, 0);
	}
	// status_t SendMessage(uint32 command, BMessage *reply) const
	NextSubTest();
	{
		SMInvoker4 invoker1(false);
		SMInvoker4 invoker2(true);
// We check the parameters first.
#ifdef TEST_OBOS
		tester.Run(invoker1, 0, 0, B_BAD_VALUE, false, false, 0);
#else
		tester.Run(invoker1, 0, 0, B_BAD_PORT_ID, false, false, 0);
#endif
		tester.Run(invoker2, 0, 0, B_BAD_PORT_ID, false, false, 0);
	}
	// status_t SendMessage(BMessage *message, BMessage *reply,
	//						bigtime_t deliveryTimeout,
	//						bigtime_t replyTimeout) const
	NextSubTest();
	{
		SMInvoker5 invoker1(true, false, B_INFINITE_TIMEOUT,
							B_INFINITE_TIMEOUT);
		SMInvoker5 invoker2(true, true, B_INFINITE_TIMEOUT,
							B_INFINITE_TIMEOUT);
#ifdef TEST_OBOS
		tester.Run(invoker1, 0, 0, B_BAD_VALUE, false, false, 0);
#else
		tester.Run(invoker1, 0, 0, B_BAD_PORT_ID, false, false, 0);
#endif
		tester.Run(invoker2, 0, 0, B_BAD_PORT_ID, false, false, 0);
	}
}

// TestInitialized
void
SendMessageTester::TestInitialized(SMTester &tester)
{
	// status_t SendMessage(uint32 command, BHandler *replyTo) const
	NextSubTest();
	{
		SMInvoker1 invoker1(false);
		SMInvoker1 invoker2(true);
		tester.Run(invoker1, 0, 0, B_OK, true, false, 0);
		tester.Run(invoker2, 0, 0, B_OK, true, true, 0);
	}
	// status_t SendMessage(BMessage *message, BHandler *replyTo,
	//						bigtime_t timeout) const
	NextSubTest();
	{
// R5 crashes when passing a NULL message.
#ifndef TEST_R5
		SMInvoker2 invoker1(false, false, B_INFINITE_TIMEOUT);
		tester.Run(invoker1, 0, 0, B_BAD_VALUE, false, false, 0);
#endif
	}
	NextSubTest();
	{
		SMInvoker2 invoker1(true, false, B_INFINITE_TIMEOUT);
		SMInvoker2 invoker2(true, true, B_INFINITE_TIMEOUT);
		tester.Run(invoker1, 0, 0, B_OK, true, false, 0);
		tester.Run(invoker2, 0, 0, B_OK, true, true, 0);
	}
	NextSubTest();
	{
		SMInvoker2 invoker1(true, false, 0);
		SMInvoker2 invoker2(true, true, 0);
		tester.Run(invoker1, 0, 0, B_OK, true, false, 0);
		tester.Run(invoker2, 0, 0, B_OK, true, true, 0);
		tester.Run(invoker1, 20000, 0, B_WOULD_BLOCK, false, false, 0);
		tester.Run(invoker2, 20000, 0, B_WOULD_BLOCK, false, false, 0);
	}
	NextSubTest();
	{
		SMInvoker2 invoker1(true, false, 20000);
		SMInvoker2 invoker2(true, true, 20000);
		tester.Run(invoker1, 10000, 0, B_OK, true, false, 10000);
		tester.Run(invoker2, 10000, 0, B_OK, true, true, 10000);
		tester.Run(invoker1, 40000, 0, B_TIMED_OUT, false, false, 20000);
		tester.Run(invoker2, 40000, 0, B_TIMED_OUT, false, false, 20000);
	}
	// status_t SendMessage(BMessage *message, BMessenger replyTo,
	//						bigtime_t timeout) const
	NextSubTest();
	{
// R5 crashes when passing a NULL message.
#ifndef TEST_R5
		SMInvoker3 invoker1(false, false, B_INFINITE_TIMEOUT);
		tester.Run(invoker1, 0, 0, B_BAD_VALUE, false, false, 0);
#endif
	}
	NextSubTest();
	{
		SMInvoker3 invoker1(true, false, B_INFINITE_TIMEOUT);
		SMInvoker3 invoker2(true, true, B_INFINITE_TIMEOUT);
		tester.Run(invoker1, 0, 0, B_OK, true, false, 0);
		tester.Run(invoker2, 0, 0, B_OK, true, true, 0);
	}
	NextSubTest();
	{
		SMInvoker3 invoker1(true, false, 0);
		SMInvoker3 invoker2(true, true, 0);
		tester.Run(invoker1, 0, 0, B_OK, true, false, 0);
		tester.Run(invoker2, 0, 0, B_OK, true, true, 0);
		tester.Run(invoker1, 20000, 0, B_WOULD_BLOCK, false, false, 0);
		tester.Run(invoker2, 20000, 0, B_WOULD_BLOCK, false, false, 0);
	}
	NextSubTest();
	{
		SMInvoker3 invoker1(true, false, 20000);
		SMInvoker3 invoker2(true, true, 20000);
		tester.Run(invoker1, 10000, 0, B_OK, true, false, 10000);
		tester.Run(invoker2, 10000, 0, B_OK, true, true, 10000);
		tester.Run(invoker1, 40000, 0, B_TIMED_OUT, false, false, 20000);
		tester.Run(invoker2, 40000, 0, B_TIMED_OUT, false, false, 20000);
	}
	// status_t SendMessage(uint32 command, BMessage *reply) const
	NextSubTest();
	{
// R5 crashes when passing a NULL reply message
#ifndef TEST_R5
		SMInvoker4 invoker1(false);
#endif
		SMInvoker4 invoker2(true);
#ifndef TEST_R5
		tester.Run(invoker1, 20000, 20000, B_BAD_VALUE, false, false, 0);
#endif
		tester.Run(invoker2, 20000, 20000, B_OK, true, true, 40000);
	}
	// status_t SendMessage(BMessage *message, BMessage *reply,
	//						bigtime_t deliveryTimeout,
	//						bigtime_t replyTimeout) const
	NextSubTest();
	{
// R5 crashes when passing a NULL message.
#ifndef TEST_R5
		SMInvoker5 invoker1(false, true, B_INFINITE_TIMEOUT,
							B_INFINITE_TIMEOUT);
		tester.Run(invoker1, 0, 0, B_BAD_VALUE, false, false, 0);
#endif
	}
	NextSubTest();
	{
		SMInvoker5 invoker1(true, true, B_INFINITE_TIMEOUT,
							B_INFINITE_TIMEOUT);
		SMInvoker5 invoker2(true, true, B_INFINITE_TIMEOUT, 0);
		SMInvoker5 invoker3(true, true, B_INFINITE_TIMEOUT, 20000);
		tester.Run(invoker1, 0, 0, B_OK, true, true, 0);
		tester.Run(invoker1, 10000, 0, B_OK, true, true, 10000);
		tester.Run(invoker1, 0, 10000, B_OK, true, true, 10000);
		// These two are race-conditional: The sending task must be pre-empted
		// before reading from the reply port and the target must reply before
		// the sending task gets another time slice.
//		tester.Run(invoker2, 0, 0, B_OK, true, true, 0);
//		tester.Run(invoker2, 10000, 0, B_OK, true, true, 10000);
		tester.Run(invoker2, 0, 10000, B_WOULD_BLOCK, true, false, 0);
		tester.Run(invoker3, 0, 10000, B_OK, true, true, 10000);
		tester.Run(invoker3, 20000, 10000, B_OK, true, true, 30000);
		tester.Run(invoker3, 0, 30000, B_TIMED_OUT, true, false, 20000);
	}
	NextSubTest();
	{
		SMInvoker5 invoker1(true, true, 0, B_INFINITE_TIMEOUT);
		SMInvoker5 invoker2(true, true, 0, 0);
		SMInvoker5 invoker3(true, true, 0, 20000);
		tester.Run(invoker1, 0, 0, B_OK, true, true, 0);
		tester.Run(invoker1, 10000, 0, B_WOULD_BLOCK, false, false, 0);
		tester.Run(invoker1, 0, 10000, B_OK, true, true, 10000);
		// This one is race-conditional: The sending task must be pre-empted
		// before reading from the reply port and the target must reply before
		// the sending task gets another time slice.
//		tester.Run(invoker2, 0, 0, B_OK, true, true, 0);
		tester.Run(invoker2, 10000, 0, B_WOULD_BLOCK, false, false, 0);
		tester.Run(invoker2, 0, 10000, B_WOULD_BLOCK, true, false, 0);
		tester.Run(invoker3, 0, 10000, B_OK, true, true, 10000);
		tester.Run(invoker3, 10000, 10000, B_WOULD_BLOCK, false, false, 0);
		tester.Run(invoker3, 0, 30000, B_TIMED_OUT, true, false, 20000);
	}
	NextSubTest();
	{
		SMInvoker5 invoker1(true, true, 20000, B_INFINITE_TIMEOUT);
		SMInvoker5 invoker2(true, true, 20000, 0);
		SMInvoker5 invoker3(true, true, 20000, 20000);
		tester.Run(invoker1, 0, 0, B_OK, true, true, 0);
		tester.Run(invoker1, 10000, 0, B_OK, true, true, 10000);
		tester.Run(invoker1, 30000, 0, B_TIMED_OUT, false, false, 20000);
		tester.Run(invoker1, 10000, 20000, B_OK, true, true, 30000);
		// These two are race-conditional: The sending task must be pre-empted
		// before reading from the reply port and the target must reply before
		// the sending task gets another time slice.
//		tester.Run(invoker2, 0, 0, B_OK, true, true, 0);
//		tester.Run(invoker2, 10000, 0, B_OK, true, true, 0);
		tester.Run(invoker2, 30000, 0, B_TIMED_OUT, false, false, 20000);
		tester.Run(invoker2, 0, 10000, B_WOULD_BLOCK, true, false, 0);
		tester.Run(invoker3, 10000, 10000, B_OK, true, true, 20000);
		tester.Run(invoker3, 30000, 10000, B_TIMED_OUT, false, false, 20000);
		tester.Run(invoker3, 10000, 30000, B_TIMED_OUT, true, false, 30000);
	}
}

/*
	The different SendMessage() flavors.
 */
void SendMessageTester::SendMessageTest1()
{
	TestUninitialized();
	target_kind targetKinds[] = {
		TARGET_LOCAL_PREFERRED,
		TARGET_LOCAL_SPECIFIC,
		TARGET_REMOTE_PREFERRED,
		TARGET_REMOTE_SPECIFIC
	};
	int32 targetKindCount = sizeof(targetKinds) / sizeof(target_kind);
	for (int32 i = 0; i < targetKindCount; i++) {
		SMTester tester(targetKinds[i]);
		TestInitialized(tester);
	}
}


Test* SendMessageTester::Suite()
{
	typedef BThreadedTestCaller<SendMessageTester> TC;

	TestSuite* testSuite = new TestSuite;

	ADD_TEST4(BMessenger, testSuite, SendMessageTester, SendMessageTest1);

	return testSuite;
}


