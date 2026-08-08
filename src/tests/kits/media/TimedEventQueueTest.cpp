/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <TimedEventQueue.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class TimedEventQueueTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(TimedEventQueueTest);
	CPPUNIT_TEST(NewQueue_EventCount_ReturnsZero);
	CPPUNIT_TEST(NewQueue_HasEvents_ReturnsFalse);
	CPPUNIT_TEST(MultipleEvents_RemoveEvent_QueueStaysConsistent);
	CPPUNIT_TEST(MultipleEvents_DoForEach_QueueStaysConsistent);
	CPPUNIT_TEST(MultipleEvents_FindFirstMatch_QueueStaysConsistent);
	CPPUNIT_TEST(MultipleEvents_FlushEvents_QueueStaysConsistent);
	CPPUNIT_TEST_SUITE_END();

	media_timed_event fDoForEachEvent;
	int fDoForEachCount;

	static BTimedEventQueue::queue_action DoForEachHook(media_timed_event* event, void* context)
	{
		TimedEventQueueTest* test = (TimedEventQueueTest*)context;
		test->fDoForEachEvent = *event;
		test->fDoForEachCount++;
		return BTimedEventQueue::B_NO_ACTION;
	}

	BTimedEventQueue* fQueue;

public:
	void setUp() { fQueue = new BTimedEventQueue; }

	void tearDown() { delete fQueue; }

	void NewQueue_EventCount_ReturnsZero() { CPPUNIT_ASSERT_EQUAL(0, fQueue->EventCount()); }

	void NewQueue_HasEvents_ReturnsFalse() { CPPUNIT_ASSERT_EQUAL(false, fQueue->HasEvents()); }

	void MultipleEvents_FlushEvents_QueueStaysConsistent()
	{
		fQueue->AddEvent(media_timed_event(0x1000, BTimedEventQueue::B_SEEK));
		fQueue->AddEvent(media_timed_event(0x1001, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1002, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1003, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1004, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1005, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1006, BTimedEventQueue::B_STOP));
		fQueue->AddEvent(media_timed_event(0x1007, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1008, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1009, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1010, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1011, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1012, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1013, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1013, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1013, BTimedEventQueue::B_SEEK));
		CPPUNIT_ASSERT_EQUAL(16, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		fQueue->FlushEvents(0x1007, BTimedEventQueue::B_AT_TIME);
		CPPUNIT_ASSERT_EQUAL(15, fQueue->EventCount());

		fQueue->FlushEvents(0x1012, BTimedEventQueue::B_AFTER_TIME, false);
		CPPUNIT_ASSERT_EQUAL(12, fQueue->EventCount());

		fQueue->FlushEvents(0x1fff, BTimedEventQueue::B_AFTER_TIME, false);
		CPPUNIT_ASSERT_EQUAL(12, fQueue->EventCount());

		fQueue->FlushEvents(0x1fff, BTimedEventQueue::B_AFTER_TIME, true);
		CPPUNIT_ASSERT_EQUAL(12, fQueue->EventCount());

		fQueue->FlushEvents(0x1010, BTimedEventQueue::B_AFTER_TIME, true);
		CPPUNIT_ASSERT_EQUAL(9, fQueue->EventCount());

		fQueue->FlushEvents(0x1006, BTimedEventQueue::B_BEFORE_TIME, false);
		CPPUNIT_ASSERT_EQUAL(3, fQueue->EventCount());

		fQueue->FlushEvents(0x1006, BTimedEventQueue::B_AT_TIME);
		CPPUNIT_ASSERT_EQUAL(2, fQueue->EventCount());

		fQueue->FlushEvents(0xffffff, BTimedEventQueue::B_BEFORE_TIME);
		CPPUNIT_ASSERT_EQUAL(0, fQueue->EventCount());
	}

	void MultipleEvents_FindFirstMatch_QueueStaysConsistent()
	{
		fQueue->AddEvent(media_timed_event(0x1000, BTimedEventQueue::B_SEEK));
		fQueue->AddEvent(media_timed_event(0x1001, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1002, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1003, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1010, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1011, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1012, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1004, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1005, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1006, BTimedEventQueue::B_STOP));
		fQueue->AddEvent(media_timed_event(0x1007, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1008, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1009, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1013, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1013, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1013, BTimedEventQueue::B_SEEK));
		CPPUNIT_ASSERT_EQUAL(16, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		{
			const media_timed_event* result = fQueue->FindFirstMatch(
				0x1001, BTimedEventQueue::B_AFTER_TIME, true, BTimedEventQueue::B_STOP);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1006, BTimedEventQueue::B_STOP));
		}
		{
			const media_timed_event* result
				= fQueue->FindFirstMatch(0x1006, BTimedEventQueue::B_AT_TIME, true);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1006, BTimedEventQueue::B_STOP));
		}
		{
			const media_timed_event* result = fQueue->FindFirstMatch(
				0x1007, BTimedEventQueue::B_BEFORE_TIME, true, BTimedEventQueue::B_STOP);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1006, BTimedEventQueue::B_STOP));
		}
		{
			const media_timed_event* result
				= fQueue->FindFirstMatch(0x1006, BTimedEventQueue::B_BEFORE_TIME, true);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1000, BTimedEventQueue::B_SEEK));
		}
		{
			const media_timed_event* result
				= fQueue->FindFirstMatch(0x1006, BTimedEventQueue::B_AFTER_TIME, true);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1006, BTimedEventQueue::B_STOP));
		}
		{
			const media_timed_event* result = fQueue->FindFirstMatch(
				0x1006, BTimedEventQueue::B_BEFORE_TIME, false, BTimedEventQueue::B_STOP);
			CPPUNIT_ASSERT(result == NULL);
		}
		{
			const media_timed_event* result = fQueue->FindFirstMatch(
				0x1006, BTimedEventQueue::B_AFTER_TIME, false, BTimedEventQueue::B_STOP);
			CPPUNIT_ASSERT(result == NULL);
		}
		{
			const media_timed_event* result = fQueue->FindFirstMatch(
				0x1006, BTimedEventQueue::B_AT_TIME, false, BTimedEventQueue::B_SEEK);
			CPPUNIT_ASSERT(result == NULL);
		}
		{
			const media_timed_event* result
				= fQueue->FindFirstMatch(0x1000, BTimedEventQueue::B_AFTER_TIME, true);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1000, BTimedEventQueue::B_SEEK));
		}
		{
			const media_timed_event* result
				= fQueue->FindFirstMatch(0x1010, BTimedEventQueue::B_BEFORE_TIME, true);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1000, BTimedEventQueue::B_SEEK));
		}
		{
			const media_timed_event* result
				= fQueue->FindFirstMatch(0x1007, BTimedEventQueue::B_BEFORE_TIME, false);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1000, BTimedEventQueue::B_SEEK));
		}
		{
			const media_timed_event* result = fQueue->FindFirstMatch(
				0x1001, BTimedEventQueue::B_AFTER_TIME, false, BTimedEventQueue::B_SEEK);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1013, BTimedEventQueue::B_SEEK));
		}
		{
			const media_timed_event* result
				= fQueue->FindFirstMatch(0x1009, BTimedEventQueue::B_AFTER_TIME, false);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1010, BTimedEventQueue::B_START));
		}
		{
			const media_timed_event* result
				= fQueue->FindFirstMatch(0x1010, BTimedEventQueue::B_AFTER_TIME, true);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1010, BTimedEventQueue::B_START));
		}
		{
			const media_timed_event* result
				= fQueue->FindFirstMatch(0x1010, BTimedEventQueue::B_AT_TIME, true);
			CPPUNIT_ASSERT(result != NULL);
			CPPUNIT_ASSERT(*result == media_timed_event(0x1010, BTimedEventQueue::B_START));
		}
	}

	void MultipleEvents_DoForEach_QueueStaysConsistent()
	{
		fQueue->AddEvent(media_timed_event(0x1000, BTimedEventQueue::B_SEEK));
		fQueue->AddEvent(media_timed_event(0x1001, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1002, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1003, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1010, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1011, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1012, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1004, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1005, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1006, BTimedEventQueue::B_STOP));
		fQueue->AddEvent(media_timed_event(0x1007, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1008, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1009, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1013, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1013, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1013, BTimedEventQueue::B_SEEK));
		CPPUNIT_ASSERT_EQUAL(16, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x1000, BTimedEventQueue::B_AT_TIME);
		CPPUNIT_ASSERT(fDoForEachEvent == media_timed_event(0x1000, BTimedEventQueue::B_SEEK));
		CPPUNIT_ASSERT_EQUAL(1, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x1006, BTimedEventQueue::B_AT_TIME);
		CPPUNIT_ASSERT(fDoForEachEvent == media_timed_event(0x1006, BTimedEventQueue::B_STOP));
		CPPUNIT_ASSERT_EQUAL(1, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x1013, BTimedEventQueue::B_AT_TIME);
		CPPUNIT_ASSERT_EQUAL(3, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x1003, BTimedEventQueue::B_BEFORE_TIME,
						  false);
		CPPUNIT_ASSERT_EQUAL(3, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x1003, BTimedEventQueue::B_BEFORE_TIME,
						  true);
		CPPUNIT_ASSERT_EQUAL(4, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x1012, BTimedEventQueue::B_AFTER_TIME,
						  false);
		CPPUNIT_ASSERT_EQUAL(3, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x1012, BTimedEventQueue::B_AFTER_TIME, true);
		CPPUNIT_ASSERT_EQUAL(4, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x1013, BTimedEventQueue::B_AFTER_TIME,
						  false);
		CPPUNIT_ASSERT_EQUAL(0, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x1013, BTimedEventQueue::B_AFTER_TIME, true);
		CPPUNIT_ASSERT_EQUAL(3, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x0, BTimedEventQueue::B_ALWAYS);
		CPPUNIT_ASSERT_EQUAL(16, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x0, BTimedEventQueue::B_ALWAYS, false,
						  BTimedEventQueue::B_WARP);
		CPPUNIT_ASSERT_EQUAL(0, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x0, BTimedEventQueue::B_ALWAYS, false,
						  BTimedEventQueue::B_SEEK);
		CPPUNIT_ASSERT_EQUAL(2, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x0999, BTimedEventQueue::B_AFTER_TIME, false,
						  BTimedEventQueue::B_SEEK);
		CPPUNIT_ASSERT_EQUAL(2, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x1014, BTimedEventQueue::B_BEFORE_TIME,
						  false, BTimedEventQueue::B_SEEK);
		CPPUNIT_ASSERT_EQUAL(2, fDoForEachCount);

		fDoForEachCount = 0;
		fQueue->DoForEach(DoForEachHook, (void*)this, 0x0004, BTimedEventQueue::B_BEFORE_TIME,
						  true);
		CPPUNIT_ASSERT_EQUAL(0, fDoForEachCount);
	}

	void MultipleEvents_RemoveEvent_QueueStaysConsistent()
	{
		fQueue->AddEvent(media_timed_event(0x1007, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1005, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x9999, BTimedEventQueue::B_STOP));
		fQueue->AddEvent(media_timed_event(0x1006, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1002, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1011, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1000, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x0777, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1001, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1000, BTimedEventQueue::B_STOP));
		fQueue->AddEvent(media_timed_event(0x1003, BTimedEventQueue::B_START));
		fQueue->AddEvent(media_timed_event(0x1000, BTimedEventQueue::B_SEEK));
		CPPUNIT_ASSERT_EQUAL(12, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e1(0x1003, BTimedEventQueue::B_START);
		fQueue->RemoveEvent(&e1);
		CPPUNIT_ASSERT_EQUAL(11, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e2(0x1007, BTimedEventQueue::B_START);
		fQueue->RemoveEvent(&e2);
		CPPUNIT_ASSERT_EQUAL(10, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e3(0x1000, BTimedEventQueue::B_STOP);
		fQueue->RemoveEvent(&e3);
		CPPUNIT_ASSERT_EQUAL(9, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e4(0x1000, BTimedEventQueue::B_SEEK);
		fQueue->RemoveEvent(&e4);
		CPPUNIT_ASSERT_EQUAL(8, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		// remove non existing element (time)
		media_timed_event e5(0x1111, BTimedEventQueue::B_STOP);
		fQueue->RemoveEvent(&e5);
		CPPUNIT_ASSERT_EQUAL(8, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		// remove non existing element (type)
		media_timed_event e6(0x1011, BTimedEventQueue::B_STOP);
		fQueue->RemoveEvent(&e6);
		CPPUNIT_ASSERT_EQUAL(8, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e7(0x1000, BTimedEventQueue::B_START);
		fQueue->RemoveEvent(&e7);
		CPPUNIT_ASSERT_EQUAL(7, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e8(0x1011, BTimedEventQueue::B_START);
		fQueue->RemoveEvent(&e8);
		CPPUNIT_ASSERT_EQUAL(6, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e9(0x1002, BTimedEventQueue::B_START);
		fQueue->RemoveEvent(&e9);
		CPPUNIT_ASSERT_EQUAL(5, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e10(0x0777, BTimedEventQueue::B_START);
		fQueue->RemoveEvent(&e10);
		CPPUNIT_ASSERT_EQUAL(4, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e11(0x9999, BTimedEventQueue::B_STOP);
		fQueue->RemoveEvent(&e11);
		CPPUNIT_ASSERT_EQUAL(3, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e12(0x1006, BTimedEventQueue::B_START);
		fQueue->RemoveEvent(&e12);
		CPPUNIT_ASSERT_EQUAL(2, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e13(0x1001, BTimedEventQueue::B_START);
		fQueue->RemoveEvent(&e13);
		CPPUNIT_ASSERT_EQUAL(1, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(true, fQueue->HasEvents());

		media_timed_event e14(0x1005, BTimedEventQueue::B_START);
		fQueue->RemoveEvent(&e14);
		CPPUNIT_ASSERT_EQUAL(0, fQueue->EventCount());
		CPPUNIT_ASSERT_EQUAL(false, fQueue->HasEvents());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TimedEventQueueTest, getTestSuiteName());
