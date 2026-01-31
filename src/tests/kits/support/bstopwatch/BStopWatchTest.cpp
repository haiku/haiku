/*
 * Copyright 2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include "BStopWatchTest.h"

#include <cstring>
#include <unistd.h>

#include <StopWatch.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


class BStopWatchTest : public BTestCase {
public:
	BStopWatchTest(std::string name = "");

	void NameWithNull_ReturnsEmptyString();
	void NameWithValidString_ReturnsSetName();
	void ElapsedTimeAfterDelay_Increases();
	void ElapsedTimeWhenSuspended_DoesNotChange();
	void LapWhenRunning_ReturnsIncreasingTime();
	void LapWhenExceedsMax_StillReturnsValidTime();
	void LapWhenSuspended_ReturnsZero();
	void ResetAfterRunning_ClearsElapsedTime();
	void ElapsedTimeAfterMultipleSuspendResume_OnlyCountsActivePeriods();
};


BStopWatchTest::BStopWatchTest(std::string name)
	:BTestCase(name)
{
}


void
BStopWatchTest::NameWithNull_ReturnsEmptyString()
{
	BStopWatch sw(NULL, true);

	const char* name = sw.Name();

	CPPUNIT_ASSERT(strcmp(name, "") == 0);
}


void
BStopWatchTest::NameWithValidString_ReturnsSetName()
{
	BStopWatch sw("mywatch", true);

	const char* name = sw.Name();

	CPPUNIT_ASSERT(strcmp(name, "mywatch") == 0);
}


void
BStopWatchTest::ElapsedTimeAfterDelay_Increases()
{
	BStopWatch sw("et", true);
	bigtime_t t1 = sw.ElapsedTime();
	CPPUNIT_ASSERT(t1 >= 0);

	usleep(10000);

	bigtime_t t2 = sw.ElapsedTime();
	CPPUNIT_ASSERT(t2 > 0);
	CPPUNIT_ASSERT(t2 > t1);
}


void
BStopWatchTest::ElapsedTimeWhenSuspended_DoesNotChange()
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


void
BStopWatchTest::LapWhenRunning_ReturnsIncreasingTime()
{
	BStopWatch sw("lap", true);

	usleep(2000);
	bigtime_t l1 = sw.Lap();
	usleep(2000);
	bigtime_t l2 = sw.Lap();

	CPPUNIT_ASSERT(l2 > l1);
}


void
BStopWatchTest::LapWhenExceedsMax_StillReturnsValidTime()
{
	BStopWatch sw("lapoverflow", true);

	for (int i = 0; i < 12; i++)
		sw.Lap();

	CPPUNIT_ASSERT(sw.Lap() > 0);
}


void
BStopWatchTest::LapWhenSuspended_ReturnsZero()
{
	BStopWatch sw("lap2", true);
	sw.Suspend();

	bigtime_t lapTime = sw.Lap();

	CPPUNIT_ASSERT_EQUAL((bigtime_t)0, lapTime);
}


void
BStopWatchTest::ResetAfterRunning_ClearsElapsedTime()
{
	BStopWatch sw("reset", true);
	usleep(50000);
	sw.Lap();
	bigtime_t beforeReset = sw.ElapsedTime();
	CPPUNIT_ASSERT(beforeReset >= 50000);

	sw.Reset();

	CPPUNIT_ASSERT(sw.ElapsedTime() < 5000);
}


void
BStopWatchTest::ElapsedTimeAfterMultipleSuspendResume_OnlyCountsActivePeriods()
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


CppUnit::Test*
BStopWatchTestSuite()
{
	CppUnit::TestSuite* suite = new CppUnit::TestSuite("BStopWatch");

	suite->addTest(new CppUnit::TestCaller<BStopWatchTest>(
		"BStopWatchTest::NameWithNull_ReturnsEmptyString",
		&BStopWatchTest::NameWithNull_ReturnsEmptyString));
	suite->addTest(new CppUnit::TestCaller<BStopWatchTest>(
		"BStopWatchTest::NameWithValidString_ReturnsSetName",
		&BStopWatchTest::NameWithValidString_ReturnsSetName));
	suite->addTest(new CppUnit::TestCaller<BStopWatchTest>(
		"BStopWatchTest::ElapsedTimeAfterDelay_Increases",
		&BStopWatchTest::ElapsedTimeAfterDelay_Increases));
	suite->addTest(new CppUnit::TestCaller<BStopWatchTest>(
		"BStopWatchTest::ElapsedTimeWhenSuspended_DoesNotChange",
		&BStopWatchTest::ElapsedTimeWhenSuspended_DoesNotChange));
	suite->addTest(new CppUnit::TestCaller<BStopWatchTest>(
		"BStopWatchTest::LapWhenRunning_ReturnsIncreasingTime",
		&BStopWatchTest::LapWhenRunning_ReturnsIncreasingTime));
	suite->addTest(new CppUnit::TestCaller<BStopWatchTest>(
		"BStopWatchTest::LapWhenExceedsMax_StillReturnsValidTime",
		&BStopWatchTest::LapWhenExceedsMax_StillReturnsValidTime));
	suite->addTest(new CppUnit::TestCaller<BStopWatchTest>(
		"BStopWatchTest::LapWhenSuspended_ReturnsZero",
		&BStopWatchTest::LapWhenSuspended_ReturnsZero));
	suite->addTest(new CppUnit::TestCaller<BStopWatchTest>(
		"BStopWatchTest::ResetAfterRunning_ClearsElapsedTime",
		&BStopWatchTest::ResetAfterRunning_ClearsElapsedTime));
	suite->addTest(new CppUnit::TestCaller<BStopWatchTest>(
		"BStopWatchTest::ElapsedTimeAfterMultipleSuspendResume_OnlyCountsActivePeriods",
		&BStopWatchTest::ElapsedTimeAfterMultipleSuspendResume_OnlyCountsActivePeriods));

	return suite;
}
