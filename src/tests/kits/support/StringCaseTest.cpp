/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringCaseTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringCaseTest);
	CPPUNIT_TEST(Capitalize_NormalString_CapitalizesFirstLetter);
	CPPUNIT_TEST(Capitalize_StringStartingWithNumber_RemainsUnchanged);
	CPPUNIT_TEST(Capitalize_EmptyString_RemainsEmpty);
	CPPUNIT_TEST(ToLower_MixedCase_ConvertsToLowercase);
	CPPUNIT_TEST(ToLower_EmptyString_RemainsEmpty);
	CPPUNIT_TEST(ToUpper_MixedCase_ConvertsToUppercase);
	CPPUNIT_TEST(ToUpper_EmptyString_RemainsEmpty);
	CPPUNIT_TEST(CapitalizeEachWord_MixedString_CapitalizesWords);
	CPPUNIT_TEST(CapitalizeEachWord_EmptyString_RemainsEmpty);
	CPPUNIT_TEST_SUITE_END();

public:
	void Capitalize_NormalString_CapitalizesFirstLetter()
	{
		BString string("this is a sentence");
		string.Capitalize();
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "This is a sentence"));
	}

	void Capitalize_StringStartingWithNumber_RemainsUnchanged()
	{
		BString string("134this is a sentence");
		string.Capitalize();
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "134this is a sentence"));
	}

	void Capitalize_EmptyString_RemainsEmpty()
	{
		BString string;
		string.Capitalize();
		CPPUNIT_ASSERT_EQUAL(BString(""), string);
	}

	void ToLower_MixedCase_ConvertsToLowercase()
	{
		BString string("1a2B3c4d5e6f7G");
		string.ToLower();
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "1a2b3c4d5e6f7g"));
	}

	void ToLower_EmptyString_RemainsEmpty()
	{
		BString string;
		string.ToLower();
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), ""));
	}

	void ToUpper_MixedCase_ConvertsToUppercase()
	{
		BString string("1a2b3c4d5E6f7g");
		string.ToUpper();
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "1A2B3C4D5E6F7G"));
	}

	void ToUpper_EmptyString_RemainsEmpty()
	{
		BString string;
		string.ToUpper();
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), ""));
	}

	void CapitalizeEachWord_MixedString_CapitalizesWords()
	{
		BString string("each wOrd 3will_be >capiTalized");
		string.CapitalizeEachWord();
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "Each Word 3Will_Be >Capitalized"));
	}

	void CapitalizeEachWord_EmptyString_RemainsEmpty()
	{
		BString string;
		string.CapitalizeEachWord();
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), ""));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringCaseTest, getTestSuiteName());
