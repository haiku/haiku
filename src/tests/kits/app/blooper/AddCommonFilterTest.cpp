//------------------------------------------------------------------------------
//	AddCommonFilterTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>
#include <MessageFilter.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "AddCommonFilterTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	AddCommonFilter(BMessageFilter* filter)
	@case		NULL filter
	@param		filter is NULL
	@results	none
	@note		R5 chokes on this test; doesn't param check, apparently.
 */
void TAddCommonFilterTest::AddCommonFilterTest1()
{
#ifndef TEST_R5
	BLooper Looper;
	Looper.AddCommonFilter(NULL);
#endif
}
//------------------------------------------------------------------------------
/**
	AddCommonFilter(BMessageFilter* filter)
	@case		Valid filter, looper not locked
	@param		Valid BMessageFilter pointer
	@results	Debugger message "Owning Looper must be locked before calling
				AddCommonFilter"
 */
void TAddCommonFilterTest::AddCommonFilterTest2()
{
	BLooper Looper;
	Looper.Unlock();
	BMessageFilter* Filter = new BMessageFilter('1234');
	Looper.AddCommonFilter(&Filter);
}
//------------------------------------------------------------------------------
/**
	AddCommonFilter(BMessageFilter* filter)
	@case		Valid filter, looper locked
	@param		Valid BMessageFilter pointer
	@results
 */
void TAddCommonFilterTest::AddCommonFilterTest3()
{
	BLooper Looper;
	BMessageFilter* Filter = new BMessageFilter('1234');
	Looper.AddCommonFilter(&Filter);
}
//------------------------------------------------------------------------------
/**
	AddCommonFilter(BMessageFilter* filter)
	@case		Valid filter, looper locked, owned by another looper
	@param		Valid BMessageFilter pointer
	@results
 */
void TAddCommonFilterTest::AddCommonFilterTest4()
{
	BLooper Looper1;
	BLooper Looper2;
	BMessageFilter* Filter = new BMessageFilter('1234');
	Looper1.AddCommonFilter(&Filter);
	Looper2.AddCommonFilter(&Filter);
}
//------------------------------------------------------------------------------
#ifdef ADD_TEST
#undef ADD_TEST
#endif
#define ADD_TEST(__test_name__)	\
	ADD_TEST4(BLooper, suite, TAddCommonFilterTest, __test_name__);
TestSuite* TAddCommonFilterTest::Suite()
{
	TestSuite* suite =
		new TestSuite("BLooper::AddCommonFilter(BMessageFilter*)");

	ADD_TEST(AddCommonFilterTest1);
	ADD_TEST(AddCommonFilterTest2);
	ADD_TEST(AddCommonFilterTest3);
	ADD_TEST(AddCommonFilterTest4);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

