/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Looper.h>
#include <MessageFilter.h>

#include "common.h"

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class LooperCommonFilterTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(LooperCommonFilterTest);
	CPPUNIT_TEST(Null_AddCommonFilter_Succeeds);
	CPPUNIT_TEST(Unlocked_AddCommonFilter_ReturnsNull);
	CPPUNIT_TEST(Locked_AddCommonFilter_Succeeds);
	CPPUNIT_TEST(AlreadyOwned_AddCommonFilter_ReturnsNull);
	CPPUNIT_TEST(Null_RemoveCommonFilter_ReturnsFalse);
	CPPUNIT_TEST(Unlocked_RemoveCommonFilter_ReturnsFalse);
	CPPUNIT_TEST(NotOwned_RemoveCommonFilter_ReturnsFalse);
	CPPUNIT_TEST(Owned_RemoveCommonFilter_ReturnsTrue);
	CPPUNIT_TEST(Null_SetCommonFilterList_ReturnsNull);
	CPPUNIT_TEST(Unlocked_SetCommonFilterList_ReturnsNull);
	CPPUNIT_TEST(Locked_SetCommonFilterList_Succeeds);
	CPPUNIT_TEST(AlreadyOwned_SetCommonFilterList_CorrectReturns);
	CPPUNIT_TEST_SUITE_END();

public:
	/**
		AddCommonFilter(BMessageFilter* filter)
		@case		NULL filter
		@param		filter is NULL
		@results	none
		@note		R5 chokes on this test; doesn't param check, apparently.
	 */
	void Null_AddCommonFilter_Succeeds()
	{
	#ifndef TEST_R5
		BLooper Looper;
		Looper.AddCommonFilter(NULL);
	#endif
	}

	/**
		AddCommonFilter(BMessageFilter* filter)
		@case		Valid filter, looper not locked
		@param		Valid BMessageFilter pointer
		@results	Debugger message "Owning Looper must be locked before calling
					AddCommonFilter".  Also, since internal filter list construction
					is delayed until a filter is added, CommonFilterList() should
					return NULL.
	 */
	void Unlocked_AddCommonFilter_ReturnsNull()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		Looper.Unlock();
		BMessageFilter* Filter = new BMessageFilter('1234');
		Looper.AddCommonFilter(Filter);
		CPPUNIT_ASSERT(Looper.CommonFilterList() == NULL);
	}

	/**
		AddCommonFilter(BMessageFilter* filter)
		@case		Valid filter, looper locked
		@param		Valid BMessageFilter pointer
		@results	Filter is successfully added to the common filter list
	 */
	void Locked_AddCommonFilter_Succeeds()
	{
		BLooper Looper;
		BMessageFilter* Filter = new BMessageFilter('1234');
		Looper.AddCommonFilter(Filter);
		CPPUNIT_ASSERT(Looper.CommonFilterList()->HasItem(Filter));
	}

	/**
		AddCommonFilter(BMessageFilter* filter)
		@case		Valid filter, looper locked, owned by another looper
		@param		Valid BMessageFilter pointer
		@results	Debugger message "A MessageFilter can only be used once."  Also,
					since internal filter list construction is delayed until a
					filter is added, Looper2.CommonFilterList() should return NULL.
	 */
	void AlreadyOwned_AddCommonFilter_ReturnsNull()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper1;
		BLooper Looper2;
		BMessageFilter* Filter = new BMessageFilter('1234');
		Looper1.AddCommonFilter(Filter);
		CPPUNIT_ASSERT(Looper1.CommonFilterList()->HasItem(Filter));
		Looper2.AddCommonFilter(Filter);
		CPPUNIT_ASSERT(Looper2.CommonFilterList() == NULL);
	}

	/**
		RemoveCommonFilter(BMessageFilter* filter)
		@case		NULL filter
		@param		filter is NULL
		@results
	 */
	void Null_RemoveCommonFilter_ReturnsFalse()
	{
		BLooper Looper;
		CPPUNIT_ASSERT(!Looper.RemoveCommonFilter(NULL));
	}

	/**
		RemoveCommonFilter(BMessageFilter* filter)
		@case		Valid filter, looper not locked
		@param		Valid BMessageFilter pointer
		@results
	 */
	void Unlocked_RemoveCommonFilter_ReturnsFalse()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		BMessageFilter* Filter = new BMessageFilter('1234');
		Looper.AddCommonFilter(Filter);
		Looper.Unlock();
		CPPUNIT_ASSERT(!Looper.RemoveCommonFilter(Filter));
	}

	/**
		RemoveCommonFilter(BMessageFilter* filter)
		@case		Valid filter, not owned by looper
		@param		Valid BMessageFilter pointer
		@results
	 */
	void NotOwned_RemoveCommonFilter_ReturnsFalse()
	{
		BLooper Looper;
		BMessageFilter Filter('1234');
		CPPUNIT_ASSERT(!Looper.RemoveCommonFilter(&Filter));
	}

	/**
		RemoveCommonFilter(BMessageFilter* filter)
		@case		Valid filter, owned by looper
		@param		Valid BMessageFilter pointer
		@results
	 */
	void Owned_RemoveCommonFilter_ReturnsTrue()
	{
		BLooper Looper;
		BMessageFilter Filter('1234');
		Looper.AddCommonFilter(&Filter);
		CPPUNIT_ASSERT(Looper.RemoveCommonFilter(&Filter));
	}

	/**
		SetCommonFilterList(BList* filters)
		@case			NULL list
		@param	filters	NULL
		@result			CommonFilterList() returns NULL
	 */
	void Null_SetCommonFilterList_ReturnsNull()
	{
		BLooper Looper;
		Looper.SetCommonFilterList(NULL);
		CPPUNIT_ASSERT(Looper.CommonFilterList() == NULL);
	}

	/**
		SetCommonFilterList(BList* filters)
		@case			Valid list, looper not locked
		@param	filters	Valid pointer to BList containing BMessageFilter
		@result			Debugger message: "Owning Looper must be locked before
						calling SetCommonFilterList".  CommonFilterList() returns
						NULL.
	 */
	void Unlocked_SetCommonFilterList_ReturnsNull()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		BList* filters = new BList;
		filters->AddItem(new BMessageFilter('1234'));
		Looper.Unlock();
		Looper.SetCommonFilterList(filters);
		CPPUNIT_ASSERT(Looper.CommonFilterList() == NULL);
	}

	/**
		SetCommonFilterList(BList* filters)
		@case			Valid list, looper locked
		@param	filters	Valid pointer to BList containing BMessageFilter
		@result			CommonFilterList() returns filters.
	 */
	void Locked_SetCommonFilterList_Succeeds()
	{
		BLooper Looper;
		BList* filters = new BList;
		filters->AddItem(new BMessageFilter('1234'));
		Looper.SetCommonFilterList(filters);
		CPPUNIT_ASSERT(Looper.CommonFilterList() == filters);
	}

	/**
		SetCommonFilterList(BList* filters)
		@case			Valid list, looper locked, owned by another looper
		@param	filters	Valid pointer to BList containing BMessageFilter
		@result			Looper2.CommonFilterList() returns NULL.
		@note			R5 implementation segfaults when the first looper is
						destroyed (it goes last because it was constructed first)
						because it tries to clean up the filter list which has
						already been cleaned up by the other looper.  Now fixed in
						Haiku.
	 */
	void AlreadyOwned_SetCommonFilterList_CorrectReturns()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper1;
		BLooper Looper2;
		BList* filters = new BList;
		filters->AddItem(new BMessageFilter('1234'));
		Looper1.SetCommonFilterList(filters);
		Looper2.SetCommonFilterList(filters);
		CPPUNIT_ASSERT(Looper1.CommonFilterList() == filters);
	#ifndef TEST_R5
		CPPUNIT_ASSERT(Looper2.CommonFilterList() == NULL);
	#else
		CPPUNIT_ASSERT(Looper2.CommonFilterList() == filters);
	#endif
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(LooperCommonFilterTest, getTestSuiteName());
