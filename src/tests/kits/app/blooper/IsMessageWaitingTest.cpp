//------------------------------------------------------------------------------
//	IsMessageWaitingTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <iostream>

// System Includes -------------------------------------------------------------
#include <Looper.h>
#include <MessageQueue.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "IsMessageWaitingTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
port_id _get_looper_port_(const BLooper* looper);

//------------------------------------------------------------------------------
//case 1: looper is unlocked and queue is empty
void TIsMessageWaitingTest::IsMessageWaiting1()
{
	BLooper Looper;
#ifndef TEST_R5
	assert(!Looper.IsMessageWaiting());
#else
	assert(Looper.IsMessageWaiting());
#endif
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
#ifndef TEST_R5
	assert(!Looper.IsMessageWaiting());
#else
	assert(Looper.IsMessageWaiting());
#endif
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

