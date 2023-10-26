/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "StringUtilsTest.h"

#include <String.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "StringUtils.h"


StringUtilsTest::StringUtilsTest()
{
}


StringUtilsTest::~StringUtilsTest()
{
}


void
StringUtilsTest::TestStartInSituTrimSpaceAndControl()
{
	BString string = "\t\n Tonic Water";

// ----------------------
	StringUtils::InSituTrimSpaceAndControl(string);
// ----------------------

	const BString expected = "Tonic Water";
	CPPUNIT_ASSERT_EQUAL(expected, string);
}


void
StringUtilsTest::TestEndInSituTrimSpaceAndControl()
{
	BString string = "Tonic Water  \x05\t";

// ----------------------
	StringUtils::InSituTrimSpaceAndControl(string);
// ----------------------

	const BString expected = "Tonic Water";
	CPPUNIT_ASSERT_EQUAL(expected, string);
}


void
StringUtilsTest::TestStartAndEndInSituTrimSpaceAndControl()
{
	BString string = "\x01Tonic Water\x02";

// ----------------------
	StringUtils::InSituTrimSpaceAndControl(string);
// ----------------------

	const BString expected = "Tonic Water";
	CPPUNIT_ASSERT_EQUAL(expected, string);
}


void
StringUtilsTest::TestNoTrimInSituTrimSpaceAndControl()
{
	BString string = "Tonic Water";

// ----------------------
	StringUtils::InSituTrimSpaceAndControl(string);
// ----------------------

	const BString expected = "Tonic Water";
	CPPUNIT_ASSERT_EQUAL(expected, string);
}


void
StringUtilsTest::TestInSituStripSpaceAndControl()
{
	BString string = "\x01 To\tnic Wa\nter  ";

// ----------------------
	StringUtils::InSituTrimSpaceAndControl(string);
// ----------------------

	const BString expected = "TonicWater";
		// note intervening space also removed
	CPPUNIT_ASSERT_EQUAL(expected, string);
}


void
StringUtilsTest::TestNoInSituStripSpaceAndControl()
{
	BString string = "Tonic Water";

// ----------------------
	StringUtils::InSituStripSpaceAndControl(string);
// ----------------------

	const BString expected = "Tonic Water";
	CPPUNIT_ASSERT_EQUAL(expected, string);
}


/*static*/ void
StringUtilsTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("StringUtilsTest");

	suite.addTest(
		new CppUnit::TestCaller<StringUtilsTest>(
			"StringUtilsTest::TestStartInSituTrimSpaceAndControl",
			&StringUtilsTest::TestStartInSituTrimSpaceAndControl));
	suite.addTest(
		new CppUnit::TestCaller<StringUtilsTest>(
			"StringUtilsTest::TestEndInSituTrimSpaceAndControl",
			&StringUtilsTest::TestEndInSituTrimSpaceAndControl));
	suite.addTest(
		new CppUnit::TestCaller<StringUtilsTest>(
			"StringUtilsTest::TestStartAndEndInSituTrimSpaceAndControl",
			&StringUtilsTest::TestStartAndEndInSituTrimSpaceAndControl));
	suite.addTest(
		new CppUnit::TestCaller<StringUtilsTest>(
			"StringUtilsTest::TestNoTrimInSituTrimSpaceAndControl",
			&StringUtilsTest::TestNoTrimInSituTrimSpaceAndControl));

	suite.addTest(
		new CppUnit::TestCaller<StringUtilsTest>(
			"StringUtilsTest::TestNoInSituStripSpaceAndControl",
			&StringUtilsTest::TestNoInSituStripSpaceAndControl));
	suite.addTest(
		new CppUnit::TestCaller<StringUtilsTest>(
			"StringUtilsTest::TestInSituStripSpaceAndControl",
			&StringUtilsTest::TestInSituStripSpaceAndControl));

	parent.addTest("StringUtilsTest", &suite);
}