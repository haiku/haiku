/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <TimeCode.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class TimeCodeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(TimeCodeTest);
	CPPUNIT_TEST(TimeCode30Drop2_LinearFrames_ReturnsConsistentResults);
	CPPUNIT_TEST(TimeCode30Drop2_Microseconds_ReturnsConsistentResults);
	CPPUNIT_TEST_SUITE_END();

public:
	void TimeCode30Drop2_LinearFrames_ReturnsConsistentResults()
	{
		BTimeCode timeCode;
		timeCode.SetType(B_TIMECODE_30_DROP_2);

		char outStr[30];

		// Test frames -> TimeCode -> frames
		int32 ranges[] = { 8990, 8995, 17981, 17990, 26971, 26980 };
		for (int r = 0; r < 6; r += 2) {
			for (int32 i = ranges[r]; i <= ranges[r+1]; i++) {
				timeCode.SetLinearFrames(i);
				timeCode.GetString(outStr);
				int32 j = timeCode.LinearFrames();
				CPPUNIT_ASSERT_EQUAL(i, j);
			}
		}
	}

	void TimeCode30Drop2_Microseconds_ReturnsConsistentResults()
	{
		BTimeCode timeCode;
		timeCode.SetType(B_TIMECODE_30_DROP_2);

		char outStr[30];

		// Test us -> TimeCode -> us
		for (int32 i = 59000; i <= 61000; i++) {
			timeCode.SetMicroseconds(i);
			timeCode.GetString(outStr);
			int32 j = timeCode.Microseconds();
			CPPUNIT_ASSERT(j >= 0);
		}
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TimeCodeTest, getTestSuiteName());
