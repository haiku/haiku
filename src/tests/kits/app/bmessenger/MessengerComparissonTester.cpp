//------------------------------------------------------------------------------
//	MessengerComparissonTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <be/app/Message.h>
#include <be/kernel/OS.h>

#include <Handler.h>
#include <Looper.h>
#include <Messenger.h>

// Project Includes ------------------------------------------------------------
#include <TestUtils.h>
#include <ThreadedTestCaller.h>
#include <cppunit/TestSuite.h>

// Local Includes --------------------------------------------------------------
#include "Helpers.h"
#include "MessengerComparissonTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// constructor
MessengerComparissonTester::MessengerComparissonTester()
	: BThreadedTestCase(),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// constructor
MessengerComparissonTester::MessengerComparissonTester(std::string name)
	: BThreadedTestCase(name),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// destructor
MessengerComparissonTester::~MessengerComparissonTester()
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
	BMessenger &operator=(const BMessenger &from)
	@case 1			from is uninitialized
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return -1.
 */
void MessengerComparissonTester::ComparissonTest1()
{
	BMessenger a;
	BMessenger b;
	CHK(a == b);
	CHK(b == a);
	CHK(!(a != b));
	CHK(!(b != a));
}

/*
	BMessenger &operator=(const BMessenger &from)
	@case 2			from is properly initialized to a local target
					(preferred handler)
	@results		IsValid() and IsTargetLocal() should return true
					Target() should return the same values as for from.
					Team() should return this team.
 */
void MessengerComparissonTester::ComparissonTest2()
{
	// create looper
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	// create messenger
	BMessenger a(NULL, looper);
	BMessenger b;
	CHK(a != b);
	CHK(b != a);
	CHK(!(a == b));
	CHK(!(b == a));
}

/*
	BMessenger &operator=(const BMessenger &from)
	@case 3			from is properly initialized to a local target
					(specific handler)
	@results		IsValid() and IsTargetLocal() should return true
					Target() should return the same values as for from.
					Team() should return this team.
 */
void MessengerComparissonTester::ComparissonTest3()
{
	// messenger1: looper and handler
	BLooper *looper1 = new BLooper;
	looper1->Run();
	LooperQuitter quitter1(looper1);
	BHandler *handler1 = new BHandler;
	HandlerDeleter deleter1(handler1);
	CHK(looper1->Lock());
	looper1->AddHandler(handler1);
	looper1->Unlock();
	BMessenger messenger1(handler1, looper1);
	BMessenger messenger1a(handler1, looper1);
	// messenger2: looper (1)
	BMessenger messenger2(NULL, looper1);
	BMessenger messenger2a(NULL, looper1);
	// messenger3: looper and handler
	BLooper *looper2 = new BLooper;
	looper2->Run();
	LooperQuitter quitter2(looper2);
	BHandler *handler2 = new BHandler;
	HandlerDeleter deleter2(handler2);
	CHK(looper2->Lock());
	looper2->AddHandler(handler2);
	looper2->Unlock();
	BMessenger messenger3(handler2, looper2);
	BMessenger messenger3a(handler2, looper2);
	// messenger4: looper (2)
	BMessenger messenger4(NULL, looper2);
	BMessenger messenger4a(NULL, looper2);
	// TODO: remote targets
	// ...

	// targets -- test data
	struct target {
		BMessenger	&messenger;
		int32		id;			// identifies the target
	} targets[] = {
		{ messenger1,	1 },
		{ messenger1a,	1 },
		{ messenger2,	2 },
		{ messenger2,	2 },
		{ messenger3,	3 },
		{ messenger3a,	3 },
		{ messenger4,	4 },
		{ messenger4a,	4 },
	};
	int32 targetCount = sizeof(targets) / sizeof(target);

	// test
	for (int32 i = 0; i < targetCount; i++) {
		NextSubTest();
		const target &target1 = targets[i];
		const BMessenger &a = target1.messenger;
		for (int32 k = 0; k < targetCount; k++) {
			const target &target2 = targets[k];
			const BMessenger &b = target2.messenger;
			bool areEqual = (target1.id == target2.id);
			CHK((a == b) == areEqual);
			CHK((b == a) == areEqual);
			CHK((a != b) == !areEqual);
			CHK((b != a) == !areEqual);
		}
	}
}


Test* MessengerComparissonTester::Suite()
{
	typedef BThreadedTestCaller<MessengerComparissonTester> TC;

	TestSuite* testSuite = new TestSuite;

	ADD_TEST(testSuite, MessengerComparissonTester, ComparissonTest1);
	ADD_TEST(testSuite, MessengerComparissonTester, ComparissonTest2);
	ADD_TEST(testSuite, MessengerComparissonTester, ComparissonTest3);

	return testSuite;
}

