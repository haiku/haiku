//------------------------------------------------------------------------------
//	LockTargetWithTimeoutTester.cpp
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
#include "LockTargetWithTimeoutTester.h"
#include "SMTarget.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// constructor
LockTargetWithTimeoutTester::LockTargetWithTimeoutTester()
	: BThreadedTestCase(),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// constructor
LockTargetWithTimeoutTester::LockTargetWithTimeoutTester(std::string name)
	: BThreadedTestCase(name),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// destructor
LockTargetWithTimeoutTester::~LockTargetWithTimeoutTester()
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

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 1			this is uninitialized
	@results		should return B_BAD_VALUE.
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest1()
{
	BMessenger messenger;
	CHK(messenger.LockTargetWithTimeout(0) == B_BAD_VALUE);
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 2			this is initialized to local target with preferred handler,
					looper is not locked
	@results		should lock the looper and return B_OK.
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest2()
{
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	BMessenger messenger(NULL, looper, &result);
	CHK(messenger.LockTargetWithTimeout(0) == B_OK);
	CHK(looper->IsLocked() == true);
	looper->Unlock();
	CHK(looper->IsLocked() == false);
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 3			this is initialized to local target with specific handler,
					looper is not locked
	@results		should lock the looper and return B_OK.
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest3()
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
	CHK(messenger.LockTargetWithTimeout(0) == B_OK);
	CHK(looper->IsLocked() == true);
	looper->Unlock();
	CHK(looper->IsLocked() == false);
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 4			this is initialized to local target with preferred handler,
					looper is locked by another thread, timeout is 100ms
	@results		should block until the looper is unlocked (after 50ms),
					lock it and return B_OK.
	@thread A		- locks the looper
					- waits 100ms
					- unlocks the looper
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest4A()
{
	CHK(fLooper->Lock() == true);
	snooze(100000);
	fLooper->Unlock();
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 4			this is initialized to local target with preferred handler,
					looper is locked by another thread, timeout is 100ms
	@results		should block until the looper is unlocked (after 50ms),
					lock it and return B_OK.
	@thread B		- waits 50ms (until thread A has acquired the looper lock)
					- tries to lock the looper via messenger and blocks
					- acquires the lock successfully after 50ms
					- unlocks the looper
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest4B()
{
	enum { JITTER = 10000 };	// Maybe critical on slow machines.
	snooze(50000);
	BMessenger messenger(NULL, fLooper);
	bigtime_t time = system_time();
	CHK(messenger.LockTargetWithTimeout(100000) == B_OK);
	time = system_time() - time - 50000;
	CHK(fLooper->IsLocked() == true);
	fLooper->Unlock();
	CHK(fLooper->IsLocked() == false);
	CHK(time > -JITTER && time < JITTER);
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 5			this is initialized to local target with preferred handler,
					looper is locked by another thread, timeout is 25ms
	@results		should block for 25ms, not until the looper is unlocked
					(after 50ms), should return B_TIMED_OUT.
	@thread A		- locks the looper
					- waits 100ms
					- unlocks the looper
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest5A()
{
	CHK(fLooper->Lock() == true);
	snooze(100000);
	fLooper->Unlock();
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 5			this is initialized to local target with preferred handler,
					looper is locked by another thread, timeout is 25ms
	@results		should block for 25ms, not until the looper is unlocked
					(after 50ms), should return B_TIMED_OUT.
	@thread B		- waits 50ms (until thread A has acquired the looper lock)
					- tries to lock the looper via messenger and blocks
					- times out after 25ms
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest5B()
{
	enum { JITTER = 10000 };	// Maybe critical on slow machines.
	snooze(50000);
	BMessenger messenger(NULL, fLooper);
	bigtime_t time = system_time();
	CHK(messenger.LockTargetWithTimeout(25000) == B_TIMED_OUT);
	time = system_time() - time - 25000;
	CHK(fLooper->IsLocked() == false);
	CHK(time > -JITTER && time < JITTER);
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 6			this is initialized to local target with specific handler,
					looper is locked by another thread, timeout is 100ms
	@results		should block until the looper is unlocked (after 50ms),
					lock it and return B_OK.
	@thread A		- locks the looper
					- waits 100ms
					- unlocks the looper
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest6A()
{
	CHK(fLooper->Lock() == true);
	snooze(100000);
	fLooper->Unlock();
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 6			this is initialized to local target with specific handler,
					looper is locked by another thread, timeout is 100ms
	@results		should block until the looper is unlocked (after 50ms),
					lock it and return B_OK.
	@thread B		- waits 50ms (until thread A has acquired the looper lock)
					- tries to lock the looper via messenger and blocks
					- acquires the lock successfully after 50ms
					- unlocks the looper
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest6B()
{
	enum { JITTER = 10000 };	// Maybe critical on slow machines.
	snooze(50000);
	BMessenger messenger(fHandler, NULL);
	bigtime_t time = system_time();
	CHK(messenger.LockTargetWithTimeout(100000) == B_OK);
	time = system_time() - time - 50000;
	CHK(fLooper->IsLocked() == true);
	fLooper->Unlock();
	CHK(fLooper->IsLocked() == false);
	CHK(time > -JITTER && time < JITTER);
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 7			this is initialized to local target with specific handler,
					looper is locked by another thread, timeout is 25ms
	@results		should block for 25ms, not until the looper is unlocked
					(after 50ms), should return B_TIMED_OUT.
	@thread A		- locks the looper
					- waits 100ms
					- unlocks the looper
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest7A()
{
	CHK(fLooper->Lock() == true);
	snooze(100000);
	fLooper->Unlock();
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 7			this is initialized to local target with specific handler,
					looper is locked by another thread, timeout is 25ms
	@results		should block for 25ms, not until the looper is unlocked
					(after 50ms), should return B_TIMED_OUT.
	@thread B		- waits 50ms (until thread A has acquired the looper lock)
					- tries to lock the looper via messenger and blocks
					- times out after 25ms
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest7B()
{
	enum { JITTER = 10000 };	// Maybe critical on slow machines.
	snooze(50000);
	BMessenger messenger(fHandler, NULL);
	bigtime_t time = system_time();
	CHK(messenger.LockTargetWithTimeout(25000) == B_TIMED_OUT);
	time = system_time() - time - 25000;
	CHK(fLooper->IsLocked() == false);
	CHK(time > -JITTER && time < JITTER);
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 8			this is initialized to remote target with preferred
					handler, looper is not locked
	@results		should not lock the looper and return B_BAD_VALUE.
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest8()
{
	RemoteSMTarget target(true);
	BMessenger messenger(target.Messenger());
	CHK(messenger.LockTargetWithTimeout(10000) == B_BAD_VALUE);
}

/*
	status_t LockTargetWithTimeout(bigtime_t timeout) const
	@case 9			this is initialized to remote target with specific handler,
					looper is not locked
	@results		should not lock the looper and return B_BAD_VALUE.
 */
void LockTargetWithTimeoutTester::LockTargetWithTimeoutTest9()
{
	RemoteSMTarget target(false);
	BMessenger messenger(target.Messenger());
	CHK(messenger.LockTargetWithTimeout(10000) == B_BAD_VALUE);
}


Test* LockTargetWithTimeoutTester::Suite()
{
	typedef BThreadedTestCaller<LockTargetWithTimeoutTester> TC;

	TestSuite* testSuite = new TestSuite;

	ADD_TEST4(BMessenger, testSuite, LockTargetWithTimeoutTester,
			 LockTargetWithTimeoutTest1);
	ADD_TEST4(BMessenger, testSuite, LockTargetWithTimeoutTester,
			 LockTargetWithTimeoutTest2);
	ADD_TEST4(BMessenger, testSuite, LockTargetWithTimeoutTester,
			 LockTargetWithTimeoutTest3);
	// test4
	LockTargetWithTimeoutTester *test4
		= new LockTargetWithTimeoutTester("LockTargetWithTimeoutTest4");
	test4->fLooper = new BLooper;
	test4->fLooper->Run();
	// test4 test caller
	TC *caller4 = new TC("BMessenger::LockTargetWithTimeoutTest4", test4);
	caller4->addThread("A",
		&LockTargetWithTimeoutTester::LockTargetWithTimeoutTest4A);
	caller4->addThread("B",
		&LockTargetWithTimeoutTester::LockTargetWithTimeoutTest4B);
	testSuite->addTest(caller4);	
	// test5
	LockTargetWithTimeoutTester *test5
		= new LockTargetWithTimeoutTester("LockTargetWithTimeoutTest5");
	test5->fLooper = new BLooper;
	test5->fLooper->Run();
	// test5 test caller
	TC *caller5 = new TC("BMessenger::LockTargetWithTimeoutTest5", test5);
	caller5->addThread("A",
		&LockTargetWithTimeoutTester::LockTargetWithTimeoutTest5A);
	caller5->addThread("B",
		&LockTargetWithTimeoutTester::LockTargetWithTimeoutTest5B);
	testSuite->addTest(caller5);	
	// test6
	LockTargetWithTimeoutTester *test6
		= new LockTargetWithTimeoutTester("LockTargetWithTimeoutTest6");
	// create looper and handler
	test6->fLooper = new BLooper;
	test6->fLooper->Run();
	test6->fHandler = new BHandler;
	if (test6->fLooper->Lock()) {
		test6->fLooper->AddHandler(test6->fHandler);
		test6->fLooper->Unlock();
	} else
		printf("ERROR: Can't init LockTargetWithTimeoutTester test6!\n");
	// test6 test caller
	TC *caller6 = new TC("BMessenger::LockTargetWithTimeoutTest6", test6);
	caller6->addThread("A",
		&LockTargetWithTimeoutTester::LockTargetWithTimeoutTest6A);
	caller6->addThread("B",
		&LockTargetWithTimeoutTester::LockTargetWithTimeoutTest6B);
	testSuite->addTest(caller6);	
	// test7
	LockTargetWithTimeoutTester *test7
		= new LockTargetWithTimeoutTester("LockTargetWithTimeoutTest7");
	// create looper and handler
	test7->fLooper = new BLooper;
	test7->fLooper->Run();
	test7->fHandler = new BHandler;
	if (test7->fLooper->Lock()) {
		test7->fLooper->AddHandler(test7->fHandler);
		test7->fLooper->Unlock();
	} else
		printf("ERROR: Can't init LockTargetWithTimeoutTester test7!\n");
	// test7 test caller
	TC *caller7 = new TC("BMessenger::LockTargetWithTimeoutTest7", test7);
	caller7->addThread("A",
		&LockTargetWithTimeoutTester::LockTargetWithTimeoutTest7A);
	caller7->addThread("B",
		&LockTargetWithTimeoutTester::LockTargetWithTimeoutTest7B);
	testSuite->addTest(caller7);	
	// tests 8-9
	ADD_TEST4(BMessenger, testSuite, LockTargetWithTimeoutTester,
			 LockTargetWithTimeoutTest8);
	ADD_TEST4(BMessenger, testSuite, LockTargetWithTimeoutTester,
			 LockTargetWithTimeoutTest9);

	return testSuite;
}


