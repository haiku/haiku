/*
 * Copyright 2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <Locale.h>
#include <NumberFormat.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class NumberFormatTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(NumberFormatTest);
	CPPUNIT_TEST(TurkishLocale_FormatPercent_ReturnsExpected);
	CPPUNIT_TEST(EnglishLocale_FormatPercent_ReturnsExpected);
	CPPUNIT_TEST(GermanLocale_FormatPercent_ReturnsExpected);
	CPPUNIT_TEST_SUITE_END();

	void _TestGeneralPercent(const char* languageCode, const char* expected)
	{
		BLanguage language(languageCode);
		BFormattingConventions formattingConventions(languageCode);
		BLocale locale(&language, &formattingConventions);
		BNumberFormat numberFormat(&locale);
		BString output;
		double input = 0.025;

		CPPUNIT_ASSERT_EQUAL(B_OK, numberFormat.FormatPercent(output, input));
		CPPUNIT_ASSERT_EQUAL(BString(expected), output);
	}

public:
	void TurkishLocale_FormatPercent_ReturnsExpected() { _TestGeneralPercent("tr", "%2"); }

	void EnglishLocale_FormatPercent_ReturnsExpected() { _TestGeneralPercent("en_US", "2%"); }

	void GermanLocale_FormatPercent_ReturnsExpected()
	{
		_TestGeneralPercent("de", "2\xc2\xa0%");
		// 2<non-breaking-space>%
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(NumberFormatTest, getTestSuiteName());
