/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "DurationFormatTest.h"

#include <DurationFormat.h>
#include <Locale.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


DurationFormatTest::DurationFormatTest()
{
}


DurationFormatTest::~DurationFormatTest()
{
}


void
DurationFormatTest::TestDefault()
{
	BDurationFormat format;
	BString buffer;
	BString expected;

	status_t result = format.Format(buffer, 0, 800000000000ll);
	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	// The exact format and language used depends on the locale settings, but
	// we can assume that whatever they are, it should put something in the
	// string.
	CPPUNIT_ASSERT(buffer.Length() > 0);
}


void
DurationFormatTest::TestDuration()
{
	BString buffer;
	BString expected;
	status_t result;

	BFormattingConventions englishFormat("en_US");
	BLanguage englishLanguage("en");

	BFormattingConventions frenchFormat("fr_FR");
	BLanguage frenchLanguage("fr");

	{
		BDurationFormat format(englishLanguage, englishFormat);
		status_t result = format.Format(buffer, 0, 800000000000ll);

		expected << "1 week, 2 days, 6 hours, 13 minutes, 20 seconds";
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(expected, buffer);
	}

	{
		BDurationFormat format(frenchLanguage, frenchFormat);
		result = format.Format(buffer, 0, 800000000000ll);

		// We check that the passed BString is not truncated.
		expected << "1 semaine, 2 jours, 6 heures, 13 minutes, 20 secondes";
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(expected, buffer);
	}
}


void
DurationFormatTest::TestTimeUnit()
{
	BString buffer;
	status_t result;

	BFormattingConventions englishFormat("en_US");
	BLanguage englishLanguage("en");

	BFormattingConventions frenchFormat("fr_FR");
	BLanguage frenchLanguage("fr");

	{
		BTimeUnitFormat format(englishLanguage, englishFormat);
		result = format.Format(buffer, 5, B_TIME_UNIT_HOUR);

		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString("5 hours"), buffer);
	}

	{
		BTimeUnitFormat format(frenchLanguage, frenchFormat);
		result = format.Format(buffer, 5, B_TIME_UNIT_HOUR);

		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		// We check that the passed BString is not truncated. This makes it easy
		// to append several units to the same string, as BDurationFormat does.
		CPPUNIT_ASSERT_EQUAL(BString("5 hours5 heures"), buffer);
	}
}


/*static*/ void
DurationFormatTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("DurationFormatTest");

	suite.addTest(new CppUnit::TestCaller<DurationFormatTest>(
		"DurationFormatTest::TestDefault", &DurationFormatTest::TestDefault));
	suite.addTest(new CppUnit::TestCaller<DurationFormatTest>(
		"DurationFormatTest::TestDuration", &DurationFormatTest::TestDuration));
	suite.addTest(new CppUnit::TestCaller<DurationFormatTest>(
		"DurationFormatTest::TestTimeUnit", &DurationFormatTest::TestTimeUnit));

	parent.addTest("DurationFormatTest", &suite);
}
