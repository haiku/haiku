
/*
 * Copyright 2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <cstring>
#include <unistd.h>

#include <StopWatch.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>


class StopWatchTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StopWatchTest);
	CPPUNIT_TEST(NameWithNull_ReturnsEmptyString);
	CPPUNIT_TEST(NameWithValidString_ReturnsSetName);
	CPPUNIT_TEST(ElapsedTimeAfterDelay_Increases);
	CPPUNIT_TEST(ElapsedTimeWhenSuspended_DoesNotChange);
	CPPUNIT_TEST(LapWhenRunning_ReturnsIncreasingTime);
	CPPUNIT_TEST(LapWhenExceedsMax_StillReturnsValidTime);
	CPPUNIT_TEST(LapWhenSuspended_ReturnsZero);
	CPPUNIT_TEST(ResetAfterRunning_ClearsElapsedTime);
	CPPUNIT_TEST(ElapsedTimeAfterMultipleSuspendResume_OnlyCountsActivePeriods);
	CPPUNIT_TEST_SUITE_END();

public:
	void NameWithNull_ReturnsEmptyString()
	{
		BStopWatch sw(NULL, true);

		const char* name = sw.Name();

		CPPUNIT_ASSERT(strcmp(name, "") == 0);
	}

	void NameWithValidString_ReturnsSetName()
	{
		BStopWatch sw("mywatch", true);

		const char* name = sw.Name();

		CPPUNIT_ASSERT(strcmp(name, "mywatch") == 0);
	}

	void ElapsedTimeAfterDelay_Increases()
	{
		BStopWatch sw("et", true);
		bigtime_t t1 = sw.ElapsedTime();
		CPPUNIT_ASSERT(t1 >= 0);

		usleep(10000);

		bigtime_t t2 = sw.ElapsedTime();
		CPPUNIT_ASSERT(t2 > 0);
		CPPUNIT_ASSERT(t2 > t1);
	}

	void ElapsedTimeWhenSuspended_DoesNotChange()
	{
		BStopWatch sw("sr", true);
		usleep(5000);

		sw.Suspend();
		bigtime_t t1 = sw.ElapsedTime();
		usleep(10000);
		bigtime_t t2 = sw.ElapsedTime();

		CPPUNIT_ASSERT_EQUAL(t1, t2);

		sw.Resume();
		usleep(5000);
		bigtime_t t3 = sw.ElapsedTime();

		CPPUNIT_ASSERT(t3 > t2);
	}

	void LapWhenRunning_ReturnsIncreasingTime()
	{
		BStopWatch sw("lap", true);

		usleep(2000);
		bigtime_t l1 = sw.Lap();
		usleep(2000);
		bigtime_t l2 = sw.Lap();

		CPPUNIT_ASSERT(l2 > l1);
	}

	void LapWhenExceedsMax_StillReturnsValidTime()
	{
		BStopWatch sw("lapoverflow", true);

		for (int i = 0; i < 12; i++)
			sw.Lap();

		CPPUNIT_ASSERT(sw.Lap() > 0);
	}

	void LapWhenSuspended_ReturnsZero()
	{
		BStopWatch sw("lap2", true);
		sw.Suspend();

		bigtime_t lapTime = sw.Lap();

		CPPUNIT_ASSERT_EQUAL((bigtime_t)0, lapTime);
	}

	void ResetAfterRunning_ClearsElapsedTime()
	{
		BStopWatch sw("reset", true);
		usleep(50000);
		sw.Lap();
		bigtime_t beforeReset = sw.ElapsedTime();
		CPPUNIT_ASSERT(beforeReset >= 50000);

		sw.Reset();

		CPPUNIT_ASSERT(sw.ElapsedTime() < 5000);
	}

	void ElapsedTimeAfterMultipleSuspendResume_OnlyCountsActivePeriods()
	{
		BStopWatch sw("multi", true);

		usleep(2000);
		sw.Suspend();
		usleep(2000);
		sw.Resume();
		usleep(2000);
		sw.Suspend();
		usleep(2000);
		sw.Resume();
		usleep(2000);
		sw.Suspend();

		bigtime_t elapsed = sw.ElapsedTime();
		CPPUNIT_ASSERT(elapsed >= 6000);
		CPPUNIT_ASSERT(elapsed < 7000);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StopWatchTest, getTestSuiteName());
