//------------------------------------------------------------------------------
//	AddHandlerTest.cpp
//
//------------------------------------------------------------------------------
/**
	@note	Most of AddHandler()'s functionality is indirectly exercised
			by the tests for RemoveHandler(), CountHandler(), HandlerAt() and
			IndexOf().  If AddHandler() isn't working correctly, it will show up
			there.  I do wonder if I should replicate those tests here anyway so
			that any problem specifically show up in this test suite.
 */

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "AddHandlerTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	AddHandler(BHandler*)
	@case		handler is NULL
	@param		handler
	@results	Nothing (bad) should happen when AddHandler() is called, and
				CountHandlers() should return 1 (for the looper itself).  R5
				can't handle this test; it has a segment violation.
 */
void TAddHandlerTest::AddHandlerTest1()
{
	BLooper Looper;
#ifndef TEST_R5
	Looper.AddHandler(NULL);
#endif
	CPPUNIT_ASSERT(Looper.CountHandlers() == 1);
}
//------------------------------------------------------------------------------
/**
	AddHandler(BHandler*)
	@case		looper is unlocked
	@param		handler
	@results	Goes to debugger with message "Looper must be locked before
				calling AddHandler."
 */
void TAddHandlerTest::AddHandlerTest2()
{
	DEBUGGER_ESCAPE;

	BLooper Looper;
	BHandler Handler;
	Looper.Unlock();
	Looper.AddHandler(&Handler);
}
//------------------------------------------------------------------------------
TestSuite* TAddHandlerTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::AddHandler(BHandler*)");

	ADD_TEST4(BLooper, suite, TAddHandlerTest, AddHandlerTest1);
	ADD_TEST4(BLooper, suite, TAddHandlerTest, AddHandlerTest2);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


