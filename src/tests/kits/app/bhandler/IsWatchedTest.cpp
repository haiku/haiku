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
	assert(!fHandler.IsWatched());
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
	assert(fHandler.IsWatched() == true);

	fHandler.StopWatching(&Watcher, '1234');
	assert(fHandler.IsWatched() == false);
}
//------------------------------------------------------------------------------
Test* TIsWatchedTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::IsWatched");

	ADD_TEST(SuiteOfTests, TIsWatchedTest, IsWatched1);
	ADD_TEST(SuiteOfTests, TIsWatchedTest, IsWatched2);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

