/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Handler.h>
#include <Looper.h>
#include <MessageFilter.h>

#include "common.h"

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class HandlerFilterTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(HandlerFilterTest);
	CPPUNIT_TEST(Null_AddFilter_Succeeds);
	CPPUNIT_TEST(NoLooper_AddFilter_Succeeds);
	CPPUNIT_TEST(UnlockedLooper_AddFilter_Succeeds);
	CPPUNIT_TEST(LockedLooper_AddFilter_Succeeds);

	CPPUNIT_TEST(Null_RemoveFilter_ReturnsFalse);
	CPPUNIT_TEST(Valid_RemoveFilter_ReturnsTrue);
	CPPUNIT_TEST(UnlockedLooper_RemoveFilter_ReturnsTrue);
	CPPUNIT_TEST(LockedLooper_RemoveFilter_ReturnsTrue);
	CPPUNIT_TEST(NotOwned_RemoveFilter_ReturnsFalse);
	CPPUNIT_TEST(NotOwnedUnlockedLooper_RemoveFilter_ReturnsFalse);
	CPPUNIT_TEST(NotOwnedLockedLooper_RemoveFilter_ReturnsFalse);

	CPPUNIT_TEST(Null_SetFilterList_FilterListIsNull);
	CPPUNIT_TEST(NoLooper_SetFilterList_FilterListMatches);
	CPPUNIT_TEST(UnlockedLooper_SetFilterList_FilterListIsNull);
	CPPUNIT_TEST(LockedLooper_SetFilterList_FilterListMatches);
	CPPUNIT_TEST(ValidThenNull_SetFilterList_CorrectValueIsSet);

	CPPUNIT_TEST(Default_FilterList_ReturnsNull);
	CPPUNIT_TEST_SUITE_END();

public:
	/**
		AddFilter(BMessageFilter* filter)
		@case			filter is NULL
		@param	filter	NULL
		@results		None (i.e., no seg faults, etc.)
		@note			Contrary to documentation, BHandler doesn't seem to care if
						it belongs to a BLooper when a filter gets added.  Also,
						the original implementation does not handle a NULL param
						gracefully, so this test is not enabled against it.
	*/
	void Null_AddFilter_Succeeds()
	{
	#ifndef TEST_R5
		BHandler Handler;
		Handler.AddFilter(NULL);
	#endif
	}

	/**
		AddFilter(BMessageFilter* filter)
		@case			filter is valid, handler has no looper
		@param	filter	Valid BMessageFilter pointer
		@results		None (i.e., no seg faults, etc.)
		@note			Contrary to documentation, BHandler doesn't seem to care if
						it belongs to a BLooper when a filter gets added.
	*/
	void NoLooper_AddFilter_Succeeds()
	{
		BHandler Handler;
		BMessageFilter* Filter = new BMessageFilter('1234');
		Handler.AddFilter(Filter);
	}

	/**
		AddFilter(BMessageFilter* filter)
		@case			filter is valid, handler has looper, looper isn't locked
		@param	filter	Valid BMessageFilter pointer
		@results		None (i.e., no seg faults, etc.)
		@note			Contrary to documentation, BHandler doesn't seem to care if
						if belongs to a BLooper when a filter gets added, or
						whether the looper is locked.
	*/
	void UnlockedLooper_AddFilter_Succeeds()
	{
		BLooper Looper;
		BHandler Handler;
		BMessageFilter* Filter = new BMessageFilter('1234');
		Looper.AddHandler(&Handler);
		Handler.AddFilter(Filter);
	}

	/**
		AddFilter(BMessageFilter* filter)
		@case			filter is valid, handler has looper, looper is locked
		@param	filter	Valid BMessageFilter pointer
		@results		None (i.e., no seg faults, etc.)
	*/
	void LockedLooper_AddFilter_Succeeds()
	{
		BLooper Looper;
		BHandler Handler;
		BMessageFilter* Filter = new BMessageFilter('1234');
		Looper.Lock();
		Looper.AddHandler(&Handler);
		Handler.AddFilter(Filter);
	}

		void Null_RemoveFilter_ReturnsFalse()
	{
		BHandler Handler;
		CPPUNIT_ASSERT(!Handler.RemoveFilter(NULL));
	}

	/**
		RemoveFilter(BMessageFilter* filter)
		@case			filter is valid, handler has no looper
		@param	filter	Valid BMessageFilter pointer
		@results		Returns true.  Contrary to documentation, original
						implementation of BHandler doesn't care if it belongs to a
						looper or not.
	*/
	void Valid_RemoveFilter_ReturnsTrue()
	{
		BHandler Handler;
		BMessageFilter* Filter = new BMessageFilter('1234');
		Handler.AddFilter(Filter);
		CPPUNIT_ASSERT(Handler.RemoveFilter(Filter));
	}

	/**
		RemoveFilter(BMessageFilter* filter)
		@case			filter is valid, handler has looper, looper isn't locked
		@param	filter	Valid BMessageFilter pointer
		@results		Returns true.  Contrary to documentation, original
						implementation of BHandler doesn't care if it belongs to a
						looper or not.
	*/
	void UnlockedLooper_RemoveFilter_ReturnsTrue()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		BMessageFilter* Filter = new BMessageFilter('1234');
		Handler.AddFilter(Filter);
		CPPUNIT_ASSERT(Handler.RemoveFilter(Filter));
	}


	/**
		RemoveFilter(BMessageFilter* filter)
		@case			filter is valid, handler has looper, looper is locked
		@param	filter	Valid BMessageFilter pointer
		@results		Return true.
	*/
	void LockedLooper_RemoveFilter_ReturnsTrue()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		Looper.Lock();
		BMessageFilter* Filter = new BMessageFilter('1234');
		Handler.AddFilter(Filter);
		CPPUNIT_ASSERT(Handler.RemoveFilter(Filter));
	}


	/**
		RemoveFilter(BMessageFilter* filter)
		@case			filter is valid, but not owned by handler, handler has no looper
		@param	filter	Valid BMessageFilter pointer
		@results		Returns false.  Contrary to documentation, original
						implementation of BHandler doesn't care if it belongs to a
						looper or not.
	*/
	void NotOwned_RemoveFilter_ReturnsFalse()
	{
		BHandler Handler;
		BMessageFilter* Filter = new BMessageFilter('1234');
		CPPUNIT_ASSERT(!Handler.RemoveFilter(Filter));
	}

	/**
		RemoveFilter(BMessageFilter* filter)
		@case			filter is valid, but not owned by handler, handler has
						looper, looper isn't locked
		@param	filter	Valid BMessageFilter pointer
		@results		Returns false.  Contrary to documentation, original
						implementation of BHandler doesn't care if its looper is
						locked or not.
	*/
	void NotOwnedUnlockedLooper_RemoveFilter_ReturnsFalse()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		BMessageFilter* Filter = new BMessageFilter('1234');
		CPPUNIT_ASSERT(!Handler.RemoveFilter(Filter));
	}

	/**
		RemoveFilter(BMessageFilter* filter)
		@case			filter is valid, but not owned by handler, handler has
						looper, looper is locked
		@param	filter	Valid BMessageFilter pointer
		@results		Returns false.
	*/
	void NotOwnedLockedLooper_RemoveFilter_ReturnsFalse()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		Looper.Lock();
		BMessageFilter* Filter = new BMessageFilter('1234');
		CPPUNIT_ASSERT(!Handler.RemoveFilter(Filter));
	}

	void Null_SetFilterList_FilterListIsNull()
	{
		BHandler Handler;
		Handler.SetFilterList(NULL);
		CPPUNIT_ASSERT(!Handler.FilterList());
	}

	void NoLooper_SetFilterList_FilterListMatches()
	{
		BList* Filters = new BList;
		BMessageFilter* Filter = new BMessageFilter('1234');
		Filters->AddItem((void*)Filter);
		BHandler Handler;
		Handler.SetFilterList(Filters);
		CPPUNIT_ASSERT(Handler.FilterList() == Filters);
	}

	/**
		SetFilterList(BList* filters)
		@case			filters is valid, handler has looper, looper isn't locked
		@param	filters	Valid pointer to BList of BMessageFilters
		@results		FilterList() returns NULL; list is not assigned
						debug message "Owning Looper must be locked before calling
						SetFilterList"
	*/
	void UnlockedLooper_SetFilterList_FilterListIsNull()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		BHandler Handler;

		BList* Filters = new BList;
		BMessageFilter* Filter = new BMessageFilter('1234');
		Filters->AddItem((void*)Filter);

		Looper.AddHandler(&Handler);
		if (Looper.IsLocked())
			Looper.Unlock();

		Handler.SetFilterList(Filters);
		CPPUNIT_ASSERT(!Handler.FilterList());
	}

	void LockedLooper_SetFilterList_FilterListMatches()
	{
		BList* Filters = new BList;
		BMessageFilter* Filter = new BMessageFilter('1234');
		Filters->AddItem((void*)Filter);
		BLooper Looper;
		BHandler Handler;
		Looper.Lock();
		Looper.AddHandler(&Handler);
		Handler.SetFilterList(Filters);
		CPPUNIT_ASSERT(Handler.FilterList() == Filters);
	}

	void ValidThenNull_SetFilterList_CorrectValueIsSet()
	{
		BList* Filters = new BList;
		BMessageFilter* Filter = new BMessageFilter('1234');
		Filters->AddItem((void*)Filter);
		BLooper Looper;
		BHandler Handler;
		Looper.Lock();
		Looper.AddHandler(&Handler);
		Handler.SetFilterList(Filters);
		CPPUNIT_ASSERT(Handler.FilterList() == Filters);

		Handler.SetFilterList(NULL);
		CPPUNIT_ASSERT(!Handler.FilterList());
	}

	/**
		FilterList();
		@case		Default constructed BHandler
		@results	FilterList() returns NULL
	 */
	void Default_FilterList_ReturnsNull()
	{
		BHandler Handler;
		CPPUNIT_ASSERT(!Handler.FilterList());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(HandlerFilterTest, getTestSuiteName());
