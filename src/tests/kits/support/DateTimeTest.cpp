/*
 * Copyright 2014, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "DateTimeTest.h"

#include <DateTime.h>

#include <cppunit/TestSuite.h>


class DateTimeTest : public BTestCase {
	public:
		DateTimeTest(std::string name = "");

		void test(void);
};


DateTimeTest::DateTimeTest(std::string name)
	:
	BTestCase(name)
{
}


void
DateTimeTest::test()
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


CppUnit::Test*
DateTimeTestSuite()
{
	CppUnit::TestSuite* testSuite = new CppUnit::TestSuite();

	testSuite->addTest(new DateTimeTest("BDateTime"));

	return testSuite;
}
