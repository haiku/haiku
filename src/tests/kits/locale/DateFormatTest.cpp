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
DateFormatTest::TestCustomFormat()
{
	int32 fields = B_DATE_ELEMENT_HOUR | B_DATE_ELEMENT_MINUTE;
	BDateTimeFormat format;
	BString buffer;
	format.SetDateTimeFormat(B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT, fields);
	status_t result = format.Format(buffer, 12345, B_SHORT_DATE_FORMAT,
		B_SHORT_TIME_FORMAT);

	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(BString("04:25"), buffer);
}


void
DateFormatTest::TestFormat()
{
	struct Value {
		const char* language;
		const char* convention;
		time_t time;
		const char* shortDate;
		const char* longDate;
		const char* shortTime;
		const char* longTime;
		const char* shortDateTime;
	};

	static const Value values[] = {
		{"en", "en_US", 12345, "1/1/70", "January 1, 1970",
			"4:25 AM", "4:25:45 AM", "1/1/70, 4:25 AM"},
		{"fr", "fr_FR", 12345, "01/01/1970", "1 janvier 1970",
			"04:25", "04:25:45", "01/01/1970 04:25"},
		{"fr", "fr_FR", 12345678, "23/05/1970", "23 mai 1970",
			"22:21", "22:21:18", "23/05/1970 22:21"},
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
		BDateTimeFormat dateTimeFormat(&language, &formatting);

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

		result = dateTimeFormat.Format(output, values[i].time,
			B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].shortDateTime), output);
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


void
DateFormatTest::TestMonthNames()
{
	BLanguage language("en");
	BFormattingConventions formatting("en_US");
	BDateFormat format(&language, &formatting);

	BString buffer;
	status_t result = format.GetMonthName(1, buffer);

	CPPUNIT_ASSERT_EQUAL(BString("January"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetMonthName(12, buffer);

	CPPUNIT_ASSERT_EQUAL(BString("December"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);
}

std::ostream& operator<<(std::ostream& stream, const BDate& date)
{
	 stream << date.Year();
	 stream << '-';
	 stream << date.Month();
	 stream << '-';
	 stream << date.Day();

	 return stream;
}


void
DateFormatTest::TestParseDate()
{
	BLanguage language("en");
	BFormattingConventions formatting("en_US");
	BDateFormat format(&language, &formatting);
	BDate date;
	status_t result;

	struct Test {
		const char* input;
		BDate output;
	};

	static const Test tests[] = {
		{"01/01/1970", BDate(1970, 1, 1)},
		{"05/07/1988", BDate(1988, 5, 7)},
		{"07/31/2345", BDate(2345, 7, 31)},
		{NULL}
	};

	for (int i = 0; tests[i].input != NULL; i++) {
		NextSubTest();
		result = format.Parse(tests[i].input, B_SHORT_DATE_FORMAT, date);

		CPPUNIT_ASSERT_EQUAL(tests[i].output, date);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
	}
}


/*static*/ void
DateFormatTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("DateFormatTest");

	suite.addTest(new CppUnit::TestCaller<DateFormatTest>(
		"DateFormatTest::TestCustomFormat", &DateFormatTest::TestCustomFormat));
	suite.addTest(new CppUnit::TestCaller<DateFormatTest>(
		"DateFormatTest::TestFormat", &DateFormatTest::TestFormat));
	suite.addTest(new CppUnit::TestCaller<DateFormatTest>(
		"DateFormatTest::TestFormatDate", &DateFormatTest::TestFormatDate));
	suite.addTest(new CppUnit::TestCaller<DateFormatTest>(
		"DateFormatTest::TestMonthNames", &DateFormatTest::TestMonthNames));
	suite.addTest(new CppUnit::TestCaller<DateFormatTest>(
		"DateFormatTest::TestParseDate", &DateFormatTest::TestParseDate));

	parent.addTest("DateFormatTest", &suite);
}
