//------------------------------------------------------------------------------
//	SetFilterListTest.cpp
//
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <be/app/Looper.h>
#if defined(TEST_R5)
#include <be/app/MessageFilter.h>
#else
#include <MessageFilter.h>
#endif

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "SetFilterListTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
	SetFilterList(BList* filters)
	@case			filters is NULL
	@param	filters	NULL
	@results		FilterList() returns NULL
 */
void TSetFilterListTest::SetFilterList1()
{
	BHandler Handler;
	Handler.SetFilterList(NULL);
	assert(!Handler.FilterList());
}
//------------------------------------------------------------------------------
/**
	SetFilterList(BList* filters)
	@case			filters is valid, handler has no looper
	@param	filters	Valid pointer to BList of BMessageFilters
	@results		FilterList() returns assigned list
 */
void TSetFilterListTest::SetFilterList2()
{
	BList* Filters = new BList;
	BMessageFilter* Filter = new BMessageFilter('1234');
	Filters->AddItem((void*)Filter);
	BHandler Handler;
	Handler.SetFilterList(Filters);
	assert(Handler.FilterList() == Filters);
}
//------------------------------------------------------------------------------
/**
	SetFilterList(BList* filters)
	@case			filters is valid, handler has looper, looper isn't locked
	@param	filters	Valid pointer to BList of BMessageFilters
	@results		FilterList() returns NULL; list is not assigned
					debug message "Owning Looper must be locked before calling
					SetFilterList"
 */
void TSetFilterListTest::SetFilterList3()
{
	BLooper Looper;
	BHandler Handler;

	BList* Filters = new BList;
	BMessageFilter* Filter = new BMessageFilter('1234');
	Filters->AddItem((void*)Filter);

	Looper.AddHandler(&Handler);
	if (Looper.IsLocked())
	{
		Looper.Unlock();
	}

	Handler.SetFilterList(Filters);
	assert(!Handler.FilterList());
}
//------------------------------------------------------------------------------
/**
	SetFilterList(BList* filters)
	@case			filters is valid, handler has looper, looper is locked
	@param	filters
	@results
 */
void TSetFilterListTest::SetFilterList4()
{
	BList* Filters = new BList;
	BMessageFilter* Filter = new BMessageFilter('1234');
	Filters->AddItem((void*)Filter);
	BLooper Looper;
	BHandler Handler;
	Looper.Lock();
	Looper.AddHandler(&Handler);
	Handler.SetFilterList(Filters);
	assert(Handler.FilterList() == Filters);
}
//------------------------------------------------------------------------------
/**
	SetFilterList(BList* filters)
	@case			filters and handler are valid; then NULL filters is passed
	@param	filters
	@results
 */
void TSetFilterListTest::SetFilterList5()
{
	BList* Filters = new BList;
	BMessageFilter* Filter = new BMessageFilter('1234');
	Filters->AddItem((void*)Filter);
	BLooper Looper;
	BHandler Handler;
	Looper.Lock();
	Looper.AddHandler(&Handler);
	Handler.SetFilterList(Filters);
	assert(Handler.FilterList() == Filters);

	Handler.SetFilterList(NULL);
	assert(!Handler.FilterList());
}
//------------------------------------------------------------------------------
Test* TSetFilterListTest::Suite()
{
	TestSuite* SuiteOfTests = new TestSuite("BHandler::SetFilterList");

	ADD_TEST(SuiteOfTests, TSetFilterListTest, SetFilterList1);
	ADD_TEST(SuiteOfTests, TSetFilterListTest, SetFilterList2);
	ADD_TEST(SuiteOfTests, TSetFilterListTest, SetFilterList3);
	ADD_TEST(SuiteOfTests, TSetFilterListTest, SetFilterList4);
	ADD_TEST(SuiteOfTests, TSetFilterListTest, SetFilterList5);

	return SuiteOfTests;
}
//------------------------------------------------------------------------------

/*
 * $Log $
 *
 * $Id  $
 *
 */

