/*
 * Copyright 2014-2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <DateTime.h>
#include <FormattingConventions.h>
#include <Language.h>
#include <TimeFormat.h>
#include <TimeZone.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


namespace BPrivate {


std::ostream&
operator<<(std::ostream& stream, const BTime& time)
{
	stream << (int)time.Hour();
	stream << ':';
	stream << (int)time.Minute();
	stream << ':';
	stream << (int)time.Second();

	return stream;
}


} // namespace BPrivate


class TimeFormatTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(TimeFormatTest);
	CPPUNIT_TEST(EnUSLocale_FormatTimeT_ReturnsCorrectFormat);
	CPPUNIT_TEST(FrFRLocale_FormatTimeT_ReturnsCorrectFormat);
	CPPUNIT_TEST(FrFRLocale_FormatTimeT_ReturnsCorrectFormat2);
	CPPUNIT_TEST(EnUSLocale0325_ParseWithShortTimeFormat_ReturnsCorrectTime);
	CPPUNIT_TEST(EnUSLocale1618_ParseWithShortTimeFormat_ReturnsCorrectTime);
	CPPUNIT_TEST(EnUSLocale2359_ParseWithShortTimeFormat_ReturnsCorrectTime);
	CPPUNIT_TEST_SUITE_END();

	void _TestFormat(const char* lang, const char* conv, const char* tz, time_t time,
					 const char* shortTime, const char* longTime)
	{
		BLanguage language(lang);
		BFormattingConventions formatting(conv);
		BTimeZone timeZone(tz);
		BTimeFormat timeFormat(language, formatting);
		BString output;

		CPPUNIT_ASSERT_EQUAL(B_OK, timeFormat.Format(output, time, B_SHORT_TIME_FORMAT, &timeZone));
		CPPUNIT_ASSERT_EQUAL(BString(shortTime), output);

		CPPUNIT_ASSERT_EQUAL(B_OK,
							 timeFormat.Format(output, time, B_MEDIUM_TIME_FORMAT, &timeZone));
		CPPUNIT_ASSERT_EQUAL(BString(longTime), output);
	}

	void _TestParseTime(const char* input, BTime expected)
	{
		BLanguage language("fr");
		BFormattingConventions formatting("fr_FR");
		BTimeFormat format(language, formatting);
		BTime time;
		CPPUNIT_ASSERT_EQUAL(B_OK, format.Parse(input, B_SHORT_TIME_FORMAT, time));
		CPPUNIT_ASSERT_EQUAL(expected, time);
	}

public:
	void EnUSLocale_FormatTimeT_ReturnsCorrectFormat()
	{
		_TestFormat("en", "en_US", "GMT+1", 12345,
					"4:25\xe2\x80\xaf"
					"AM",
					"4:25:45\xe2\x80\xaf"
					"AM");
	}

	void FrFRLocale_FormatTimeT_ReturnsCorrectFormat()
	{
		_TestFormat("fr", "fr_FR", "GMT+1", 12345, "04:25", "04:25:45");
	}

	void FrFRLocale_FormatTimeT_ReturnsCorrectFormat2()
	{
		_TestFormat("fr", "fr_FR", "GMT+1", 12345678, "22:21", "22:21:18");
	}

	void EnUSLocale0325_ParseWithShortTimeFormat_ReturnsCorrectTime()
	{
		_TestParseTime("03:25", BTime(3, 25, 0));
	}

	void EnUSLocale1618_ParseWithShortTimeFormat_ReturnsCorrectTime()
	{
		_TestParseTime("16:18", BTime(16, 18, 0));
	}

	void EnUSLocale2359_ParseWithShortTimeFormat_ReturnsCorrectTime()
	{
		_TestParseTime("23:59", BTime(23, 59, 0));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(TimeFormatTest, getTestSuiteName());
