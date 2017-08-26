/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Akshay Agarwal <agarwal.akshay.akshay8@gmail.com>
 */

#include "RelativeDateTimeFormatTest.h"

#include <time.h>

#include <Locale.h>
#include <RelativeDateTimeFormat.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


const int kMonthsPerYear = 12;
const int kMaxDaysPerMonth = 31;
const int kDaysPerWeek = 7;
const int kHoursPerDay = 24;
const int kMinutesPerHour = 60;
const int kSecondsPerMinute = 60;


RelativeDateTimeFormatTest::RelativeDateTimeFormatTest()
{
}


RelativeDateTimeFormatTest::~RelativeDateTimeFormatTest()
{
}


void
RelativeDateTimeFormatTest::TestDefault()
{
	BRelativeDateTimeFormat format;
	BString buffer;
	BString expected;

	status_t result = format.Format(buffer, time(NULL));
	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	// The exact format and language used depends on the locale settings, but
	// we can assume that whatever they are, it should put something in the
	// string.
	CPPUNIT_ASSERT(buffer.Length() > 0);
}


void
RelativeDateTimeFormatTest::TestFormat()
{
	struct Value {
		const char* language;
		const char* convention;
		time_t timeDelta;
		const char* relativeDate;
	};

	static const Value values[] = {
		{"en", "en_US", 0, "in 0 seconds"},
		{"en", "en_US", 20, "in 20 seconds"},
		{"en", "en_US", -20, "20 seconds ago"},
		{"en", "en_US", 5 * kSecondsPerMinute, "in 5 minutes"},
		{"en", "en_US", -5 * kSecondsPerMinute, "5 minutes ago"},
		{"en", "en_US", 2 * kMinutesPerHour * kSecondsPerMinute, "in 2 hours"},
		{"en", "en_US", -2 * kMinutesPerHour * kSecondsPerMinute, "2 hours ago"},
		{"fr", "fr_FR", 2 * kMinutesPerHour * kSecondsPerMinute, "dans 2 heures"},
		{"fr", "fr_FR", -2 * kMinutesPerHour * kSecondsPerMinute, "il y a 2 heures"},
		{"en", "en_US", 5 * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
			"in 5 days"},
		{"en", "en_US", -5 * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
			"5 days ago"},
		{"de", "de_DE", 5 * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
			"in 5 Tagen"},
		{"de", "de_DE", -5 * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
			"vor 5 Tagen"},
		{"en", "en_US", 3 * kDaysPerWeek * kHoursPerDay * kMinutesPerHour
			* kSecondsPerMinute, "in 3 weeks"},
		{"en", "en_US", -3 * kDaysPerWeek * kHoursPerDay * kMinutesPerHour
			* kSecondsPerMinute, "3 weeks ago"},
		{"en", "en_US", 4 * kMaxDaysPerMonth * kHoursPerDay * kMinutesPerHour
			* kSecondsPerMinute, "in 4 months"},
		{"en", "en_US", -4 * kMaxDaysPerMonth * kHoursPerDay * kMinutesPerHour
			* kSecondsPerMinute, "4 months ago"},
		{"en", "en_US", 2 * kMonthsPerYear * kMaxDaysPerMonth * kHoursPerDay
			* kMinutesPerHour * kSecondsPerMinute, "in 2 years"},
		{"en", "en_US", -2 * kMonthsPerYear * kMaxDaysPerMonth * kHoursPerDay
			* kMinutesPerHour * kSecondsPerMinute, "2 years ago"},
		{NULL}
	};

	status_t result;

	for (int i = 0; values[i].language != NULL; i++) {
		NextSubTest();

		BString output;
		BLanguage language(values[i].language);
		BFormattingConventions formatting(values[i].convention);
		BRelativeDateTimeFormat format(language, formatting);

		result = format.Format(output, time(NULL) + values[i].timeDelta);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(values[i].relativeDate), output);
	}
}


/*static*/ void
RelativeDateTimeFormatTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("RelativeDateTimeFormatTest");

	suite.addTest(new CppUnit::TestCaller<RelativeDateTimeFormatTest>(
		"RelativeDateTimeFormatTest::TestDefault", &RelativeDateTimeFormatTest::TestDefault));
	suite.addTest(new CppUnit::TestCaller<RelativeDateTimeFormatTest>(
		"RelativeDateTimeFormatTest::TestFormat", &RelativeDateTimeFormatTest::TestFormat));
	parent.addTest("RelativeDateTimeFormatTest", &suite);
}
