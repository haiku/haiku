//------------------------------------------------------------------------------
//	NextHandlerTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "NextHandlerTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	NextHandler()
	@case		Default constructed BHandler
	@results	Returns NULL
 */
void TNextHandlerTest::NextHandler1()
{
	BHandler Handler;
	CPPUNIT_ASSERT(Handler.NextHandler() == NULL);
}
//------------------------------------------------------------------------------
/**
	NextHandler();
	@case		Default constructed BHandler added to BLooper
	@results	Returns parent BLooper
 */
void TNextHandlerTest::NextHandler2()
{
	BHandler Handler;
	BLooper Looper;
	Looper.AddHandler(&Handler);
	CPPUNIT_ASSERT(Handler.NextHandler() == &Looper);
}
//------------------------------------------------------------------------------
Test* TNextHandlerTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::NextHandler");

	ADD_TEST4(BHandler, SuiteOfTests, TNextHandlerTest, NextHandler1);
	ADD_TEST4(BHandler, SuiteOfTests, TNextHandlerTest, NextHandler2);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


