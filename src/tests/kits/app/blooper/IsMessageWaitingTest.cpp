//------------------------------------------------------------------------------
//	IsMessageWaitingTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#if defined(SYSTEM_TEST)
#include <be/app/Looper.h>
#include <be/app/MessageQueue.h>
#else
#include "../../../../lib/application/headers/Looper.h"
#include "../../../../lib/application/headers/MessageQueue.h"
#endif

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "IsMessageWaitingTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
//case 1: looper is unlocked and queue is empty
void TIsMessageWaitingTest::IsMessageWaiting1()
{
	BLooper Looper;
	assert(!Looper.IsMessageWaiting());
}
//------------------------------------------------------------------------------
//case 2: looper is unlocked and queue is filled
void TIsMessageWaitingTest::IsMessageWaiting2()
{
	BLooper Looper;
	Looper.PostMessage('1234');
	assert(Looper.IsMessageWaiting());
}
//------------------------------------------------------------------------------
//case 3: looper is locked and queue is empty
void TIsMessageWaitingTest::IsMessageWaiting3()
{
	BLooper Looper;
	Looper.Lock();
	assert(!Looper.IsMessageWaiting());
}
//------------------------------------------------------------------------------
//case 4: looper is locked and queue is filled
void TIsMessageWaitingTest::IsMessageWaiting4()
{
	BLooper Looper;
	Looper.Lock();
	Looper.PostMessage('1234');
	assert(Looper.IsMessageWaiting());
}
//------------------------------------------------------------------------------
//case 5: looper is locked, message is posted, queue is empty
void TIsMessageWaitingTest::IsMessageWaiting5()
{
	BLooper* Looper = new BLooper(__PRETTY_FUNCTION__);
	Looper->Run();

	// Prevent a port read
	Looper->Lock();
	Looper->PostMessage('1234');
	assert(Looper->MessageQueue()->IsEmpty());
	assert(Looper->IsMessageWaiting());
}
//------------------------------------------------------------------------------
Test* TIsMessageWaitingTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::IsMessageWaiting()");

	ADD_TEST(suite, TIsMessageWaitingTest, IsMessageWaiting1);
	ADD_TEST(suite, TIsMessageWaitingTest, IsMessageWaiting2);
	ADD_TEST(suite, TIsMessageWaitingTest, IsMessageWaiting3);
	ADD_TEST(suite, TIsMessageWaitingTest, IsMessageWaiting4);
	ADD_TEST(suite, TIsMessageWaitingTest, IsMessageWaiting5);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

