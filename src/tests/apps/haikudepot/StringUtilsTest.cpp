/*
 * Copyright 2023-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "StringUtils.h"


class StringUtilsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringUtilsTest);
	CPPUNIT_TEST(InSituTrimSpaceAndControl_Start);
	CPPUNIT_TEST(InSituTrimSpaceAndControl_End);
	CPPUNIT_TEST(InSituTrimSpaceAndControl_BothEnds);
	CPPUNIT_TEST(InSituTrimSpaceAndControl_None);
	CPPUNIT_TEST(InSituStripSpaceAndControl_Mixed);
	CPPUNIT_TEST(InSituStripSpaceAndControl_None);
	CPPUNIT_TEST_SUITE_END();

public:
	void InSituStripSpaceAndControl_None()
	{
		BString string = "Tonic Water";

		StringUtils::InSituStripSpaceAndControl(string);

		const BString expected = "TonicWater";
		CPPUNIT_ASSERT_EQUAL(expected, string);
	}

	void InSituStripSpaceAndControl_Mixed()
	{
		BString string = "\x01 To\tnic Wa\nter  ";

		StringUtils::InSituStripSpaceAndControl(string);

		const BString expected = "TonicWater";
			// note intervening space also removed
		CPPUNIT_ASSERT_EQUAL(expected, string);
	}

	void InSituTrimSpaceAndControl_None()
	{
		BString string = "Tonic Water";

		StringUtils::InSituTrimSpaceAndControl(string);

		const BString expected = "Tonic Water";
		CPPUNIT_ASSERT_EQUAL(expected, string);
	}

	void InSituTrimSpaceAndControl_BothEnds()
	{
		BString string = "\x01Tonic Water\x02";

		StringUtils::InSituTrimSpaceAndControl(string);

		const BString expected = "Tonic Water";
		CPPUNIT_ASSERT_EQUAL(expected, string);
	}

	void InSituTrimSpaceAndControl_End()
	{
		BString string = "Tonic Water  \x05\t";

		StringUtils::InSituTrimSpaceAndControl(string);

		const BString expected = "Tonic Water";
		CPPUNIT_ASSERT_EQUAL(expected, string);
	}

	void InSituTrimSpaceAndControl_Start()
	{
		BString string = "\t\n Tonic Water";

		StringUtils::InSituTrimSpaceAndControl(string);

		const BString expected = "Tonic Water";
		CPPUNIT_ASSERT_EQUAL(expected, string);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringUtilsTest, getTestSuiteName());
