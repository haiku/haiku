//------------------------------------------------------------------------------
//	RemoveHandlerTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Handler.h>
#include <Looper.h>
#include <MessageFilter.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "RemoveHandlerTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	RemoveHandler(BHandler* handler)
	@case		handler is NULL
	@param		handler	NULL
	@results	RemoveHandler() returns false.  R5 implementation seg faults;
				we've fixed that.
 */
void TRemoveHandlerTest::RemoveHandler1()
{
	BLooper Looper;
#ifndef TEST_R5
	CPPUNIT_ASSERT(!Looper.RemoveHandler(NULL));
#endif
}
//------------------------------------------------------------------------------
/**
	RemoveHandler(BHandler* handler)
	@case		handler doesn't belong to this looper
	@param		handler	Valid BHandler pointer, not assigned to looper
	@results	
 */
void TRemoveHandlerTest::RemoveHandler2()
{
	BLooper Looper;
	BHandler Handler;
	CPPUNIT_ASSERT(!Looper.RemoveHandler(&Handler));
}
//------------------------------------------------------------------------------
/**
	RemoveHandler(BHandler* handler)
	@case		handler is valid, looper is unlocked
	@param		handler	Valid BHandler pointer, assigned to looper
	@results	goes to debugger, but removes handler anyway
 */
void TRemoveHandlerTest::RemoveHandler3()
{
	DEBUGGER_ESCAPE;

	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	Looper.Unlock();
	CPPUNIT_ASSERT(Looper.RemoveHandler(&Handler));
}
//------------------------------------------------------------------------------
/**
	RemoveHandler(BHandler* handler)
	@case		handler doesn't belong to this looper, looper is unlocked
	@param		handler Valid BHandler pointer, not assigned to looper
	@results
 */
void TRemoveHandlerTest::RemoveHandler4()
{
	DEBUGGER_ESCAPE;

	BLooper Looper;
	BHandler Handler;
	Looper.Unlock();
	CPPUNIT_ASSERT(!Looper.RemoveHandler(&Handler));
}
//------------------------------------------------------------------------------
/**
	RemoveHandler(BHandler* handler)
	@case		Valid looper and handler; handler has filters
	@param		handler	Valid BHandler pointer
	@results	RemoveHandler() returns true
				handler->FilterList() returns NULL after removal
 */
void TRemoveHandlerTest::RemoveHandler5()
{
	BLooper Looper;
	BHandler Handler;
	BMessageFilter* MessageFilter = new BMessageFilter('1234');

	Handler.AddFilter(MessageFilter);
	Looper.AddHandler(&Handler);
	CPPUNIT_ASSERT(Looper.RemoveHandler(&Handler));
	CPPUNIT_ASSERT(Handler.FilterList());
}
//------------------------------------------------------------------------------
Test* TRemoveHandlerTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::RemoveHandler(BHandler* handler)");

	ADD_TEST4(BLooper, suite, TRemoveHandlerTest, RemoveHandler1);
	ADD_TEST4(BLooper, suite, TRemoveHandlerTest, RemoveHandler2);
	ADD_TEST4(BLooper, suite, TRemoveHandlerTest, RemoveHandler3);
	ADD_TEST4(BLooper, suite, TRemoveHandlerTest, RemoveHandler4);
	ADD_TEST4(BLooper, suite, TRemoveHandlerTest, RemoveHandler5);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


