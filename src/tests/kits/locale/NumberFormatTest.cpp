/*
 * Copyright 2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#include "NumberFormatTest.h"

#include <NumberFormat.h>
#include <Locale.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


NumberFormatTest::NumberFormatTest()
{
}


NumberFormatTest::~NumberFormatTest()
{
}


void
NumberFormatTest::TestPercentTurkish()
{
	_TestGeneralPercent("tr", "%2");
}


void
NumberFormatTest::TestPercentEnglish()
{
	_TestGeneralPercent("en_US", "2%");
}


void
NumberFormatTest::TestPercentGerman()
{
	_TestGeneralPercent("de", "2\xc2\xa0%");
		// 2<non-breaking-space>%
}


void
NumberFormatTest::_TestGeneralPercent(const char* languageCode,
	const char* expected)
{
	// GIVEN
	BLanguage turkishLanguage(languageCode);
	BFormattingConventions formattingConventions(languageCode);
	BLocale turkishLocale(&turkishLanguage, &formattingConventions);
	BNumberFormat numberFormat(&turkishLocale);
	BString output;
	double input = 0.025;

	// WHEN
	status_t result = numberFormat.FormatPercent(output, input);

	// THEN
	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(BString(expected), output);
}


/*static*/ void
NumberFormatTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("NumberFormatTest");

	suite.addTest(new CppUnit::TestCaller<NumberFormatTest>(
		"NumberFormatTest::TestPercentTurkish",
		&NumberFormatTest::TestPercentTurkish));
	suite.addTest(new CppUnit::TestCaller<NumberFormatTest>(
		"NumberFormatTest::TestPercentEnglish",
		&NumberFormatTest::TestPercentEnglish));
	suite.addTest(new CppUnit::TestCaller<NumberFormatTest>(
		"NumberFormatTest::TestPercentGerman",
		&NumberFormatTest::TestPercentGerman));

	parent.addTest("NumberFormatTest", &suite);
}
