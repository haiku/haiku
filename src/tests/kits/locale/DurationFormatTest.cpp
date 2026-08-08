/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <DurationFormat.h>
#include <Locale.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class DurationFormatTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DurationFormatTest);
	CPPUNIT_TEST(DefaultLocale_Format_ReturnsNotEmpty);
	CPPUNIT_TEST(EnUSLocale_Format_ReturnsFullFormat);
	CPPUNIT_TEST(FrFRLocale_Format_ReturnsFullFormat);
	CPPUNIT_TEST(EnUSLocale_FormatAbbreviated_ReturnsAbbreviatedFormat);
	CPPUNIT_TEST(FrFRLocale_FormatAbbreviated_ReturnsAbbreviatedFormat);
	CPPUNIT_TEST(EnUSLocale_FormatHour_ReturnsOnlyHour);
	CPPUNIT_TEST(FrFRLocaleAndNonEmptyString_FormatHour_AppendsHour);
	CPPUNIT_TEST_SUITE_END();

public:
	void DefaultLocale_Format_ReturnsNotEmpty()
	{
		BDurationFormat format;
		BString buffer;

		CPPUNIT_ASSERT_EQUAL(B_OK, format.Format(buffer, 0, 800000000000ll));
		// The exact format and language used depends on the locale settings, but
		// we can assume that whatever they are, it should put something in the
		// string.
		CPPUNIT_ASSERT(buffer.Length() > 0);
	}

	void EnUSLocale_Format_ReturnsFullFormat()
	{
		BFormattingConventions englishFormat("en_US");
		BLanguage englishLanguage("en");
		BDurationFormat format(englishLanguage, englishFormat);
		BString buffer;

		CPPUNIT_ASSERT_EQUAL(B_OK, format.Format(buffer, 0, 800000000000ll));
		CPPUNIT_ASSERT_EQUAL(BString("1 week, 2 days, 6 hours, 13 minutes, 20 seconds"), buffer);
	}

	void FrFRLocale_Format_ReturnsFullFormat()
	{
		BFormattingConventions frenchFormat("fr_FR");
		BLanguage frenchLanguage("fr");
		BDurationFormat format(frenchLanguage, frenchFormat);
		BString buffer;

		CPPUNIT_ASSERT_EQUAL(B_OK, format.Format(buffer, 0, 800000000000ll));
		CPPUNIT_ASSERT_EQUAL(BString("1\xc2\xa0semaine, 2\xc2\xa0jours, 6\xc2\xa0heures, "
									 "13 minutes, 20\xc2\xa0secondes"),
							 buffer);
	}

	void EnUSLocale_FormatAbbreviated_ReturnsAbbreviatedFormat()
	{
		BFormattingConventions englishFormat("en_US");
		BLanguage englishLanguage("en");
		BDurationFormat format(englishLanguage, englishFormat, ":", B_TIME_UNIT_ABBREVIATED);
		BString buffer;

		CPPUNIT_ASSERT_EQUAL(B_OK, format.Format(buffer, 0, 800000000000ll));
		CPPUNIT_ASSERT_EQUAL(BString("1 wk:2 days:6 hr:13 min:20 sec"), buffer);
	}

	void FrFRLocale_FormatAbbreviated_ReturnsAbbreviatedFormat()
	{
		BFormattingConventions frenchFormat("fr_FR");
		BLanguage frenchLanguage("fr");
		BDurationFormat format(frenchLanguage, frenchFormat, ":", B_TIME_UNIT_ABBREVIATED);
		BString buffer;

		CPPUNIT_ASSERT_EQUAL(B_OK, format.Format(buffer, 0, 800000000000ll));
		CPPUNIT_ASSERT_EQUAL(BString("1\xe2\x80\xafsem.:2\xe2\x80\xafj:6\xe2\x80\xafh:"
									 "13\xc2\xa0min:20\xe2\x80\xafs"),
							 buffer);
	}

	void EnUSLocale_FormatHour_ReturnsOnlyHour()
	{
		BFormattingConventions englishFormat("en_US");
		BLanguage englishLanguage("en");
		BTimeUnitFormat format(englishLanguage, englishFormat);
		BString buffer;

		CPPUNIT_ASSERT_EQUAL(B_OK, format.Format(buffer, 5, B_TIME_UNIT_HOUR));
		CPPUNIT_ASSERT_EQUAL(BString("5 hours"), buffer);
	}

	void FrFRLocaleAndNonEmptyString_FormatHour_AppendsHour()
	{
		BString buffer = "5 hours";

		BFormattingConventions frenchFormat("fr_FR");
		BLanguage frenchLanguage("fr");
		BTimeUnitFormat frFormat(frenchLanguage, frenchFormat);
		CPPUNIT_ASSERT_EQUAL(B_OK, frFormat.Format(buffer, 5, B_TIME_UNIT_HOUR));

		// We check that the passed BString is not truncated. This makes it easy
		// to append several units to the same string, as BDurationFormat does.
		CPPUNIT_ASSERT_EQUAL(BString("5 hours5\xc2\xa0heures"), buffer);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DurationFormatTest, getTestSuiteName());
