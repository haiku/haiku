//------------------------------------------------------------------------------
//	TargetTester.cpp
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

#define CHK	CPPUNIT_ASSERT

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "Helpers.h"
#include "SMTarget.h"
#include "TargetTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

/*
	bool IsTargetLocal() const
	@case 1			this is uninitialized
	@results		should return false.
 */
void TargetTester::IsTargetLocalTest1()
{
	BMessenger messenger;
	CHK(messenger.IsTargetLocal() == false);
}

/*
	bool IsTargetLocal() const
	@case 2			this is initialized to local target with preferred handler
	@results		should return true.
 */
void TargetTester::IsTargetLocalTest2()
{
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	BMessenger messenger(NULL, looper, &result);
	CHK(messenger.IsTargetLocal() == true);
}

/*
	bool IsTargetLocal() const
	@case 3			this is initialized to local target with specific handler
	@results		should return true.
 */
void TargetTester::IsTargetLocalTest3()
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
	CHK(messenger.IsTargetLocal() == true);
}

/*
	bool IsTargetLocal() const
	@case 4			this is initialized to remote target with preferred handler
	@results		should return false.
 */
void TargetTester::IsTargetLocalTest4()
{
	RemoteSMTarget target(true);
	BMessenger messenger(target.Messenger());
	CHK(messenger.IsTargetLocal() == false);
}

/*
	bool IsTargetLocal() const
	@case 5			this is initialized to remote target with specific handler
	@results		should return false.
 */
void TargetTester::IsTargetLocalTest5()
{
	RemoteSMTarget target(false);
	BMessenger messenger(target.Messenger());
	CHK(messenger.IsTargetLocal() == false);
}

/*
	BHandler *Target(BLooper **looper) const
	@case 1			this is uninitialized, looper is NULL
	@results		should return NULL.
 */
void TargetTester::TargetTest1()
{
	BMessenger messenger;
	CHK(messenger.Target(NULL) == NULL);
}

/*
	BHandler *Target(BLooper **looper) const
	@case 2			this is initialized to local target with preferred handler,
					looper is NULL
	@results		should return NULL.
 */
void TargetTester::TargetTest2()
{
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	BMessenger messenger(NULL, looper, &result);
	CHK(messenger.Target(NULL) == NULL);
}

/*
	BHandler *Target(BLooper **looper) const
	@case 3			this is initialized to local target with specific handler,
					looper is NULL
	@results		should return correct handler.
 */
void TargetTester::TargetTest3()
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
	CHK(messenger.Target(NULL) == handler);
}

/*
	BHandler *Target(BLooper **looper) const
	@case 4			this is initialized to remote target with preferred
					handler, looper is NULL
	@results		should return NULL.
 */
void TargetTester::TargetTest4()
{
	RemoteSMTarget target(true);
	BMessenger messenger(target.Messenger());
	CHK(messenger.Target(NULL) == NULL);
}

/*
	BHandler *Target(BLooper **looper) const
	@case 5			this is initialized to remote target with specific handler,
					looper is NULL
	@results		should return NULL.
 */
void TargetTester::TargetTest5()
{
	RemoteSMTarget target(false);
	BMessenger messenger(target.Messenger());
	CHK(messenger.Target(NULL) == NULL);
}


Test* TargetTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST4(BMessenger, SuiteOfTests, TargetTester, IsTargetLocalTest1);
	ADD_TEST4(BMessenger, SuiteOfTests, TargetTester, IsTargetLocalTest2);
	ADD_TEST4(BMessenger, SuiteOfTests, TargetTester, IsTargetLocalTest3);
	ADD_TEST4(BMessenger, SuiteOfTests, TargetTester, IsTargetLocalTest4);
	ADD_TEST4(BMessenger, SuiteOfTests, TargetTester, IsTargetLocalTest5);

	ADD_TEST4(BMessenger, SuiteOfTests, TargetTester, TargetTest1);
	ADD_TEST4(BMessenger, SuiteOfTests, TargetTester, TargetTest2);
	ADD_TEST4(BMessenger, SuiteOfTests, TargetTester, TargetTest3);
	ADD_TEST4(BMessenger, SuiteOfTests, TargetTester, TargetTest4);
	ADD_TEST4(BMessenger, SuiteOfTests, TargetTester, TargetTest5);

	return SuiteOfTests;
}


