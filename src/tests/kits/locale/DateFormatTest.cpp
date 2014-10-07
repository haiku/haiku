/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "DateFormatTest.h"

#include <DateFormat.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <TimeFormat.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


DateFormatTest::DateFormatTest()
{
}


DateFormatTest::~DateFormatTest()
{
}


void
DateFormatTest::TestFormat()
{
	struct Value {
		char* language;
		char* convention;
		time_t time;
		char* shortDate;
		char* longDate;
		char* shortTime;
		char* longTime;
	};

	static const Value values[] = {
		{"en", "en_US", 12345, "1/1/70", "January 1, 1970",
			"4:25 AM", "4:25:45 AM"},
		{"fr", "fr_FR", 12345, "01/01/1970", "1 janvier 1970",
			"04:25", "04:25:45"},
		{"fr", "fr_FR", 12345678, "23/05/1970", "23 mai 1970",
			"22:21", "22:21:18"},
		{NULL}
	};

	BString output;
	status_t result;

	for (int i = 0; values[i].language != NULL; i++) {
		NextSubTest();

		BLanguage language(values[i].language);
		BFormattingConventions formatting(values[i].convention);
		BDateFormat dateFormat(&language, &formatting);
		BTimeFormat timeFormat(&language, &formatting);

		result = dateFormat.Format(output, values[i].time, B_SHORT_DATE_FORMAT);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].shortDate), output);

		result = dateFormat.Format(output, values[i].time, B_LONG_DATE_FORMAT);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].longDate), output);

		result = timeFormat.Format(output, values[i].time, B_SHORT_TIME_FORMAT);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].shortTime), output);

		result = timeFormat.Format(output, values[i].time, B_MEDIUM_TIME_FORMAT);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].longTime), output);
	}
}


void
DateFormatTest::TestFormatDate()
{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(&language, &formatting);

		BString output;
		status_t result;

		BDate date(2014, 9, 29);

		result = format.Format(output, date, B_LONG_DATE_FORMAT);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString("September 29, 2014"), output);

		// Test an invalid date - must return B_BAD_DATA
		date.SetDate(2014, 29, 29);
		result = format.Format(output, date, B_LONG_DATE_FORMAT);
		CPPUNIT_ASSERT_EQUAL(B_BAD_DATA, result);
}


/*static*/ void
DateFormatTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("DateFormatTest");

	suite.addTest(new CppUnit::TestCaller<DateFormatTest>(
		"DateFormatTest::TestFormat", &DateFormatTest::TestFormat));
	suite.addTest(new CppUnit::TestCaller<DateFormatTest>(
		"DateFormatTest::TestFormatDate", &DateFormatTest::TestFormatDate));

	parent.addTest("DateFormatTest", &suite);
}
