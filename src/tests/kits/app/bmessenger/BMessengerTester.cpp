//------------------------------------------------------------------------------
//	BMessengerTester.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <be/app/Message.h>
#include <be/kernel/OS.h>

//#ifdef SYSTEM_TEST
//#include <be/app/Handler.h>
//#include <be/app/Looper.h>
//#include <be/app/Messenger.h>
//#else
#include <Handler.h>
#include <Looper.h>
#include <Messenger.h>
//#endif

#define CHK	CPPUNIT_ASSERT

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "BMessengerTester.h"
#include "Helpers.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------

/*
	BMessenger()
	@case 1
	@results		IsValid() should return false.
					IsTargetLocal() should return false.
					Target() should return NULL and NULL for looper.
					Team() should return -1.
 */
void TBMessengerTester::BMessenger1()
{
	BMessenger messenger;
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *looper;
	CHK(messenger.Target(&looper) == NULL);
	CHK(looper == NULL);
	CHK(messenger.Team() == -1);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 1			handler is NULL, looper is NULL, result is NULL
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return -1.
 */
void TBMessengerTester::BMessenger2()
{
	BMessenger messenger((const BHandler*)NULL, NULL, NULL);
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *looper;
	CHK(messenger.Target(&looper) == NULL);
	CHK(looper == NULL);
	CHK(messenger.Team() == -1);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 2			handler is NULL, looper is NULL, result is not NULL
	@results		IsValid() and IsTargetLocal() should return false.
					Target() should return NULL and NULL for looper.
					Team() should return -1.
					result is set to B_BAD_VALUE.
 */
void TBMessengerTester::BMessenger3()
{
	status_t result = B_OK;
	BMessenger messenger((const BHandler*)NULL, NULL, &result);
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *looper;
	CHK(messenger.Target(&looper) == NULL);
	CHK(looper == NULL);
	CHK(messenger.Team() == -1);
	CHK(result == B_BAD_VALUE);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 3			handler is NULL, looper is not NULL, result is not NULL
	@results		IsValid() and IsTargetLocal() should return true.
					Target() should return NULL and the correct value for
					looper.
					Team() should return this team.
					result is set to B_OK.
 */
void TBMessengerTester::BMessenger4()
{
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	BMessenger messenger(NULL, looper, &result);
	CHK(messenger.IsValid() == true);
	CHK(messenger.IsTargetLocal() == true);
	BLooper *resultLooper;
	CHK(messenger.Target(&resultLooper) == NULL);
	CHK(resultLooper == looper);
	CHK(messenger.Team() == get_this_team());
	CHK(result == B_OK);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 4			handler is not NULL, looper is NULL, result is not NULL,
					handler doesn't belong to a looper
	@results		IsValid() and IsTargetLocal() should return false.
					Target() should return NULL and NULL for looper.
					Team() should return -1.
					result is set to B_MISMATCHED_VALUES.
 */
void TBMessengerTester::BMessenger5()
{
	status_t result = B_OK;
	BHandler *handler = new BHandler;
	HandlerDeleter deleter(handler);
	BMessenger messenger(handler, NULL, &result);
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *resultLooper;
	CHK(messenger.Target(&resultLooper) == NULL);
	CHK(resultLooper == NULL);
	CHK(messenger.Team() == -1);
	CHK(result == B_MISMATCHED_VALUES);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 5			handler is not NULL, looper is NULL, result is not NULL
					handler does belong to a looper
	@results		IsValid() and IsTargetLocal() should return true.
					Target() should return the correct handler and the
					handler->Looper() for looper.
					Team() should return this team.
					result is set to B_OK.
 */
void TBMessengerTester::BMessenger6()
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
	CHK(messenger.IsValid() == true);
	CHK(messenger.IsTargetLocal() == true);
	BLooper *resultLooper;
	CHK(messenger.Target(&resultLooper) == handler);
	CHK(resultLooper == looper);
	CHK(messenger.Team() == get_this_team());
//printf("result: %lx\n", result);
	CHK(result == B_OK);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 6			handler is not NULL, looper is not NULL, result is not NULL
					handler does belong to the looper
	@results		IsValid() and IsTargetLocal() should return true.
					Target() should return the correct handler and the correct value
					for looper.
					Team() should return this team.
					result is set to B_OK.
 */
void TBMessengerTester::BMessenger7()
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
	BMessenger messenger(handler, looper, &result);
	CHK(messenger.IsValid() == true);
	CHK(messenger.IsTargetLocal() == true);
	BLooper *resultLooper;
	CHK(messenger.Target(&resultLooper) == handler);
	CHK(resultLooper == looper);
	CHK(messenger.Team() == get_this_team());
//printf("result: %lx\n", result);
	CHK(result == B_OK);
}

/*
	BMessenger(const BHandler *handler, const BLooper *looper,
			   status_t *result)
	@case 7			handler is not NULL, looper is not NULL, result is not NULL
					handler does belong to a different looper
	@results		IsValid() and IsTargetLocal() should return false.
					Target() should return NULL and NULL for looper.
					Team() should return -1.
					result is set to B_MISMATCHED_VALUES.
 */
void TBMessengerTester::BMessenger8()
{
	// create loopers and handler
	status_t result = B_OK;
	BLooper *looper = new BLooper;
	looper->Run();
	LooperQuitter quitter(looper);
	BLooper *looper2 = new BLooper;
	looper2->Run();
	LooperQuitter quitter2(looper2);
	BHandler *handler = new BHandler;
	HandlerDeleter deleter(handler);
	CHK(looper->Lock());
	looper->AddHandler(handler);
	looper->Unlock();
	// create the messenger and do the checks
	BMessenger messenger(handler, looper2, &result);
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *resultLooper;
	CHK(messenger.Target(&resultLooper) == NULL);
	CHK(resultLooper == NULL);
	CHK(messenger.Team() == -1);
//printf("result: %lx\n", result);
	CHK(result == B_MISMATCHED_VALUES);
}

/*
	BMessenger(const BMessenger &from)
	@case 1			from is uninitialized
	@results		IsValid() and IsTargetLocal() should return false
					Target() should return NULL and NULL for looper.
					Team() should return -1.
 */
void TBMessengerTester::BMessenger9()
{
	BMessenger from;
	BMessenger messenger(from);
	CHK(messenger.IsValid() == false);
	CHK(messenger.IsTargetLocal() == false);
	BLooper *looper;
	CHK(messenger.Target(&looper) == NULL);
	CHK(looper == NULL);
	CHK(messenger.Team() == -1);
}


/*
	BMessenger(const BMessenger &from)
	@case 2			from is properly initialized to a local target
	@results		IsValid() and IsTargetLocal() should return true
					Target() should return the same values as for from.
					Team() should return this team.
 */
void TBMessengerTester::BMessenger10()
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
	BMessenger messenger(from);
	CHK(messenger.IsValid() == true);
	CHK(messenger.IsTargetLocal() == true);
	resultLooper = NULL;
	CHK(messenger.Target(&resultLooper) == handler);
	CHK(resultLooper == looper);
	CHK(messenger.Team() == get_this_team());
}



Test* TBMessengerTester::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite;

	ADD_TEST(SuiteOfTests, TBMessengerTester, BMessenger1);
	ADD_TEST(SuiteOfTests, TBMessengerTester, BMessenger2);
	ADD_TEST(SuiteOfTests, TBMessengerTester, BMessenger3);
	ADD_TEST(SuiteOfTests, TBMessengerTester, BMessenger4);
	ADD_TEST(SuiteOfTests, TBMessengerTester, BMessenger5);
	ADD_TEST(SuiteOfTests, TBMessengerTester, BMessenger6);
	ADD_TEST(SuiteOfTests, TBMessengerTester, BMessenger7);
	ADD_TEST(SuiteOfTests, TBMessengerTester, BMessenger8);
	ADD_TEST(SuiteOfTests, TBMessengerTester, BMessenger9);
	ADD_TEST(SuiteOfTests, TBMessengerTester, BMessenger10);

	return SuiteOfTests;
}


