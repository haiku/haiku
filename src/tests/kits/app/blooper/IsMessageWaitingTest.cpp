//------------------------------------------------------------------------------
//	IsMessageWaitingTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <iostream>
#include <posix/string.h>

// System Includes -------------------------------------------------------------
#include <Looper.h>
#include <Message.h>
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
	DEBUGGER_ESCAPE;

	BLooper Looper;
	Looper.Unlock();
	CPPUNIT_ASSERT(!Looper.IsMessageWaiting());
}
//------------------------------------------------------------------------------
//case 2: looper is unlocked and queue is filled
void TIsMessageWaitingTest::IsMessageWaiting2()
{
	DEBUGGER_ESCAPE;

	BLooper Looper;
	Looper.Unlock();
	Looper.PostMessage('1234');
	CPPUNIT_ASSERT(!Looper.IsMessageWaiting());
}
//------------------------------------------------------------------------------
//case 3: looper is locked and queue is empty
void TIsMessageWaitingTest::IsMessageWaiting3()
{
	BLooper Looper;
	Looper.Lock();
#ifndef TEST_R5
	CPPUNIT_ASSERT(!Looper.IsMessageWaiting());
#else
#if 0
	// Testing to figure out why we get false positives from the R5
	// implementation of BLooper::IsMessageWaiting().
	CPPUNIT_ASSERT(Looper.IsLocked());
	CPPUNIT_ASSERT(Looper.MessageQueue()->IsEmpty());

	int32 count;
	do
	{
		count = port_buffer_size_etc(_get_looper_port_(&Looper), B_TIMEOUT, 0);
	} while (count == B_INTERRUPTED);

	CPPUNIT_ASSERT(count < 0);
	cout << endl << "port_buffer_size_etc: " << strerror(count) << endl;
#endif
	CPPUNIT_ASSERT(Looper.IsMessageWaiting());
#endif
}
//------------------------------------------------------------------------------
//case 4: looper is locked and queue is filled
void TIsMessageWaitingTest::IsMessageWaiting4()
{
	BLooper Looper;
	Looper.Lock();
	Looper.PostMessage('1234');
	CPPUNIT_ASSERT(Looper.IsMessageWaiting());
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
	CPPUNIT_ASSERT(Looper->MessageQueue()->IsEmpty());
	CPPUNIT_ASSERT(Looper->IsMessageWaiting());

	int32 count;
	do
	{
		count = port_buffer_size_etc(_get_looper_port_(Looper), B_TIMEOUT, 0);
	} while (count == B_INTERRUPTED);

	cout << endl << "port_buffer_size_etc: ";
	if (count < 0)
		cout << strerror(count);
	else
		cout << count;
	cout << endl;
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

