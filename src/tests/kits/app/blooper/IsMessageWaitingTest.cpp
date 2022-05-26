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
/**
	IsMessageWaiting()
	@case		looper is unlocked and queue is empty
	@results	IsMessageWaiting() returns false
 */
void TIsMessageWaitingTest::IsMessageWaiting1()
{
	DEBUGGER_ESCAPE;

	BLooper Looper;
	Looper.Unlock();
	CPPUNIT_ASSERT(!Looper.IsMessageWaiting());
}
//------------------------------------------------------------------------------
/**
	IsMessageWaiting()
	@case		looper is unlocked and queue is filled
	@results	IsMessageWaiting() returns false
 */
void TIsMessageWaitingTest::IsMessageWaiting2()
{
	DEBUGGER_ESCAPE;

	BLooper Looper;
	Looper.Unlock();
	Looper.PostMessage('1234');
	CPPUNIT_ASSERT(!Looper.IsMessageWaiting());
}
//------------------------------------------------------------------------------
/**
	IsMessageWaiting()
	@case		looper is locked and queue is empty
	@results	IsMessageWaiting() returns false
	@note		R5 will return true in this test.  The extra testing below
				indicates that the R5 version probably returns != 0 from
				port_buffer_size_etc(), resulting in an incorrect true in cases
				where the operation would block.
 */
void TIsMessageWaitingTest::IsMessageWaiting3()
{
	BLooper Looper;
	Looper.Lock();
#ifndef TEST_R5
	CPPUNIT_ASSERT(!Looper.IsMessageWaiting());
#else
#if 0
	// Testing to figure out why we get false positives from the R5
	// implementation of BLooper::IsMessageWaiting().  Basically, it tests for
	// port_buffer_size_etc() != 0 -- which means that return values like
	// B_WOULD_BLOCK make the function return true, which is just not correct.
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
/**
	IsMessageWaiting()
	@case		looper is locked and queue is filled
	@results	IsMessageWaiting() returns true.
 */
void TIsMessageWaitingTest::IsMessageWaiting4()
{
	BLooper Looper;
	Looper.Lock();
	Looper.PostMessage('1234');
	CPPUNIT_ASSERT(Looper.IsMessageWaiting());
}
//------------------------------------------------------------------------------
/**
	IsMessageWaiting()
	@case		looper is locked, message is posted, queue is empty
	@results	IsMessageWaiting() returns true.
	@note		The first assert always worked under R5 but only sometimes for
				Haiku.  Answer: the Haiku implementation of BLooper was attempting
				to lock itself prior to fetching the message from the queue.  I
				moved the lock attempt after the fetch and it worked the same.
				I realized that if the system was loaded heavily enough, the
				assert might still fail simply because the looper would not have
				had enough time to get to the fetch (thereby emptying the queue),
				so the assert is no longer used.  If we do manage to call
				IsMessageWaiting() before the fetch happens (which does happen
				every once in a while), we still get a true result because the
				port buffer is checked.  Later: it's finally dawned on me that
				if the system is loaded *lightly* enough, the message will not
				only get fetched, but popped off the queue as well.  Since R5
				returns the bogus true, the second assert works even when the
				message has been de-queued.  Haiku, of course, will (correctly)
				fail the assert in that situation.  Unfortunately, that renders
				this test completely unreliable.  It is pulled until a fully
				reliable test can be devised.
 */
void TIsMessageWaitingTest::IsMessageWaiting5()
{
	BLooper* Looper = new BLooper(__PRETTY_FUNCTION__);
	Looper->Run();

	// Prevent a port read
	Looper->Lock();
	Looper->PostMessage('1234');
//	CPPUNIT_ASSERT(Looper->MessageQueue()->IsEmpty());
	CPPUNIT_ASSERT(Looper->IsMessageWaiting());

#if 0
	ssize_t count;
	do
	{
		count = port_buffer_size_etc(_get_looper_port_(Looper), B_TIMEOUT, 0);
	} while (count == B_INTERRUPTED);

	cout << endl << "port_buffer_size_etc: ";
	if (count < 0)
	{
		cout << strerror(count);
	}
	else
	{
		cout << count << endl;
		char* buffer = new char[count];
		int32 code;
		read_port(_get_looper_port_(Looper), &code, (void*)buffer, count);
		cout << "code: " << code << endl;
		cout << "buffer: ";
		for (int32 i = 0; i < count; ++i)
		{
			cout << buffer[i];
		}
		cout << endl;
	}
	cout << endl;
#endif
}
//------------------------------------------------------------------------------
Test* TIsMessageWaitingTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::IsMessageWaiting()");

	ADD_TEST4(BLooper, suite, TIsMessageWaitingTest, IsMessageWaiting1);
	ADD_TEST4(BLooper, suite, TIsMessageWaitingTest, IsMessageWaiting2);
	ADD_TEST4(BLooper, suite, TIsMessageWaitingTest, IsMessageWaiting3);
	ADD_TEST4(BLooper, suite, TIsMessageWaitingTest, IsMessageWaiting4);

	// See note for test
//	ADD_TEST4(BLooper, suite, TIsMessageWaitingTest, IsMessageWaiting5);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


