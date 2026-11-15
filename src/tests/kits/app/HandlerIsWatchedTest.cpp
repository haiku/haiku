/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <Handler.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class HandlerIsWatchedTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(HandlerIsWatchedTest);
	CPPUNIT_TEST(NoWatchers_IsWatched_ReturnsFalse);
	CPPUNIT_TEST(AddThenRemove_IsWatched_ReturnsFalse);
	CPPUNIT_TEST(AddNotifyRemove_IsWatched_ReturnsFalse);
	CPPUNIT_TEST(RemoveNonexistent_IsWatched_ReturnsFalse);
	CPPUNIT_TEST(SendNoticeWithoutWatchers_IsWatched_ReturnsFalse);
	CPPUNIT_TEST_SUITE_END();

	BHandler fHandler;

public:
	/**
		IsWatched()
		@case		No added watchers
		@results	Returns false
	*/
	void NoWatchers_IsWatched_ReturnsFalse()
	{
		CPPUNIT_ASSERT(!fHandler.IsWatched());
	}

	/**
		IsWatched()
		@case		Add then remove watcher
		@results	Returns true after add, returns false after remove
		@note		Original implementation fails this test.  Either the removal
					doesn't happen (unlikely) or some list-within-a-list doesn't
					get removed when there's nothing in it anymore.
	*/
	void AddThenRemove_IsWatched_ReturnsFalse()
	{
		BHandler Watcher;
		fHandler.StartWatching(&Watcher, '1234');
		CPPUNIT_ASSERT(fHandler.IsWatched() == true);

		fHandler.StopWatching(&Watcher, '1234');
	#ifndef TEST_R5
		CPPUNIT_ASSERT(fHandler.IsWatched() == false);
	#endif
	}

	/**
		IsWatched()
		@case		Add watcher, send notice, then remove watcher
		@results	Returns true after add, returns false after remove
	*/
	void AddNotifyRemove_IsWatched_ReturnsFalse()
	{
		BHandler Watcher;
		fHandler.StartWatching(&Watcher, '1234');
		CPPUNIT_ASSERT(fHandler.IsWatched() == true);

		fHandler.SendNotices('1234');

		fHandler.StopWatching(&Watcher, '1234');
		CPPUNIT_ASSERT(fHandler.IsWatched() == false);
	}

	/**
		IsWatched()
		@case		Remove inexistent watcher
		@results	Returns false
	*/
	void RemoveNonexistent_IsWatched_ReturnsFalse()
	{
		BHandler Watcher;

		fHandler.StopWatching(&Watcher, '1234');
		CPPUNIT_ASSERT(fHandler.IsWatched() == false);
	}

	/**
		IsWatched()
		@case		Send notices without watchers
		@results	Returns false
	*/
	void SendNoticeWithoutWatchers_IsWatched_ReturnsFalse()
	{
		BHandler Watcher;

		// Create handler's internal list
		fHandler.StartWatching(&Watcher, '1234');
		fHandler.StopWatching(&Watcher, '1234');

		fHandler.SendNotices('1234');
		CPPUNIT_ASSERT(fHandler.IsWatched() == false);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(HandlerIsWatchedTest, getTestSuiteName());
