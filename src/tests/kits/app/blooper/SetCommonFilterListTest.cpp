//------------------------------------------------------------------------------
//	SetCommonFilterListTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <List.h>
#include <Looper.h>
#include <MessageFilter.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "SetCommonFilterListTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	SetCommonFilterList(BList* filters)
	case	NULL list
 */
void TSetCommonFilterListTest::SetCommonFilterListTest1()
{
	BLooper Looper;
	Looper.SetCommonFilterList(NULL);
	CPPUNIT_ASSERT(Looper.CommonFilterList() == NULL);
}
//------------------------------------------------------------------------------
/**
	SetCommonFilterList(BList* filters)
	case	Valid list, looper not locked
 */
void TSetCommonFilterListTest::SetCommonFilterListTest2()
{
	BLooper Looper;
	BList* filters = new BList;
	filters->AddItem(new BMessageFilter('1234'));
	Looper.Unlock();
	Looper.SetCommonFilterList(filters);
	CPPUNIT_ASSERT(Looper.CommonFilterList() == NULL);
}
//------------------------------------------------------------------------------
/**
	SetCommonFilterList(BList* filters)
	case	Valid list, looper locked
 */
void TSetCommonFilterListTest::SetCommonFilterListTest3()
{
	BLooper Looper;
	BList* filters = new BList;
	filters->AddItem(new BMessageFilter('1234'));
	Looper.SetCommonFilterList(filters);
	CPPUNIT_ASSERT(Looper.CommonFilterList() == filters);
}
//------------------------------------------------------------------------------
/**
	SetCommonFilterList(BList* filters)
	case	Valid list, looper locked, owned by another looper
 */
void TSetCommonFilterListTest::SetCommonFilterListTest4()
{
	DEBUGGER_ESCAPE;
	BLooper Looper1;
	BLooper Looper2;
	BList* filters = new BList;
	filters->AddItem(new BMessageFilter('1234'));
	Looper1.SetCommonFilterList(filters);
	Looper2.SetCommonFilterList(filters);
	CPPUNIT_ASSERT(Looper1.CommonFilterList() == filters);
	CPPUNIT_ASSERT(Looper2.CommonFilterList() == filters);
}
//------------------------------------------------------------------------------
#ifdef ADD_TEST
#undef ADD_TEST
#endif
#define ADD_TEST(__test_name__)	\
	ADD_TEST4(BLooper, suite, TSetCommonFilterListTest, __test_name__)
TestSuite* TSetCommonFilterListTest::Suite()
{
	TestSuite* suite = new TestSuite("BLooper::SetCommonFilterList(BList*)");

	ADD_TEST(SetCommonFilterListTest1);
	ADD_TEST(SetCommonFilterListTest2);
	ADD_TEST(SetCommonFilterListTest3);
	ADD_TEST(SetCommonFilterListTest4);

	return suite;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

