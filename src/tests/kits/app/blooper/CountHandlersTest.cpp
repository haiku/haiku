//------------------------------------------------------------------------------
//	CountHandlersTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Handler.h>
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "CountHandlersTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	CountHandlers()
	@case		No handlers added
	@results
 */
void TCountHandlersTest::CountHandlersTest1()
{
	BLooper Looper;
	CPPUNIT_ASSERT(Looper.CountHandlers() == 1);
}
//------------------------------------------------------------------------------
/**
	CountHandlers()
	@case		Several handlers added, then removed
	@results
 */
void TCountHandlersTest::CountHandlersTest2()
{
	BLooper Looper;
	BHandler Handler1;
	BHandler Handler2;
	BHandler Handler3;

	Looper.AddHandler(&Handler1);
	CPPUNIT_ASSERT(Looper.CountHandlers() == 2);
	Looper.AddHandler(&Handler2);
	CPPUNIT_ASSERT(Looper.CountHandlers() == 3);
	Looper.AddHandler(&Handler3);
	CPPUNIT_ASSERT(Looper.CountHandlers() == 4);

	Looper.RemoveHandler(&Handler3);
	CPPUNIT_ASSERT(Looper.CountHandlers() == 3);
	Looper.RemoveHandler(&Handler2);
	CPPUNIT_ASSERT(Looper.CountHandlers() == 2);
	Looper.RemoveHandler(&Handler1);
	CPPUNIT_ASSERT(Looper.CountHandlers() == 1);
}
//------------------------------------------------------------------------------
TestSuite* TCountHandlersTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::CountHandlers()");

	ADD_TEST4(BLooper, suite, TCountHandlersTest, CountHandlersTest1);
	ADD_TEST4(BLooper, suite, TCountHandlersTest, CountHandlersTest2);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


