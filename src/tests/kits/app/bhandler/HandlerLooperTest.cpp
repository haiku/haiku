//------------------------------------------------------------------------------
//	LooperTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "HandlerLooperTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	Looper()
	@case		Not added to a BLooper
	@results		Returns NULL
 */
void TLooperTest::LooperTest1()
{
	CPPUNIT_ASSERT(fHandler.Looper() == NULL);
}
//------------------------------------------------------------------------------
/**
	Looper()
	@case		Add to a BLooper, then remove
	@results	Returns the added-to BLooper; when removed, returns NULL
 */
void TLooperTest::LooperTest2()
{
	BLooper Looper;
	Looper.AddHandler(&fHandler);
	CPPUNIT_ASSERT(fHandler.Looper() == &Looper);

	CPPUNIT_ASSERT(Looper.RemoveHandler(&fHandler));
	CPPUNIT_ASSERT(fHandler.Looper() == NULL);
}
//------------------------------------------------------------------------------
Test* TLooperTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::Looper");

	ADD_TEST4(BHandler, SuiteOfTests, TLooperTest, LooperTest1);
	ADD_TEST4(BHandler, SuiteOfTests, TLooperTest, LooperTest2);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


