//------------------------------------------------------------------------------
//	LooperTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <be/app/Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "LooperTest.h"

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
	assert(fHandler.Looper() == NULL);
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
	assert(fHandler.Looper() == &Looper);

	assert(Looper.RemoveHandler(&fHandler));
	assert(fHandler.Looper() == NULL);
}
//------------------------------------------------------------------------------
Test* TLooperTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::Looper");

	ADD_TEST(SuiteOfTests, TLooperTest, LooperTest1);
	ADD_TEST(SuiteOfTests, TLooperTest, LooperTest2);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

