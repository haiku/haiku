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
DurationFormatTest::TestDuration()
{
	BDurationFormat format;
	BString buffer;
	BString expected;

	BFormattingConventions englishFormat("en_US");
	BLanguage englishLanguage("en");

	BFormattingConventions frenchFormat("fr_FR");
	BLanguage frenchLanguage("fr");

	format.SetFormattingConventions(englishFormat);
	format.SetLanguage(englishLanguage);
	status_t result = format.Format(0, 800000000000ll, &buffer);

	expected << "1 week, 2 days, 6 hours, 13 minutes, 20 seconds";
	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(expected, buffer);

	format.SetFormattingConventions(frenchFormat);
	format.SetLanguage(frenchLanguage);
	result = format.Format(0, 800000000000ll, &buffer);

	// We check that the passed BString is not truncated.
	expected << "1 semaine, 2 jours, 6 heures, 13 minutes, 20 secondes";
	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(expected, buffer);
}


void
DurationFormatTest::TestTimeUnit()
{
	BTimeUnitFormat format;
	BString buffer;

	BFormattingConventions englishFormat("en_US");
	BLanguage englishLanguage("en");

	BFormattingConventions frenchFormat("fr_FR");
	BLanguage frenchLanguage("fr");

	format.SetFormattingConventions(englishFormat);
	format.SetLanguage(englishLanguage);
	status_t result = format.Format(5, B_TIME_UNIT_HOUR, &buffer);

	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	CPPUNIT_ASSERT_EQUAL(BString("5 hours"), buffer);

	format.SetFormattingConventions(frenchFormat);
	format.SetLanguage(frenchLanguage);
	result = format.Format(5, B_TIME_UNIT_HOUR, &buffer);

	CPPUNIT_ASSERT_EQUAL(B_OK, result);
	// We check that the passed BString is not truncated. This makes it easy
	// to append several units to the same string, as BDurationFormat does.
	CPPUNIT_ASSERT_EQUAL(BString("5 hours5 heures"), buffer);
}


/*static*/ void
DurationFormatTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("DurationFormatTest");

	suite.addTest(new CppUnit::TestCaller<DurationFormatTest>(
		"DurationFormatTest::TestDuration", &DurationFormatTest::TestDuration));
	suite.addTest(new CppUnit::TestCaller<DurationFormatTest>(
		"DurationFormatTest::TestTimeUnit", &DurationFormatTest::TestTimeUnit));

	parent.addTest("DurationFormatTest", &suite);
}
