//------------------------------------------------------------------------------
//	RemoveFilterTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>
#include <MessageFilter.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "RemoveFilterTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	RemoveFilter(BMessageFilter* filter)
	@case			filter is NULL
	@param	filter	NULL
	@results		Returns false
 */
void TRemoveFilterTest::RemoveFilter1()
{
	BHandler Handler;
	CPPUNIT_ASSERT(!Handler.RemoveFilter(NULL));
}
//------------------------------------------------------------------------------
/**
	RemoveFilter(BMessageFilter* filter)
	@case			filter is valid, handler has no looper
	@param	filter	Valid BMessageFilter pointer
	@results		Returns true.  Contrary to documentation, original
					implementation of BHandler doesn't care if it belongs to a
					looper or not.
 */
void TRemoveFilterTest::RemoveFilter2()
{
	BHandler Handler;
	BMessageFilter* Filter = new BMessageFilter('1234');
	Handler.AddFilter(Filter);
	CPPUNIT_ASSERT(Handler.RemoveFilter(Filter));
}
//------------------------------------------------------------------------------
/**
	RemoveFilter(BMessageFilter* filter)
	@case			filter is valid, handler has looper, looper isn't locked
	@param	filter	Valid BMessageFilter pointer
	@results		Returns true.  Contrary to documentation, original
					implementation of BHandler doesn't care if it belongs to a
					looper or not.
 */
void TRemoveFilterTest::RemoveFilter3()
{
	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	BMessageFilter* Filter = new BMessageFilter('1234');
	Handler.AddFilter(Filter);
	CPPUNIT_ASSERT(Handler.RemoveFilter(Filter));
}
//------------------------------------------------------------------------------
/**
	RemoveFilter(BMessageFilter* filter)
	@case			filter is valid, handler has looper, looper is locked
	@param	filter	Valid BMessageFilter pointer
	@results		Return true.
 */
void TRemoveFilterTest::RemoveFilter4()
{
	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	Looper.Lock();
	BMessageFilter* Filter = new BMessageFilter('1234');
	Handler.AddFilter(Filter);
	CPPUNIT_ASSERT(Handler.RemoveFilter(Filter));
}
//------------------------------------------------------------------------------
/**
	RemoveFilter(BMessageFilter* filter)
	@case			filter is valid, but not owned by handler, handler has no looper
	@param	filter	Valid BMessageFilter pointer
	@results		Returns false.  Contrary to documentation, original
					implementation of BHandler doesn't care if it belongs to a
					looper or not.
 */
void TRemoveFilterTest::RemoveFilter5()
{
	BHandler Handler;
	BMessageFilter* Filter = new BMessageFilter('1234');
	CPPUNIT_ASSERT(!Handler.RemoveFilter(Filter));
}
//------------------------------------------------------------------------------
/**
	RemoveFilter(BMessageFilter* filter)
	@case			filter is valid, but not owned by handler, handler has
					looper, looper isn't locked
	@param	filter	Valid BMessageFilter pointer
	@results		Returns false.  Contrary to documentation, original
					implementation of BHandler doesn't care if its looper is
					locked or not.
 */
void TRemoveFilterTest::RemoveFilter6()
{
	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	BMessageFilter* Filter = new BMessageFilter('1234');
	CPPUNIT_ASSERT(!Handler.RemoveFilter(Filter));
}
//------------------------------------------------------------------------------
/**
	RemoveFilter(BMessageFilter* filter)
	@case			filter is valid, but not owned by handler, handler has
					looper, looper is locked
	@param	filter	Valid BMessageFilter pointer
	@results		Returns false.
 */
void TRemoveFilterTest::RemoveFilter7()
{
	BLooper Looper;
	BHandler Handler;
	Looper.AddHandler(&Handler);
	Looper.Lock();
	BMessageFilter* Filter = new BMessageFilter('1234');
	CPPUNIT_ASSERT(!Handler.RemoveFilter(Filter));
}
//------------------------------------------------------------------------------
Test* TRemoveFilterTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::RemoveFilter");

	ADD_TEST4(BHandler, SuiteOfTests, TRemoveFilterTest, RemoveFilter1);
	ADD_TEST4(BHandler, SuiteOfTests, TRemoveFilterTest, RemoveFilter2);
	ADD_TEST4(BHandler, SuiteOfTests, TRemoveFilterTest, RemoveFilter3);
	ADD_TEST4(BHandler, SuiteOfTests, TRemoveFilterTest, RemoveFilter4);
	ADD_TEST4(BHandler, SuiteOfTests, TRemoveFilterTest, RemoveFilter5);
	ADD_TEST4(BHandler, SuiteOfTests, TRemoveFilterTest, RemoveFilter6);
	ADD_TEST4(BHandler, SuiteOfTests, TRemoveFilterTest, RemoveFilter7);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */


