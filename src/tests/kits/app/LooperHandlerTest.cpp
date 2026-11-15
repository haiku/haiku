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


/**
	@note	Most of AddHandler()'s functionality is indirectly exercised
			by the tests for RemoveHandler(), CountHandler(), HandlerAt() and
			IndexOf().  If AddHandler() isn't working correctly, it will show up
			there.  I do wonder if I should replicate those tests here anyway so
			that any problem specifically show up in this test suite.
 */

class LooperHandlerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(LooperHandlerTest);
	CPPUNIT_TEST(Null_AddHandler_Succeeds);
	CPPUNIT_TEST(Unlocked_AddHandler_Succeeds);
	CPPUNIT_TEST(Null_RemoveHandler_ReturnsFalse);
	CPPUNIT_TEST(NotOwned_RemoveHandler_ReturnsFalse);
	CPPUNIT_TEST(Unlocked_RemoveHandler_Succeeds);
	CPPUNIT_TEST(NotOwnedUnlocked_RemoveHandler_ReturnsFalse);
	CPPUNIT_TEST(WithFilters_RemoveHandler_Succeeds);
	CPPUNIT_TEST(Default_CountHandlers_ReturnsOne);
	CPPUNIT_TEST(Many_CountHandlers_ReturnsCorrectCount);
	CPPUNIT_TEST(NoHandlersAdded_HandlerAt_ReturnsLooper);
	CPPUNIT_TEST(IndexOutOfRange_HandlerAt_ReturnsNull);
	CPPUNIT_TEST(ManyHandlersAdded_HandlerAt_ReturnsCorrectHandlers);
	CPPUNIT_TEST(Unlocked_HandlerAt_Succeeds);
	CPPUNIT_TEST(Null_IndexOf_ReturnsError);
	CPPUNIT_TEST(NotOwned_IndexOf_ReturnsError);
	CPPUNIT_TEST(Owned_IndexOf_ReturnsCorrectIndex);
	CPPUNIT_TEST(Many_IndexOf_ReturnsCorrectIndices);
	CPPUNIT_TEST(Unlocked_IndexOf_Succeeds);
	CPPUNIT_TEST(Self_IndexOf_ReturnsZero);
	CPPUNIT_TEST_SUITE_END();

public:
	/**
		AddHandler(BHandler*)
		@case		handler is NULL
		@param		handler
		@results	Nothing (bad) should happen when AddHandler() is called, and
					CountHandlers() should return 1 (for the looper itself).  R5
					can't handle this test; it has a segment violation.
	 */
	void Null_AddHandler_Succeeds()
	{
		BLooper Looper;
	#ifndef TEST_R5
		Looper.AddHandler(NULL);
	#endif
		CPPUNIT_ASSERT(Looper.CountHandlers() == 1);
	}

	/**
		AddHandler(BHandler*)
		@case		looper is unlocked
		@param		handler
		@results	Goes to debugger with message "Looper must be locked before
					calling AddHandler."
	 */
	void Unlocked_AddHandler_Succeeds()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		BHandler Handler;
		Looper.Unlock();
		Looper.AddHandler(&Handler);
	}

	/**
		RemoveHandler(BHandler* handler)
		@case		handler is NULL
		@param		handler	NULL
		@results	RemoveHandler() returns false.  R5 implementation seg faults;
					we've fixed that.
	 */
	void Null_RemoveHandler_ReturnsFalse()
	{
		BLooper Looper;
	#ifndef TEST_R5
		CPPUNIT_ASSERT(!Looper.RemoveHandler(NULL));
	#endif
	}

	/**
		RemoveHandler(BHandler* handler)
		@case		handler doesn't belong to this looper
		@param		handler	Valid BHandler pointer, not assigned to looper
		@results
	 */
	void NotOwned_RemoveHandler_ReturnsFalse()
	{
		BLooper Looper;
		BHandler Handler;
		CPPUNIT_ASSERT(!Looper.RemoveHandler(&Handler));
	}

	/**
		RemoveHandler(BHandler* handler)
		@case		handler is valid, looper is unlocked
		@param		handler	Valid BHandler pointer, assigned to looper
		@results	goes to debugger, but removes handler anyway
	 */
	void Unlocked_RemoveHandler_Succeeds()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		Looper.Unlock();
		CPPUNIT_ASSERT(Looper.RemoveHandler(&Handler));
	}

	/**
		RemoveHandler(BHandler* handler)
		@case		handler doesn't belong to this looper, looper is unlocked
		@param		handler Valid BHandler pointer, not assigned to looper
		@results
	 */
	void NotOwnedUnlocked_RemoveHandler_ReturnsFalse()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		BHandler Handler;
		Looper.Unlock();
		CPPUNIT_ASSERT(!Looper.RemoveHandler(&Handler));
	}

	/**
		RemoveHandler(BHandler* handler)
		@case		Valid looper and handler; handler has filters
		@param		handler	Valid BHandler pointer
		@results	RemoveHandler() returns true
					handler->FilterList() returns NULL after removal
	 */
	void WithFilters_RemoveHandler_Succeeds()
	{
		BLooper Looper;
		BHandler Handler;
		BMessageFilter* MessageFilter = new BMessageFilter('1234');

		Handler.AddFilter(MessageFilter);
		Looper.AddHandler(&Handler);
		CPPUNIT_ASSERT(Looper.RemoveHandler(&Handler));
		CPPUNIT_ASSERT(Handler.FilterList());
	}

	void Default_CountHandlers_ReturnsOne()
	{
		BLooper Looper;
		CPPUNIT_ASSERT(Looper.CountHandlers() == 1);
	}

	void Many_CountHandlers_ReturnsCorrectCount()
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


	void NoHandlersAdded_HandlerAt_ReturnsLooper()
	{
		BLooper Looper;
		CPPUNIT_ASSERT(Looper.HandlerAt(0) == &Looper);
	}

	void IndexOutOfRange_HandlerAt_ReturnsNull()
	{
		BLooper Looper;
		CPPUNIT_ASSERT(Looper.HandlerAt(1) == NULL);
		CPPUNIT_ASSERT(Looper.HandlerAt(-1) == NULL);
	}

	void ManyHandlersAdded_HandlerAt_ReturnsCorrectHandlers()
	{
		BLooper Looper;
		BHandler Handler1, Handler2, Handler3;
		Looper.AddHandler(&Handler1);
		Looper.AddHandler(&Handler2);
		Looper.AddHandler(&Handler3);

		CPPUNIT_ASSERT(Looper.HandlerAt(0) == &Looper);
		CPPUNIT_ASSERT(Looper.HandlerAt(1) == &Handler1);
		CPPUNIT_ASSERT(Looper.HandlerAt(2) == &Handler2);
		CPPUNIT_ASSERT(Looper.HandlerAt(3) == &Handler3);
	}

	void Unlocked_HandlerAt_Succeeds()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		Looper.Unlock();
		CPPUNIT_ASSERT(Looper.HandlerAt(0) == &Looper);
	}

	void Null_IndexOf_ReturnsError()
	{
		BLooper Looper;
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(NULL));
	}

	void NotOwned_IndexOf_ReturnsError()
	{
		BLooper Looper;
		BHandler Handler;
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler));
	}

	void Owned_IndexOf_ReturnsCorrectIndex()
	{
		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler) == 1);
	}

	void Many_IndexOf_ReturnsCorrectIndices()
	{
		BLooper Looper;
		BHandler Handler1;
		BHandler Handler2;
		BHandler Handler3;
		BHandler Handler4;
		BHandler Handler5;

		Looper.AddHandler(&Handler1);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
		Looper.AddHandler(&Handler2);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
		Looper.AddHandler(&Handler3);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == 3);
		Looper.AddHandler(&Handler4);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == 3);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler4) == 4);
		Looper.AddHandler(&Handler5);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == 3);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler4) == 4);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler5) == 5);

		// Now we remove them
		Looper.RemoveHandler(&Handler5);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == 3);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler4) == 4);
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler5));
		Looper.RemoveHandler(&Handler4);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler3) == 3);
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler4));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler5));
		Looper.RemoveHandler(&Handler3);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler2) == 2);
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler3));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler4));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler5));
		Looper.RemoveHandler(&Handler2);
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler1) == 1);
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler2));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler3));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler4));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler5));
		Looper.RemoveHandler(&Handler1);
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler1));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler2));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler3));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler4));
		CPPUNIT_ASSERT_EQUAL(B_ERROR, Looper.IndexOf(&Handler5));
	}

	/**
		IndexOf(BHandler* handler)
		@case		handler is valid, looper is unlocked
		@param		handler	Valid BHandler pointer, assigned to this looper
		@results	IndexOf returns 1.  Debugger message "Looper must be locked
					before calling IndexOf."
	 */
	void Unlocked_IndexOf_Succeeds()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		BHandler Handler;
		Looper.AddHandler(&Handler);
		Looper.Unlock();
		CPPUNIT_ASSERT(Looper.IndexOf(&Handler) == 1);
	}

	void Self_IndexOf_ReturnsZero()
	{
		BLooper Looper;
		CPPUNIT_ASSERT(Looper.IndexOf(&Looper) == 0);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(LooperHandlerTest, getTestSuiteName());
