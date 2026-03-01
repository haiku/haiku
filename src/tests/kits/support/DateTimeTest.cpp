/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */

#include <DateTime.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>


class DateTimeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DateTimeTest);
	CPPUNIT_TEST(SetToMinusOne_IsValidAndReturnsCorrectProperties);
	CPPUNIT_TEST_SUITE_END();

public:
	void SetToMinusOne_IsValidAndReturnsCorrectProperties()
	{
		BDateTime dateTime;

		// Should be just one second before epoch
		dateTime.SetTime_t(-1);

		CPPUNIT_ASSERT(dateTime.IsValid());
		CPPUNIT_ASSERT_EQUAL(59, dateTime.Time().Second());
		CPPUNIT_ASSERT_EQUAL(59, dateTime.Time().Minute());
		CPPUNIT_ASSERT_EQUAL(23, dateTime.Time().Hour());
		CPPUNIT_ASSERT_EQUAL(31, dateTime.Date().Day());
		CPPUNIT_ASSERT_EQUAL(12, dateTime.Date().Month());
		CPPUNIT_ASSERT_EQUAL(1969, dateTime.Date().Year());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DateTimeTest, getTestSuiteName());
