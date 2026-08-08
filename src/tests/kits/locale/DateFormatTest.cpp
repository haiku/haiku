/*
 * Copyright 2014-2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <DateFormat.h>
#include <DateTime.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <TimeZone.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


namespace BPrivate {


std::ostream&
operator<<(std::ostream& stream, const BDate& date)
{
	stream << date.Year();
	stream << '-';
	stream << (int)date.Month();
	stream << '-';
	stream << (int)date.Day();

	return stream;
}


} // namespace BPrivate


class DateFormatTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DateFormatTest);
	CPPUNIT_TEST(EnUSLocale_FormatTimeT_ReturnsCorrectFormat);
	CPPUNIT_TEST(FrFRLocale_FormatTimeT_ReturnsCorrectFormat);
	CPPUNIT_TEST(FrFRLocaleTimeTMay1970_FormatTimeT_ReturnsCorrectFormat);
	CPPUNIT_TEST(ValidDate_Format_ReturnsCorrectFormat);
	CPPUNIT_TEST(InvalidDate_Format_ReturnsBadData);
	CPPUNIT_TEST(EnUSLocale_GetMonthNameWithoutStyle_ReturnsCorrectName);
	CPPUNIT_TEST(EnUSLocale_GetMonthNameWithFullDateFormat_ReturnsCorrectName);
	CPPUNIT_TEST(EnUSLocale_GetMonthNameWithLongDateFormat_ReturnsCorrectName);
	CPPUNIT_TEST(EnUSLocale_GetMonthNameWithMediumDateFormat_ReturnsCorrectName);
	CPPUNIT_TEST(EnUSLocale_GetMonthNameWithShortDateFormat_ReturnsCorrectName);
	CPPUNIT_TEST(EnUSLocale_GetDayNameWithoutStyle_ReturnsCorrectName);
	CPPUNIT_TEST(EnUSLocale_GetDayNameWithFullDateFormat_ReturnsCorrectName);
	CPPUNIT_TEST(EnUSLocale_GetDayNameWithLongDateFormat_ReturnsCorrectName);
	CPPUNIT_TEST(EnUSLocale_GetDayNameWithMediumDateFormat_ReturnsCorrectName);
	CPPUNIT_TEST(EnUSLocale_GetDayNameWithShortDateFormat_ReturnsCorrectName);
	CPPUNIT_TEST(EnUSLocale01011970_ParseWithShortDateFormat_ReturnsCorrectDate);
	CPPUNIT_TEST(EnUSLocale05071988_ParseWithShortDateFormat_ReturnsCorrectDate);
	CPPUNIT_TEST(EnUSLocale07312345_ParseWithShortDateFormat_ReturnsCorrectDate);
	CPPUNIT_TEST_SUITE_END();

	void _TestFormatTimeT(const char* lang, const char* conv, const char* tz, time_t time,
						  const char* shortDate, const char* longDate)
	{
		BLanguage language(lang);
		BFormattingConventions formatting(conv);
		BTimeZone timeZone(tz);
		BDateFormat dateFormat(language, formatting);
		BString output;

		CPPUNIT_ASSERT_EQUAL(B_OK, dateFormat.Format(output, time, B_SHORT_DATE_FORMAT, &timeZone));
		CPPUNIT_ASSERT_EQUAL(BString(shortDate), output);

		CPPUNIT_ASSERT_EQUAL(B_OK, dateFormat.Format(output, time, B_LONG_DATE_FORMAT, &timeZone));
		CPPUNIT_ASSERT_EQUAL(BString(longDate), output);
	}

	void _TestParse(const char* input, BDate expected)
	{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);
		BDate date;
		CPPUNIT_ASSERT_EQUAL(B_OK, format.Parse(input, B_SHORT_DATE_FORMAT, date));
		CPPUNIT_ASSERT_EQUAL(expected, date);
	}

	void _AssertMonthName(const BDateFormat& format, int month, const char* expected,
						  const BDateFormatStyle style)
	{
		BString buffer;

		CPPUNIT_ASSERT_EQUAL(B_OK, format.GetMonthName(month, buffer, style));
		CPPUNIT_ASSERT_EQUAL(BString(expected), buffer);
	}

	void _AssertDayName(const BDateFormat& format, int day, const char* expected,
						const BDateFormatStyle style)
	{
		BString buffer;

		CPPUNIT_ASSERT_EQUAL(B_OK, format.GetDayName(day, buffer, style));
		CPPUNIT_ASSERT_EQUAL(BString(expected), buffer);
	}

public:
	void EnUSLocale_FormatTimeT_ReturnsCorrectFormat()
	{
		_TestFormatTimeT("en", "en_US", "GMT+1", 12345, "1/1/70", "January 1, 1970");
	}

	void FrFRLocale_FormatTimeT_ReturnsCorrectFormat()
	{
		_TestFormatTimeT("fr", "fr_FR", "GMT+1", 12345, "01/01/1970", "1 janvier 1970");
	}

	void FrFRLocaleTimeTMay1970_FormatTimeT_ReturnsCorrectFormat()
	{
		_TestFormatTimeT("fr", "fr_FR", "GMT+1", 12345678, "23/05/1970", "23 mai 1970");
	}

	void ValidDate_Format_ReturnsCorrectFormat()
	{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		BString output;
		BDate date(2014, 9, 29);

		CPPUNIT_ASSERT_EQUAL(B_OK, format.Format(output, date, B_LONG_DATE_FORMAT));
		CPPUNIT_ASSERT_EQUAL(BString("September 29, 2014"), output);
	}

	void InvalidDate_Format_ReturnsBadData()
	{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		BString output;
		BDate date(2014, 29, 29);
		CPPUNIT_ASSERT_EQUAL(B_BAD_DATA, format.Format(output, date, B_LONG_DATE_FORMAT));
	}

	void EnUSLocale_GetMonthNameWithoutStyle_ReturnsCorrectName()
	{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		BString buffer;
		CPPUNIT_ASSERT_EQUAL(B_OK, format.GetMonthName(1, buffer));
		CPPUNIT_ASSERT_EQUAL(BString("January"), buffer);

		buffer.Truncate(0);
		CPPUNIT_ASSERT_EQUAL(B_OK, format.GetMonthName(12, buffer));
		CPPUNIT_ASSERT_EQUAL(BString("December"), buffer);
	}

	void EnUSLocale_GetMonthNameWithFullDateFormat_ReturnsCorrectName()
	{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		_AssertMonthName(format, 1, "January", B_FULL_DATE_FORMAT);
		_AssertMonthName(format, 12, "December", B_FULL_DATE_FORMAT);
	}

	void EnUSLocale_GetMonthNameWithLongDateFormat_ReturnsCorrectName()
	{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		_AssertMonthName(format, 1, "Jan", B_LONG_DATE_FORMAT);
		_AssertMonthName(format, 12, "Dec", B_LONG_DATE_FORMAT);
	}

	void EnUSLocale_GetMonthNameWithMediumDateFormat_ReturnsCorrectName()
	{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		_AssertMonthName(format, 1, "Jan", B_MEDIUM_DATE_FORMAT);
		_AssertMonthName(format, 12, "Dec", B_MEDIUM_DATE_FORMAT);
	}

	void EnUSLocale_GetMonthNameWithShortDateFormat_ReturnsCorrectName()
	{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		_AssertMonthName(format, 1, "J", B_SHORT_DATE_FORMAT);
		_AssertMonthName(format, 12, "D", B_SHORT_DATE_FORMAT);
	}

	void EnUSLocale_GetDayNameWithoutStyle_ReturnsCorrectName()
	{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		BString buffer;
		CPPUNIT_ASSERT_EQUAL(B_OK, format.GetDayName(1, buffer));
		CPPUNIT_ASSERT_EQUAL(BString("Monday"), buffer);

		buffer.Truncate(0);
		CPPUNIT_ASSERT_EQUAL(B_OK, format.GetDayName(2, buffer));
		CPPUNIT_ASSERT_EQUAL(BString("Tuesday"), buffer);
	}

	void EnUSLocale_GetDayNameWithFullDateFormat_ReturnsCorrectName()
	{
		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		_AssertDayName(format, 1, "Monday", B_FULL_DATE_FORMAT);
		_AssertDayName(format, 2, "Tuesday", B_FULL_DATE_FORMAT);
	}
	void EnUSLocale_GetDayNameWithLongDateFormat_ReturnsCorrectName()
	{

		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		_AssertDayName(format, 1, "Mon", B_LONG_DATE_FORMAT);
		_AssertDayName(format, 2, "Tue", B_LONG_DATE_FORMAT);
	}
	void EnUSLocale_GetDayNameWithMediumDateFormat_ReturnsCorrectName()
	{

		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		_AssertDayName(format, 1, "Mo", B_MEDIUM_DATE_FORMAT);
		_AssertDayName(format, 2, "Tu", B_MEDIUM_DATE_FORMAT);
	}
	void EnUSLocale_GetDayNameWithShortDateFormat_ReturnsCorrectName()
	{

		BLanguage language("en");
		BFormattingConventions formatting("en_US");
		BDateFormat format(language, formatting);

		_AssertDayName(format, 1, "M", B_SHORT_DATE_FORMAT);
		_AssertDayName(format, 2, "T", B_SHORT_DATE_FORMAT);
	}

	void EnUSLocale01011970_ParseWithShortDateFormat_ReturnsCorrectDate()
	{
		_TestParse("01/01/1970", BDate(1970, 1, 1));
	}

	void EnUSLocale05071988_ParseWithShortDateFormat_ReturnsCorrectDate()
	{
		_TestParse("05/07/1988", BDate(1988, 5, 7));
	}

	void EnUSLocale07312345_ParseWithShortDateFormat_ReturnsCorrectDate()
	{
		_TestParse("07/31/2345", BDate(2345, 7, 31));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DateFormatTest, getTestSuiteName());
