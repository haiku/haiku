//------------------------------------------------------------------------------
//	LockTargetTester.cpp
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
#include "LockTargetTester.h"
#include "SMTarget.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// constructor
LockTargetTester::LockTargetTester()
	: BThreadedTestCase(),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// constructor
LockTargetTester::LockTargetTester(std::string name)
	: BThreadedTestCase(name),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// destructor
LockTargetTester::~LockTargetTester()
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
	bool LockTarget() const
	@case 1			this is uninitialized
	@results		should return false.
 */
void LockTargetTester::LockTargetTest1()
{
	BMessenger messenger;
	CHK(messenger.LockTarget() == false);
}

/*
	bool LockTarget() const
	@case 2			this is initialized to local target with preferred handler,
					looper is not locked
	@results		should lock the looper and return true.
 */
void LockTargetTester::LockTargetTest2()
{
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	BMessenger messenger(NULL, looper, &result);
	CHK(messenger.LockTarget() == true);
	CHK(looper->IsLocked() == true);
	looper->Unlock();
	CHK(looper->IsLocked() == false);
}

/*
	bool LockTarget() const
	@case 3			this is initialized to local target with specific handler,
					looper is not locked
	@results		should lock the looper and return true.
 */
void LockTargetTester::LockTargetTest3()
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
	CHK(messenger.LockTarget() == true);
	CHK(looper->IsLocked() == true);
	looper->Unlock();
	CHK(looper->IsLocked() == false);
}

/*
	bool LockTarget() const
	@case 4			this is initialized to local target with preferred handler,
					looper is locked by another thread
	@results		should block until the looper is unlocked, lock it and
					return true.
	@thread A		- locks the looper
					- waits 100ms
					- unlocks the looper
 */
void LockTargetTester::LockTargetTest4A()
{
	CHK(fLooper->Lock() == true);
	snooze(100000);
	fLooper->Unlock();
}

/*
	bool LockTarget() const
	@case 4			this is initialized to local target with preferred handler,
					looper is locked by another thread
	@results		should block until the looper is unlocked, lock it and
					return true.
	@thread B		- waits 50ms (until thread A has acquired the looper lock)
					- tries to lock the looper via messenger and blocks
					- acquires the lock successfully after 50ms
					- unlocks the looper
 */
void LockTargetTester::LockTargetTest4B()
{
	enum { JITTER = 10000 };	// Maybe critical on slow machines.
	snooze(50000);
	BMessenger messenger(NULL, fLooper);
	bigtime_t time = system_time();
	CHK(messenger.LockTarget() == true);
	time = system_time() - time - 50000;
	CHK(fLooper->IsLocked() == true);
	fLooper->Unlock();
	CHK(fLooper->IsLocked() == false);
	CHK(time > -JITTER && time < JITTER);
}

/*
	bool LockTarget() const
	@case 5			this is initialized to local target with specific handler,
					looper is locked by another thread
	@results		should block until the looper is unlocked, lock it and
					return true.
	@thread A		- locks the looper
					- waits 100ms
					- unlocks the looper
 */
void LockTargetTester::LockTargetTest5A()
{
	CHK(fLooper->Lock() == true);
	snooze(100000);
	fLooper->Unlock();
}

/*
	bool LockTarget() const
	@case 5			this is initialized to local target with specific handler,
					looper is locked by another thread
	@results		should block until the looper is unlocked, lock it and
					return true.
	@thread B		- waits 50ms (until thread A has acquired the looper lock)
					- tries to lock the looper via messenger and blocks
					- acquires the lock successfully after 50ms
					- unlocks the looper
 */
void LockTargetTester::LockTargetTest5B()
{
	enum { JITTER = 10000 };	// Maybe critical on slow machines.
	snooze(50000);
	BMessenger messenger(fHandler, NULL);
	bigtime_t time = system_time();
	CHK(messenger.LockTarget() == true);
	time = system_time() - time - 50000;
	CHK(fLooper->IsLocked() == true);
	fLooper->Unlock();
	CHK(fLooper->IsLocked() == false);
	CHK(time > -JITTER && time < JITTER);
}

/*
	bool LockTarget() const
	@case 6			this is initialized to remote target with preferred
					handler, looper is not locked
	@results		should not lock the looper and return false.
 */
void LockTargetTester::LockTargetTest6()
{
	RemoteSMTarget target(true);
	BMessenger messenger(target.Messenger());
	CHK(messenger.LockTarget() == false);
}

/*
	bool LockTarget() const
	@case 7			this is initialized to remote target with specific handler,
					looper is not locked
	@results		should not lock the looper and return false.
 */
void LockTargetTester::LockTargetTest7()
{
	RemoteSMTarget target(false);
	BMessenger messenger(target.Messenger());
	CHK(messenger.LockTarget() == false);
}


Test* LockTargetTester::Suite()
{
	typedef BThreadedTestCaller<LockTargetTester> TC;

	TestSuite* testSuite = new TestSuite;

	ADD_TEST4(BMessenger, testSuite, LockTargetTester, LockTargetTest1);
	ADD_TEST4(BMessenger, testSuite, LockTargetTester, LockTargetTest2);
	ADD_TEST4(BMessenger, testSuite, LockTargetTester, LockTargetTest3);
	// test4
	LockTargetTester *test4
		= new LockTargetTester("LockTargetTest4");
	test4->fLooper = new BLooper;
	test4->fLooper->Run();
	// test4 test caller
	TC *caller4 = new TC("BMessenger::LockTargetTest4", test4);
	caller4->addThread("A", &LockTargetTester::LockTargetTest4A);
	caller4->addThread("B", &LockTargetTester::LockTargetTest4B);
	testSuite->addTest(caller4);	
	// test5
	LockTargetTester *test5
		= new LockTargetTester("LockTargetTest5");
	// create looper and handler
	test5->fLooper = new BLooper;
	test5->fLooper->Run();
	test5->fHandler = new BHandler;
	if (test5->fLooper->Lock()) {
		test5->fLooper->AddHandler(test5->fHandler);
		test5->fLooper->Unlock();
	} else
		printf("ERROR: Can't init LockTargetTester test5!\n");
	// test5 test caller
	TC *caller5 = new TC("BMessenger::LockTargetTest5", test5);
	caller5->addThread("A", &LockTargetTester::LockTargetTest5A);
	caller5->addThread("B", &LockTargetTester::LockTargetTest5B);
	testSuite->addTest(caller5);	
	// tests 6-7
	ADD_TEST4(BMessenger, testSuite, LockTargetTester, LockTargetTest6);
	ADD_TEST4(BMessenger, testSuite, LockTargetTester, LockTargetTest7);

	return testSuite;
}


