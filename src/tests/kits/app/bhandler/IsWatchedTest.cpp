//------------------------------------------------------------------------------
//	IsWatchedTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "IsWatchedTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	IsWatched()
	@case		No added watchers
	@results	Returns false
 */
void TIsWatchedTest::IsWatched1()
{
	CPPUNIT_ASSERT(!fHandler.IsWatched());
}
//------------------------------------------------------------------------------
/**
	IsWatched()
	@case		Add then remove watcher
	@results	Returns true after add, returns false after remove
	@note		Original implementation fails this test.  Either the removal
				doesn't happen (unlikely) or some list-within-a-list doesn't
				get removed when there's nothing in it anymore.
 */
void TIsWatchedTest::IsWatched2()
{
	BHandler Watcher;
	fHandler.StartWatching(&Watcher, '1234');
	CPPUNIT_ASSERT(fHandler.IsWatched() == true);

	fHandler.StopWatching(&Watcher, '1234');
#ifndef TEST_R5
	CPPUNIT_ASSERT(fHandler.IsWatched() == false);
#endif
}
//------------------------------------------------------------------------------
Test* TIsWatchedTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::IsWatched");

	ADD_TEST4(BHandler, SuiteOfTests, TIsWatchedTest, IsWatched1);
	ADD_TEST4(BHandler, SuiteOfTests, TIsWatchedTest, IsWatched2);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


