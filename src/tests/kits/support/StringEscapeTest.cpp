/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringEscapeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringEscapeTest);
	CPPUNIT_TEST(CharacterEscape_ValidString_EscapesChars);
	CPPUNIT_TEST(CharacterEscape_EmptyString_RemainsEmpty);
	CPPUNIT_TEST(CharacterEscape_StringWithoutChars_RemainsUnchanged);
	CPPUNIT_TEST(CharacterEscape_WithOriginalString_EscapesAndAssigns);
	CPPUNIT_TEST(CharacterEscape_EmptyStringWithOriginalString_EscapesAndAssigns);
	CPPUNIT_TEST(CharacterDeescape_ValidString_DeescapesChars);
	CPPUNIT_TEST(CharacterDeescape_EmptyString_RemainsEmpty);
	CPPUNIT_TEST(CharacterDeescape_StringWithoutEscapeChar_RemainsUnchanged);
	CPPUNIT_TEST(CharacterDeescape_WithOriginalString_DeescapesAndAssigns);
	CPPUNIT_TEST(CharacterDeescape_EmptyStringWithOriginalString_DeescapesAndAssigns);
	CPPUNIT_TEST(CharacterDeescape_OriginalStringWithoutEscapeChar_AssignsUnchanged);
#ifndef TEST_R5
	CPPUNIT_TEST(CharacterEscape_WithNullOriginalString_AssignsEmpty);
	CPPUNIT_TEST(CharacterDeescape_WithNullOriginalString_AssignsEmpty);
#endif
	CPPUNIT_TEST_SUITE_END();

public:
	void CharacterEscape_ValidString_EscapesChars()
	{
		BString string1("abcdefghi");
		string1.CharacterEscape("acf", '/');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "/ab/cde/fghi"));
	}

	void CharacterEscape_EmptyString_RemainsEmpty()
	{
		BString string1;
		string1.CharacterEscape("abc", '/');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), ""));
	}

	void CharacterEscape_StringWithoutChars_RemainsUnchanged()
	{
		BString string1("abcdefghi");
		string1.CharacterEscape("z34", 'z');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "abcdefghi"));
	}

	void CharacterEscape_WithOriginalString_EscapesAndAssigns()
	{
		BString string1("something");
		string1.CharacterEscape("newstring", "esi", '0');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "n0ew0str0ing"));
	}

	void CharacterEscape_EmptyStringWithOriginalString_EscapesAndAssigns()
	{
		BString string1;
		string1.CharacterEscape("newstring", "esi", '0');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "n0ew0str0ing"));
	}

	void CharacterDeescape_ValidString_DeescapesChars()
	{
		BString string1("/a/nh/g/bhhgy/fgtuhjkb/");
		string1.CharacterDeescape('/');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "anhgbhhgyfgtuhjkb"));
	}

	void CharacterDeescape_EmptyString_RemainsEmpty()
	{
		BString string1;
		string1.CharacterDeescape('/');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), ""));
	}

	void CharacterDeescape_StringWithoutEscapeChar_RemainsUnchanged()
	{
		BString string1("/a/nh/g/bhhgy/fgtuhjkb/");
		string1.CharacterDeescape('-');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "/a/nh/g/bhhgy/fgtuhjkb/"));
	}

	void CharacterDeescape_WithOriginalString_DeescapesAndAssigns()
	{
		BString string1("oldString");
		string1.CharacterDeescape("-ne-ws-tri-ng-", '-');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "newstring"));
	}

	void CharacterDeescape_EmptyStringWithOriginalString_DeescapesAndAssigns()
	{
		BString string1;
		string1.CharacterDeescape("new/str/ing", '/');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "newstring"));
	}

	void CharacterDeescape_OriginalStringWithoutEscapeChar_AssignsUnchanged()
	{
		BString string1("Old");
		string1.CharacterDeescape("/a/nh/g/bhhgy/fgtuhjkb/", '-');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "/a/nh/g/bhhgy/fgtuhjkb/"));
	}

#ifndef TEST_R5
	void CharacterEscape_WithNullOriginalString_AssignsEmpty()
	{
		BString string1("something");
		string1.CharacterEscape((char*)NULL, "ei", '-');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), ""));
	}

	void CharacterDeescape_WithNullOriginalString_AssignsEmpty()
	{
		BString string1("pippo");
		string1.CharacterDeescape((char*)NULL, '/');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), ""));
	}
#endif
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringEscapeTest, getTestSuiteName());
