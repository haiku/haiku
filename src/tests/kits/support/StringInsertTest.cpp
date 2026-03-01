/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringInsertTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringInsertTest);
	CPPUNIT_TEST(CStringPos_InsertsString);
	CPPUNIT_TEST(CStringNegativePos_InsertsStringWithOffset);
	CPPUNIT_TEST(CStringLenPos_InsertsSubstring);
#ifndef TEST_R5
	CPPUNIT_TEST(CStringInvalidPos_IgnoresInsertion);
	CPPUNIT_TEST(CStringLargeNegativePos_ClearsString);
	CPPUNIT_TEST(CStringLenInvalidPos_IgnoresInsertion);
#endif
	CPPUNIT_TEST(CStringLargeLenPos_InsertsWholeString);
	CPPUNIT_TEST(CStringOffsetLenPos_InsertsSubstringFromOffset);
	CPPUNIT_TEST(CharCountPos_InsertsMultipleChars);
	CPPUNIT_TEST(CharCountNegativePos_InsertsMultipleCharsAtZero);
	CPPUNIT_TEST(BStringPos_InsertsString);
	CPPUNIT_TEST(SelfPos_IgnoresInsertion);
	CPPUNIT_TEST(BStringNegativePos_InsertsStringWithOffset);
	CPPUNIT_TEST(BStringLenPos_InsertsSubstring);
	CPPUNIT_TEST(BStringOffsetLenPos_InsertsSubstringFromOffset);
	CPPUNIT_TEST_SUITE_END();

public:
	void CStringPos_InsertsString()
	{
		BString str1("String");
		str1.Insert("INSERTED", 3);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "StrINSERTEDing"));
	}

	void CStringNegativePos_InsertsStringWithOffset()
	{
		BString str1;
		str1.Insert("INSERTED", -1);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "NSERTED"));
	}

	void CStringLenPos_InsertsSubstring()
	{
		BString str1("string");
		str1.Insert("INSERTED", 2, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "stINring"));
	}

#ifndef TEST_R5
	void CStringInvalidPos_IgnoresInsertion()
	{
		BString str1("String");
		str1.Insert("INSERTED", 10);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "String"));
	}

	void CStringLargeNegativePos_ClearsString()
	{
		BString str1;
		str1.Insert("INSERTED", -142364253);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), ""));
	}

	void CStringLenInvalidPos_IgnoresInsertion()
	{
		BString str1("string");
		str1.Insert("INSERTED", 2, 30);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "string"));
	}
#endif

	void CStringLargeLenPos_InsertsWholeString()
	{
		BString str1("string");
		str1.Insert("INSERTED", 10, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "stINSERTEDring"));
	}

	void CStringOffsetLenPos_InsertsSubstringFromOffset()
	{
		BString str1("string");
		str1.Insert("INSERTED", 4, 30, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "stRTEDring"));
	}

	void CharCountPos_InsertsMultipleChars()
	{
		BString str1("string");
		str1.Insert('P', 5, 3);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "strPPPPPing"));
	}

	void CharCountNegativePos_InsertsMultipleCharsAtZero()
	{
		BString str1("string");
		str1.Insert('P', 5, -2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "PPPstring"));
	}

	void BStringPos_InsertsString()
	{
		BString str1("string");
		BString str2("INSERTED");
		str1.Insert(str2, 0);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "INSERTEDstring"));
	}

	void SelfPos_IgnoresInsertion()
	{
		BString str1("string");
		str1.Insert(str1, 0);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "string"));
	}

	void BStringNegativePos_InsertsStringWithOffset()
	{
		BString str1;
		BString str2("INSERTED");
		str1.Insert(str2, -1);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "NSERTED"));
	}

	void BStringLenPos_InsertsSubstring()
	{
		BString str1("string");
		BString str2("INSERTED");
		str1.Insert(str2, 2, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "stINring"));
	}

	void BStringOffsetLenPos_InsertsSubstringFromOffset()
	{
		BString str1("string");
		BString str2("INSERTED");
		str1.Insert(str2, 4, 30, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "stRTEDring"));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringInsertTest, getTestSuiteName());
