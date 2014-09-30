/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "DateFormatTest.h"

#include <DateFormat.h>
#include <FormattingConventions.h>
#include <Language.h>

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
	};

	static const Value values[] = {
		{"en", "en_US", 12345, "1/1/70", "January 1, 1970"},
		{"fr", "fr_FR", 12345, "01/01/70", "1 janvier 1970"},
		{"fr", "fr_FR", 12345678, "23/05/70", "23 mai 1970"},
		{NULL}
	};

	BString output;
	status_t result;

	for (int i = 0; values[i].language != NULL; i++) {
		NextSubTest();

		BLanguage language(values[i].language);
		BFormattingConventions formatting(values[i].convention);
		BDateFormat format(&language, &formatting);

		result = format.Format(output, values[i].time, B_SHORT_DATE_FORMAT);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].shortDate), output);

		result = format.Format(output, values[i].time, B_LONG_DATE_FORMAT);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].longDate), output);
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
