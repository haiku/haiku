//------------------------------------------------------------------------------
//	RemoveCommonFilterTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>
#include <MessageFilter.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "RemoveCommonFilterTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	RemoveCommonFilter(BMessageFilter* filter)
	@case		NULL filter
	@param		filter is NULL
	@results
 */
void TRemoveCommonFilterTest::RemoveCommonFilterTest1()
{
	BLooper Looper;
	CPPUNIT_ASSERT(!Looper.RemoveCommonFilter(NULL));
}
//------------------------------------------------------------------------------
/**
	RemoveCommonFilter(BMessageFilter* filter)
	@case		Valid filter, looper not locked
	@param		Valid BMessageFilter pointer
	@results
 */
void TRemoveCommonFilterTest::RemoveCommonFilterTest2()
{
	DEBUGGER_ESCAPE;

	BLooper Looper;
	BMessageFilter* Filter = new BMessageFilter('1234');
	Looper.AddCommonFilter(Filter);
	Looper.Unlock();
	CPPUNIT_ASSERT(!Looper.RemoveCommonFilter(Filter));
}
//------------------------------------------------------------------------------
/**
	RemoveCommonFilter(BMessageFilter* filter)
	@case		Valid filter, not owned by looper
	@param		Valid BMessageFilter pointer
	@results
 */
void TRemoveCommonFilterTest::RemoveCommonFilterTest3()
{
	BLooper Looper;
	BMessageFilter Filter('1234');
	CPPUNIT_ASSERT(!Looper.RemoveCommonFilter(&Filter));
}
//------------------------------------------------------------------------------
/**
	RemoveCommonFilter(BMessageFilter* filter)
	@case		Valid filter, owned by looper
	@param		Valid BMessageFilter pointer
	@results
 */
void TRemoveCommonFilterTest::RemoveCommonFilterTest4()
{
	BLooper Looper;
	BMessageFilter Filter('1234');
	Looper.AddCommonFilter(&Filter);
	CPPUNIT_ASSERT(Looper.RemoveCommonFilter(&Filter));
}
//------------------------------------------------------------------------------
#ifdef ADD_TEST
#undef ADD_TEST
#endif
#define ADD_TEST(__test_name__)	\
	ADD_TEST4(BLooper, suite, TRemoveCommonFilterTest, __test_name__)
TestSuite* TRemoveCommonFilterTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::RemoveCommonFilter(BMessageFilter*)");

	ADD_TEST(RemoveCommonFilterTest1);
	ADD_TEST(RemoveCommonFilterTest2);
	ADD_TEST(RemoveCommonFilterTest3);
	ADD_TEST(RemoveCommonFilterTest4);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

