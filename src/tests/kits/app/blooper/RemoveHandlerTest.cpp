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
	@case		Valid looper and handler; handler has filters
	@param		handler	Valid BHandler pointer
	@results	RemoveHandler() returns true
				handler->FilterList() returns NULL after removal
 */
void TRemoveHandlerTest::RemoveHandler1()
{
	BLooper Looper;
	BHandler Handler;
	BMessageFilter* MessageFilter = new BMessageFilter('1234');

	Handler.AddFilter(MessageFilter);
	Looper.AddHandler(&Handler);
	Looper.RemoveHandler(&Handler);
	assert(Handler.FilterList());
}
//------------------------------------------------------------------------------
Test* TRemoveHandlerTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::RemoveHandler(BHandler* handler)");

	ADD_TEST(suite, TRemoveHandlerTest, RemoveHandler1);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

