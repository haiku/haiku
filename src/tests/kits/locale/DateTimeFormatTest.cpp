/*
 * Copyright 2014-2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <DateFormat.h>
#include <DateTime.h>
#include <DateTimeFormat.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <TimeFormat.h>
#include <TimeZone.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


namespace BPrivate {

std::ostream& operator<<(std::ostream& stream, const BDate& date);
std::ostream& operator<<(std::ostream& stream, const BTime& date);

} // namespace BPrivate


class DateTimeFormatTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DateTimeFormatTest);
	CPPUNIT_TEST(EnUSLocale_CustomFormatDayHourMinute_ReturnsCorrectDateTime);
	CPPUNIT_TEST(EnUSLocale_CustomFormatMonthYearHour_ReturnsCorrectDateTime);
	CPPUNIT_TEST(EnUSLocale_CustomFormatMonthDayHourMinuteTimeZone_ReturnsCorrectDateTime);
	CPPUNIT_TEST(FrFRLocale_CustomFormatMonthYearHour_ReturnsCorrectDateTime);
	CPPUNIT_TEST(EnUSLocale_FormatTimeT_ReturnsCorrectFormat);
	CPPUNIT_TEST(FrFRLocale_FormatTimeT_ReturnsCorrectFormat);
	CPPUNIT_TEST(FrFRLocale_FormatTimeT_ReturnsCorrectFormat2);
	CPPUNIT_TEST_SUITE_END();

	void _TestCustomFormat(const char* lang, const char* conv, const char* tz, int32 fields,
						   const char* expected, const char* force24, const char* force12)
	{
		BLanguage language(lang);
		BFormattingConventions formatting(conv);
		BTimeZone timeZone(tz);
		BString buffer;

		// Test default
		{
			BDateTimeFormat format(language, formatting);
			format.SetDateTimeFormat(B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT, fields);
			CPPUNIT_ASSERT_EQUAL(B_OK,
								 format.Format(buffer, 12345678, B_SHORT_DATE_FORMAT,
											   B_SHORT_TIME_FORMAT, &timeZone));
			CPPUNIT_ASSERT_EQUAL(BString(expected), buffer);
		}

		// Test forced 24 hours
		{
			formatting.SetExplicitUse24HourClock(true);
			BDateTimeFormat format(language, formatting);
			format.SetDateTimeFormat(B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT, fields);
			CPPUNIT_ASSERT_EQUAL(B_OK,
								 format.Format(buffer, 12345678, B_SHORT_DATE_FORMAT,
											   B_SHORT_TIME_FORMAT, &timeZone));
			CPPUNIT_ASSERT_EQUAL(BString(force24), buffer);
		}

		// Test forced 12 hours
		{
			formatting.SetExplicitUse24HourClock(false);
			BDateTimeFormat format(language, formatting);
			format.SetDateTimeFormat(B_SHORT_DATE_FORMAT, B_SHORT_TIME_FORMAT, fields);
			CPPUNIT_ASSERT_EQUAL(B_OK,
								 format.Format(buffer, 12345678, B_SHORT_DATE_FORMAT,
											   B_SHORT_TIME_FORMAT, &timeZone));
			CPPUNIT_ASSERT_EQUAL(BString(force12), buffer);
		}
	}

	void _TestFormat(const char* lang, const char* conv, const char* tz, time_t time,
					 const char* shortDateTime)
	{
		BLanguage language(lang);
		BFormattingConventions formatting(conv);
		BTimeZone timeZone(tz);
		BDateTimeFormat dateTimeFormat(language, formatting);
		BString output;

		CPPUNIT_ASSERT_EQUAL(B_OK,
							 dateTimeFormat.Format(output, time, B_SHORT_DATE_FORMAT,
												   B_SHORT_TIME_FORMAT, &timeZone));
		CPPUNIT_ASSERT_EQUAL(BString(shortDateTime), output);
	}

public:
	void EnUSLocale_CustomFormatDayHourMinute_ReturnsCorrectDateTime()
	{
		_TestCustomFormat("en", "en_US", "GMT+1",
						  B_DATE_ELEMENT_DAY | B_DATE_ELEMENT_HOUR | B_DATE_ELEMENT_MINUTE,
						  "23, 10:21\xe2\x80\xafPM", "23, 22:21", "23, 10:21\xe2\x80\xafPM");
	}

	void EnUSLocale_CustomFormatMonthYearHour_ReturnsCorrectDateTime()
	{
		_TestCustomFormat("en", "en_US", "GMT+1",
						  B_DATE_ELEMENT_YEAR | B_DATE_ELEMENT_MONTH | B_DATE_ELEMENT_HOUR,
						  "05/1970, 10\xe2\x80\xafPM", "05/1970, 22", "05/1970, 10\xe2\x80\xafPM");
	}

	void EnUSLocale_CustomFormatMonthDayHourMinuteTimeZone_ReturnsCorrectDateTime()
	{
		_TestCustomFormat("en", "en_US", "GMT+1",
						  B_DATE_ELEMENT_MONTH | B_DATE_ELEMENT_DAY | B_DATE_ELEMENT_HOUR
							  | B_DATE_ELEMENT_MINUTE | B_DATE_ELEMENT_TIMEZONE,
						  "05/23, 10:21\xe2\x80\xafPM GMT+1", "05/23, 22:21 GMT+1",
						  "05/23, 10:21\xe2\x80\xafPM GMT+1");
	}

	void FrFRLocale_CustomFormatMonthYearHour_ReturnsCorrectDateTime()
	{
		_TestCustomFormat("fr", "fr_FR", "GMT+1",
						  B_DATE_ELEMENT_YEAR | B_DATE_ELEMENT_MONTH | B_DATE_ELEMENT_HOUR
							  | B_DATE_ELEMENT_MINUTE,
						  "05/1970 22:21", "05/1970 22:21", "05/1970 10:21\xe2\x80\xafPM");
	}

	void EnUSLocale_FormatTimeT_ReturnsCorrectFormat()
	{
		_TestFormat("en", "en_US", "GMT+1", 12345,
					"1/1/70, 4:25\xe2\x80\xaf"
					"AM");
	}

	void FrFRLocale_FormatTimeT_ReturnsCorrectFormat()
	{
		_TestFormat("fr", "fr_FR", "GMT+1", 12345, "01/01/1970 04:25");
	}

	void FrFRLocale_FormatTimeT_ReturnsCorrectFormat2()
	{
		_TestFormat("fr", "fr_FR", "GMT+1", 12345678, "23/05/1970 22:21");
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DateTimeFormatTest, getTestSuiteName());
