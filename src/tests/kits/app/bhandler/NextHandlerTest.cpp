//------------------------------------------------------------------------------
//	NextHandlerTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <be/app/Looper.h>

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
	assert(Handler.NextHandler() == NULL);
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
	assert(Handler.NextHandler() == &Looper);
}
//------------------------------------------------------------------------------
Test* TNextHandlerTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::NextHandler");

	ADD_TEST(SuiteOfTests, TNextHandlerTest, NextHandler1);
	ADD_TEST(SuiteOfTests, TNextHandlerTest, NextHandler2);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

