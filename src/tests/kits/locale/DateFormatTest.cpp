/*
 * Copyright 2014-2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "DateFormatTest.h"

#include <DateFormat.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <TimeFormat.h>
#include <TimeZone.h>

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
	struct Test {
		const char* language;
		const char* formatting;
		const char* timeZone;
		int32 fields;

		BString expected;
		BString force24;
		BString force12;
	};

	BString buffer;

	const Test tests[] = {
		{ "en", "en_US", "GMT+1", B_DATE_ELEMENT_HOUR | B_DATE_ELEMENT_MINUTE,
			"10:21 PM", "22:21", "10:21 PM" },
		{ "en", "en_US", "GMT+1",
			B_DATE_ELEMENT_HOUR | B_DATE_ELEMENT_MINUTE | B_DATE_ELEMENT_SECOND,
			"10:21:18 PM", "22:21:18", "10:21:18 PM" },
		{ "en", "en_US", "GMT+1", B_DATE_ELEMENT_HOUR | B_DATE_ELEMENT_MINUTE
			| B_DATE_ELEMENT_TIMEZONE,
			"10:21 PM GMT+1", "22:21 GMT+1", "10:21 PM GMT+1" },
		{ "en", "en_US", "GMT+1", B_DATE_ELEMENT_HOUR | B_DATE_ELEMENT_MINUTE
			| B_DATE_ELEMENT_SECOND | B_DATE_ELEMENT_TIMEZONE,
			"10:21:18 PM GMT+1", "22:21:18 GMT+1", "10:21:18 PM GMT+1" },
		{ "fr", "fr_FR", "GMT+1", B_DATE_ELEMENT_HOUR | B_DATE_ELEMENT_MINUTE,
			"22:21", "22:21", "10:21 PM" },
		{ NULL }
	};

	for (int i = 0; tests[i].language != NULL; i++)
	{
		NextSubTest();

		BLanguage language(tests[i].language);
		BFormattingConventions formatting(tests[i].formatting);
		BTimeZone timeZone(tests[i].timeZone);
		status_t result;

		// Test default for language/formatting
		{
			BDateTimeFormat format(language, formatting);
			format.SetDateTimeFormat(B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT,
				tests[i].fields);

			result = format.Format(buffer, 12345678, B_SHORT_DATE_FORMAT,
				B_SHORT_TIME_FORMAT, &timeZone);

			CPPUNIT_ASSERT_EQUAL(B_OK, result);
			CPPUNIT_ASSERT_EQUAL(tests[i].expected, buffer);
		}

		// Test forced 24 hours
		{
			formatting.SetExplicitUse24HourClock(true);
			BDateTimeFormat format(language, formatting);
			format.SetDateTimeFormat(B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT,
				tests[i].fields);
			result = format.Format(buffer, 12345678, B_SHORT_DATE_FORMAT,
				B_SHORT_TIME_FORMAT, &timeZone);

			CPPUNIT_ASSERT_EQUAL(B_OK, result);
			CPPUNIT_ASSERT_EQUAL(tests[i].force24, buffer);
		}

		// Test forced 12 hours
		{
			formatting.SetExplicitUse24HourClock(false);
			BDateTimeFormat format(language, formatting);
			format.SetDateTimeFormat(B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT,
				tests[i].fields);
			result = format.Format(buffer, 12345678, B_SHORT_DATE_FORMAT,
				B_SHORT_TIME_FORMAT, &timeZone);

			CPPUNIT_ASSERT_EQUAL(B_OK, result);
			CPPUNIT_ASSERT_EQUAL(tests[i].force12, buffer);
		}
	}
}


void
DateFormatTest::TestFormat()
{
	struct Value {
		const char* language;
		const char* convention;
		const char* timeZone;
		time_t time;
		const char* shortDate;
		const char* longDate;
		const char* shortTime;
		const char* longTime;
		const char* shortDateTime;
	};

	static const Value values[] = {
		{"en", "en_US", "GMT+1", 12345, "1/1/70", "January 1, 1970",
			"4:25 AM", "4:25:45 AM", "1/1/70, 4:25 AM"},
		{"fr", "fr_FR", "GMT+1", 12345, "01/01/1970", "1 janvier 1970",
			"04:25", "04:25:45", "01/01/1970 04:25"},
		{"fr", "fr_FR", "GMT+1", 12345678, "23/05/1970", "23 mai 1970",
			"22:21", "22:21:18", "23/05/1970 22:21"},
		{NULL}
	};

	BString output;
	status_t result;

	for (int i = 0; values[i].language != NULL; i++) {
		NextSubTest();

		BLanguage language(values[i].language);
		BFormattingConventions formatting(values[i].convention);
		BTimeZone timeZone(values[i].timeZone);
		BDateFormat dateFormat(language, formatting);
		BTimeFormat timeFormat(language, formatting);
		BDateTimeFormat dateTimeFormat(language, formatting);

		result = dateFormat.Format(output, values[i].time, B_SHORT_DATE_FORMAT,
			&timeZone);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].shortDate), output);

		result = dateFormat.Format(output, values[i].time, B_LONG_DATE_FORMAT,
			&timeZone);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].longDate), output);

		result = timeFormat.Format(output, values[i].time, B_SHORT_TIME_FORMAT,
			&timeZone);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].shortTime), output);

		result = timeFormat.Format(output, values[i].time, B_MEDIUM_TIME_FORMAT,
			&timeZone);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].longTime), output);

		result = dateTimeFormat.Format(output, values[i].time,
			B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT, &timeZone);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].shortDateTime), output);
	}
}


void
DateFormatTest::TestFormatDate()
{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

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
	BDateFormat format(language, formatting);

	BString buffer;
	status_t result = format.GetMonthName(1, buffer);

	CPPUNIT_ASSERT_EQUAL(BString("January"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetMonthName(12, buffer);

	CPPUNIT_ASSERT_EQUAL(BString("December"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetMonthName(1, buffer, B_FULL_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("January"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetMonthName(12, buffer, B_FULL_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("December"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetMonthName(1, buffer, B_LONG_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("Jan"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetMonthName(12, buffer, B_LONG_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("Dec"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetMonthName(1, buffer, B_MEDIUM_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("Jan"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetMonthName(12, buffer, B_MEDIUM_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("Dec"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetMonthName(1, buffer, B_SHORT_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("J"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetMonthName(12, buffer, B_SHORT_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("D"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

}


void
DateFormatTest::TestDayNames()
{
	BLanguage language("en");
	BFormattingConventions formatting("en_US");
	BDateFormat format(language, formatting);

	BString buffer;
	status_t result = format.GetDayName(1, buffer);

	CPPUNIT_ASSERT_EQUAL(BString("Monday"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetDayName(2, buffer);

	CPPUNIT_ASSERT_EQUAL(BString("Tuesday"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetDayName(1, buffer, B_FULL_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("Monday"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetDayName(2, buffer, B_FULL_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("Tuesday"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetDayName(1, buffer, B_LONG_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("Mon"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetDayName(2, buffer, B_LONG_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("Tue"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetDayName(1, buffer, B_MEDIUM_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("Mo"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetDayName(2, buffer, B_MEDIUM_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("Tu"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetDayName(1, buffer, B_SHORT_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("M"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);

	buffer.Truncate(0);
	result = format.GetDayName(2, buffer, B_SHORT_DATE_FORMAT);

	CPPUNIT_ASSERT_EQUAL(BString("T"), buffer);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);
}


namespace BPrivate {


std::ostream& operator<<(std::ostream& stream, const BDate& date)
{
	stream << date.Year();
	stream << '-';
	stream << date.Month();
	stream << '-';
	stream << date.Day();

	return stream;
}


std::ostream& operator<<(std::ostream& stream, const BTime& date)
{
	stream << date.Hour();
	stream << ':';
	stream << date.Minute();
	stream << ':';
	stream << date.Second();

	return stream;
}


} // namespace BPrivate


void
DateFormatTest::TestParseDate()
{
	BLanguage language("en");
	BFormattingConventions formatting("en_US");
	BDateFormat format(language, formatting);
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


void
DateFormatTest::TestParseTime()
{
	BLanguage language("fr");
	BFormattingConventions formatting("fr_FR");
	BTimeFormat format(language, formatting);
	BTime date;
	status_t result;

	struct Test {
		const char* input;
		BTime output;
	};

	static const Test tests[] = {
		{"03:25", BTime(3, 25, 0)},
		{"16:18", BTime(16, 18, 0)},
		{"23:59", BTime(23, 59, 0)},
		{NULL}
	};

	for (int i = 0; tests[i].input != NULL; i++) {
		NextSubTest();
		result = format.Parse(tests[i].input, B_SHORT_TIME_FORMAT, date);

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
		"DateFormatTest::TestDayNames", &DateFormatTest::TestDayNames));
	suite.addTest(new CppUnit::TestCaller<DateFormatTest>(
		"DateFormatTest::TestParseDate", &DateFormatTest::TestParseDate));
	suite.addTest(new CppUnit::TestCaller<DateFormatTest>(
		"DateFormatTest::TestParseTime", &DateFormatTest::TestParseTime));

	parent.addTest("DateFormatTest", &suite);
}
