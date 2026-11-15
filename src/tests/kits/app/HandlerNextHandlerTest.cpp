/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Handler.h>
#include <Looper.h>

#include "common.h"

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class HandlerNextHandlerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(HandlerNextHandlerTest);
	CPPUNIT_TEST(Default_NextHandler_ReturnsNull);
	CPPUNIT_TEST(HandlerInLooper_NextHandler_ReturnsParentLooper);
	CPPUNIT_TEST(Clear_SetNextHandler_ReturnsNull);
	CPPUNIT_TEST(NotBelongingToLooper_SetNextHandler_ReturnsNull);
	CPPUNIT_TEST(UnlockedLooper_SetNextHandler_ReturnsLooper);
	CPPUNIT_TEST(LockedLooperMismatch_SetNextHandler_ReturnsLooper);
	CPPUNIT_TEST(OnlyTargetInUnlockedLooper_SetNextHandler_ReturnsNull);
	CPPUNIT_TEST(OnlyTargetInLockedLooper_SetNextHandler_ReturnsNull);
	CPPUNIT_TEST(BothInDifferentUnlockedLoopers_SetNextHandler_ReturnsLooper1);
	CPPUNIT_TEST(LockedUnlockedMismatch_SetNextHandler_ReturnsLooper1);
	CPPUNIT_TEST(UnlockedLockedMismatch_SetNextHandler_ReturnsLooper1);
	CPPUNIT_TEST(BothInDifferentLockedLoopers_SetNextHandler_ReturnsLooper1);
	CPPUNIT_TEST(SameUnlockedLooper_SetNextHandler_ReturnsLooper);
	CPPUNIT_TEST(SameLockedLooper_SetNextHandler_ReturnsHandler2);
	CPPUNIT_TEST_SUITE_END();

	BHandler fHandler;

public:
	void Default_NextHandler_ReturnsNull()
	{
		BHandler Handler;
		CPPUNIT_ASSERT(Handler.NextHandler() == NULL);
	}

	void HandlerInLooper_NextHandler_ReturnsParentLooper()
	{
		BHandler Handler;
		BLooper Looper;
		Looper.AddHandler(&Handler);
		CPPUNIT_ASSERT(Handler.NextHandler() == &Looper);
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Clear the next handler
		@param	handler	Valid BHandler pointer, then NULL
		@results		NextHandler() returns Handler2, then NULL
	*/
	void Clear_SetNextHandler_ReturnsNull()
	{
		BLooper Looper;
		BHandler Handler1;
		BHandler Handler2;

		Looper.AddHandler(&Handler1);
		Looper.AddHandler(&Handler2);

		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == &Handler2);

		Handler1.SetNextHandler(NULL);
		CPPUNIT_ASSERT(!Handler1.NextHandler());
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Handler1 and Handler2 do not belong to a BLooper
		@param	handler	Valid BHandler pointer
		@results		NextHandler() returns NULL
						debug message "handler must belong to looper before setting
						NextHandler"
	 */
	void NotBelongingToLooper_SetNextHandler_ReturnsNull()
	{
		DEBUGGER_ESCAPE;

		BHandler Handler1;
		BHandler Handler2;
		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == NULL);
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Handler1 belongs to a unlocked BLooper, Handler2 does not
		@param	handler	Valid BHandler pointer
		@results		NextHandler() returns BLooper
						debug message "The handler's looper must be locked before
						setting NextHandler"
	 */
	void UnlockedLooper_SetNextHandler_ReturnsLooper()
	{
		DEBUGGER_ESCAPE;

		BHandler Handler1;
		BHandler Handler2;
		BLooper Looper;
		Looper.AddHandler(&Handler1);
		Looper.Unlock();
		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper);
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Handler1 belongs to a locked BLooper, Handler2 does not
		@param	handler	Valid BHandler pointer
		@results		NextHandler() returns BLooper
						debug message "The handler and its NextHandler must have
						the same looper"
	 */
	void LockedLooperMismatch_SetNextHandler_ReturnsLooper()
	{
		DEBUGGER_ESCAPE;

		BHandler Handler1;
		BHandler Handler2;
		BLooper Looper;
		Looper.AddHandler(&Handler1);
		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper);
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Handler2 belongs to a unlocked BLooper, Handler1 does not
		@param	handler	Valid BHandler pointer
		@results		NextHandler() returns NULL
						debug message "handler must belong to looper before setting
						NextHandler"
	 */
	void OnlyTargetInUnlockedLooper_SetNextHandler_ReturnsNull()
	{
		DEBUGGER_ESCAPE;

		BHandler Handler1;
		BHandler Handler2;
		BLooper Looper;
		Looper.AddHandler(&Handler2);
		Looper.Unlock();
		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == NULL);
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Handler2 belongs to a locked BLooper, Handler1 does not
		@param	handler	Valid BHandler pointer
		@results		NextHandler() returns NULL
						debug message "handler must belong to looper before setting
						NextHandler"
	 */
	void OnlyTargetInLockedLooper_SetNextHandler_ReturnsNull()
	{
		DEBUGGER_ESCAPE;

		BHandler Handler1;
		BHandler Handler2;
		BLooper Looper;
		Looper.AddHandler(&Handler2);
		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == NULL);
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Handler1 and Handler2 belong to different unlocked BLoopers
		@param	handler	Valid BHandler pointer
		@results		Returns BLooper;
						debug message "The handler and its NextHandler must have the
						same looper"
	 */
	void BothInDifferentUnlockedLoopers_SetNextHandler_ReturnsLooper1()
	{
		DEBUGGER_ESCAPE;

		BHandler Handler1;
		BHandler Handler2;
		BLooper Looper1;
		BLooper Looper2;
		Looper1.AddHandler(&Handler1);
		Looper2.AddHandler(&Handler2);
		Looper1.Unlock();
		Looper2.Unlock();
		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper1);
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Handler1 and Handler2 belong to different BLoopers;
						Handler1's is locked; Handler2's is not
		@param	handler	Valid BHandler pointer
		@result			Returns BLooper;
						debug message "The handler and its NextHandler must have the
						same looper"
	 */
	void LockedUnlockedMismatch_SetNextHandler_ReturnsLooper1()
	{
		DEBUGGER_ESCAPE;

		BHandler Handler1;
		BHandler Handler2;
		BLooper Looper1;
		BLooper Looper2;
		Looper1.AddHandler(&Handler1);
		Looper2.AddHandler(&Handler2);
		Looper2.Unlock();
		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper1);
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Handler1 and Handler2 belong to different BLoopers;
						Handler1's is unlocked; Handler2's is locked
		@param	handler	Valid BHandler pointer
		@results		Returns BLooper
						debug message "The handler and its NextHandler must have the
						same looper"
	 */
	void UnlockedLockedMismatch_SetNextHandler_ReturnsLooper1()
	{
		DEBUGGER_ESCAPE;

		BHandler Handler1;
		BHandler Handler2;
		BLooper Looper1;
		BLooper Looper2;
		Looper1.AddHandler(&Handler1);
		Looper2.AddHandler(&Handler2);
		Looper1.Unlock();
		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper1);
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Handler1 and Handler2 belong to different locked BLoopers
		@param	handler	Valid BHandler pointer
		@results		Returns BLooper
						debug message "The handler and its NextHandler must have the
						same looper"
	 */
	void BothInDifferentLockedLoopers_SetNextHandler_ReturnsLooper1()
	{
		DEBUGGER_ESCAPE;

		BHandler Handler1;
		BHandler Handler2;
		BLooper Looper1;
		BLooper Looper2;
		Looper1.AddHandler(&Handler1);
		Looper2.AddHandler(&Handler2);
		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper1);
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Handler1 and Handler2 belong to the same unlocked BLooper
		@param	handler	Valid BHandler pointer
		@results		Returns Handler2
						debug message "The handler's looper must be locked before
						setting NextHandler"
		@note			R5 implementation allows the next handler to be set anyway;
						we do the same.
	 */
	void SameUnlockedLooper_SetNextHandler_ReturnsLooper()
	{
		DEBUGGER_ESCAPE;

		BLooper Looper;
		BHandler Handler1;
		BHandler Handler2;
		Looper.AddHandler(&Handler1);
		Looper.AddHandler(&Handler2);
		Looper.Unlock();
		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == &Looper);
	}

	/**
		SetNextHandler(BHandler* handler);
		NextHandler();
		@case			Handler1 and Handler2 belong to the same locked BLooper
		@param	handler	Valid BHandler pointer
		@results		Returns Handler2
	 */
	void SameLockedLooper_SetNextHandler_ReturnsHandler2()
	{
		BLooper Looper;
		BHandler Handler1;
		BHandler Handler2;
		Looper.AddHandler(&Handler1);
		Looper.AddHandler(&Handler2);
		Handler1.SetNextHandler(&Handler2);
		CPPUNIT_ASSERT(Handler1.NextHandler() == &Handler2);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(HandlerNextHandlerTest, getTestSuiteName());
