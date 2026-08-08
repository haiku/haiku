/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Akshay Agarwal <agarwal.akshay.akshay8@gmail.com>
 */


#include <time.h>

#include <Locale.h>
#include <RelativeDateTimeFormat.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


const int kMonthsPerYear = 12;
const int kMaxDaysPerMonth = 31;
const int kDaysPerWeek = 7;
const int kHoursPerDay = 24;
const int kMinutesPerHour = 60;
const int kSecondsPerMinute = 60;


class RelativeDateTimeFormatTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(RelativeDateTimeFormatTest);
	CPPUNIT_TEST(DefaultLocale_Format_ReturnsNotEmpty);
	CPPUNIT_TEST(EnUSLocale_Format_ReturnsIn0s);
	CPPUNIT_TEST(EnUSLocale_Format_ReturnsIn20s);
	CPPUNIT_TEST(EnUSLocale_Format_Returns20sAgo);
	CPPUNIT_TEST(EnUSLocale_Format_ReturnsIn5m);
	CPPUNIT_TEST(EnUSLocale_Format_Returns5mAgo);
	CPPUNIT_TEST(EnUSLocale_Format_ReturnsIn2h);
	CPPUNIT_TEST(EnUSLocale_Format_Returns2hAgo);
	CPPUNIT_TEST(FrFRLocale_Format_ReturnsIn2h);
	CPPUNIT_TEST(FrFRLocale_Format_Returns2hAgo);
	CPPUNIT_TEST(EnUSLocale_Format_ReturnsIn5d);
	CPPUNIT_TEST(EnUSLocale_Format_Returns5dAgo);
	CPPUNIT_TEST(DeDELocale_Format_ReturnsIn5d);
	CPPUNIT_TEST(DeDELocale_Format_Returns5dAgo);
	CPPUNIT_TEST(EnUSLocale_Format_ReturnsIn3w);
	CPPUNIT_TEST(EnUSLocale_Format_Returns3wAgo);
	CPPUNIT_TEST(EnUSLocale_Format_ReturnsIn4mo);
	CPPUNIT_TEST(EnUSLocale_Format_Returns4moAgo);
	CPPUNIT_TEST(EnUSLocale_Format_ReturnsIn2y);
	CPPUNIT_TEST(EnUSLocale_Format_Returns2yAgo);
	CPPUNIT_TEST_SUITE_END();

	void _TestFormat(const char* lang, const char* conv, time_t timeDelta, const char* expected)
	{
		BLanguage language(lang);
		BFormattingConventions formatting(conv);
		BRelativeDateTimeFormat format(language, formatting);
		BString output;

		CPPUNIT_ASSERT_EQUAL(B_OK, format.Format(output, time(NULL) + timeDelta));
		CPPUNIT_ASSERT_EQUAL(BString(expected), output);
	}

public:
	void DefaultLocale_Format_ReturnsNotEmpty()
	{
		BRelativeDateTimeFormat format;
		BString buffer;

		CPPUNIT_ASSERT_EQUAL(B_OK, format.Format(buffer, time(NULL)));
		// The exact format and language used depends on the locale settings, but
		// we can assume that whatever they are, it should put something in the
		// string.
		CPPUNIT_ASSERT(buffer.Length() > 0);
	}

	void EnUSLocale_Format_ReturnsIn0s() { _TestFormat("en", "en_US", 0, "in 0 seconds"); }
	void EnUSLocale_Format_ReturnsIn20s() { _TestFormat("en", "en_US", 20, "in 20 seconds"); }
	void EnUSLocale_Format_Returns20sAgo() { _TestFormat("en", "en_US", -20, "20 seconds ago"); }
	void EnUSLocale_Format_ReturnsIn5m()
	{
		_TestFormat("en", "en_US", 5 * kSecondsPerMinute, "in 5 minutes");
	}

	void EnUSLocale_Format_Returns5mAgo()
	{
		_TestFormat("en", "en_US", -5 * kSecondsPerMinute, "5 minutes ago");
	}

	void EnUSLocale_Format_ReturnsIn2h()
	{
		_TestFormat("en", "en_US", 2 * kMinutesPerHour * kSecondsPerMinute, "in 2 hours");
	}

	void EnUSLocale_Format_Returns2hAgo()
	{
		_TestFormat("en", "en_US", -2 * kMinutesPerHour * kSecondsPerMinute, "2 hours ago");
	}

	void FrFRLocale_Format_ReturnsIn2h()
	{
		_TestFormat("fr", "fr_FR", 2 * kMinutesPerHour * kSecondsPerMinute, "dans 2 heures");
	}

	void FrFRLocale_Format_Returns2hAgo()
	{
		_TestFormat("fr", "fr_FR", -2 * kMinutesPerHour * kSecondsPerMinute, "il y a 2 heures");
	}

	void EnUSLocale_Format_ReturnsIn5d()
	{
		_TestFormat("en", "en_US", 5 * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
					"in 5 days");
	}

	void EnUSLocale_Format_Returns5dAgo()
	{
		_TestFormat("en", "en_US", -5 * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
					"5 days ago");
	}

	void DeDELocale_Format_ReturnsIn5d()
	{
		_TestFormat("de", "de_DE", 5 * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
					"in 5 Tagen");
	}

	void DeDELocale_Format_Returns5dAgo()
	{
		_TestFormat("de", "de_DE", -5 * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
					"vor 5 Tagen");
	}

	void EnUSLocale_Format_ReturnsIn3w()
	{
		_TestFormat("en", "en_US",
					3 * kDaysPerWeek * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
					"in 3 weeks");
	}

	void EnUSLocale_Format_Returns3wAgo()
	{
		_TestFormat("en", "en_US",
					-3 * kDaysPerWeek * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
					"3 weeks ago");
	}

	void EnUSLocale_Format_ReturnsIn4mo()
	{
		_TestFormat("en", "en_US",
					4 * kMaxDaysPerMonth * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
					"in 4 months");
	}

	void EnUSLocale_Format_Returns4moAgo()
	{
		_TestFormat("en", "en_US",
					-4 * kMaxDaysPerMonth * kHoursPerDay * kMinutesPerHour * kSecondsPerMinute,
					"4 months ago");
	}

	void EnUSLocale_Format_ReturnsIn2y()
	{
		_TestFormat("en", "en_US",
					2 * kMonthsPerYear * kMaxDaysPerMonth * kHoursPerDay * kMinutesPerHour
						* kSecondsPerMinute,
					"in 2 years");
	}

	void EnUSLocale_Format_Returns2yAgo()
	{
		_TestFormat("en", "en_US",
					-2 * kMonthsPerYear * kMaxDaysPerMonth * kHoursPerDay * kMinutesPerHour
						* kSecondsPerMinute,
					"2 years ago");
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(RelativeDateTimeFormatTest, getTestSuiteName());
