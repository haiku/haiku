//------------------------------------------------------------------------------
//	MessengerAssignmentTester.cpp
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
#include "MessengerAssignmentTester.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

// constructor
MessengerAssignmentTester::MessengerAssignmentTester()
	: BThreadedTestCase(),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// constructor
MessengerAssignmentTester::MessengerAssignmentTester(std::string name)
	: BThreadedTestCase(name),
	  fHandler(NULL),
	  fLooper(NULL)
{
}

// destructor
MessengerAssignmentTester::~MessengerAssignmentTester()
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
void MessengerAssignmentTester::AssignmentTest1()
{
	BMessenger from;
	BMessenger messenger;
	CHK(&(messenger = from) == &messenger);
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *looper;
	CHK(messenger.Target(&looper) == NULL);
	CHK(looper == NULL);
	CHK(messenger.Team() == -1);
}

/*
	BMessenger &operator=(const BMessenger &from)
	@case 2			from is properly initialized to a local target
					(preferred handler)
	@results		IsValid() and IsTargetLocal() should return true
					Target() should return the same values as for from.
					Team() should return this team.
 */
void MessengerAssignmentTester::AssignmentTest2()
{
	// create looper
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	// create the from messenger
	BMessenger from(NULL, looper, &result);
	CHK(from.IsValid() == true);
	CHK(from.IsTargetLocal() == true);
	BLooper *resultLooper;
	CHK(from.Target(&resultLooper) == NULL);
	CHK(resultLooper == looper);
	CHK(from.Team() == get_this_team());
	CHK(result == B_OK);
	// create the messenger and do the checks
	BMessenger messenger;
	CHK(&(messenger = from) == &messenger);
	CHK(messenger.IsValid() == true);
	CHK(messenger.IsTargetLocal() == true);
	resultLooper = NULL;
	CHK(messenger.Target(&resultLooper) == NULL);
	CHK(resultLooper == looper);
	CHK(messenger.Team() == get_this_team());
}

/*
	BMessenger &operator=(const BMessenger &from)
	@case 3			from is properly initialized to a local target
					(specific handler)
	@results		IsValid() and IsTargetLocal() should return true
					Target() should return the same values as for from.
					Team() should return this team.
 */
void MessengerAssignmentTester::AssignmentTest3()
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
	BMessenger messenger;
	CHK(&(messenger = from) == &messenger);
	CHK(messenger.IsValid() == true);
	CHK(messenger.IsTargetLocal() == true);
	resultLooper = NULL;
	CHK(messenger.Target(&resultLooper) == handler);
	CHK(resultLooper == looper);
	CHK(messenger.Team() == get_this_team());
}


Test* MessengerAssignmentTester::Suite()
{
	typedef BThreadedTestCaller<MessengerAssignmentTester> TC;

	TestSuite* testSuite = new TestSuite;

	ADD_TEST4(BMessenger, testSuite, MessengerAssignmentTester, AssignmentTest1);
	ADD_TEST4(BMessenger, testSuite, MessengerAssignmentTester, AssignmentTest2);
	ADD_TEST4(BMessenger, testSuite, MessengerAssignmentTester, AssignmentTest3);

	return testSuite;
}


