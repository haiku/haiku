/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>
#include <UTF8.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringUTF8Test : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringUTF8Test);
	CPPUNIT_TEST(LengthAndCountChars_UTF8String_ReturnsCorrectValues);
	CPPUNIT_TEST(ReplaceCharsSet_UTF8String_ReplacesChars);
	CPPUNIT_TEST(MoveCharsInto_UTF8String_MovesCharsAndLeavesEllipsis);
	CPPUNIT_TEST(RemoveCharsSet_UTF8String_RemovesSpecifiedChars);
	CPPUNIT_TEST(SetToChars_UTF8String_SetsStringCorrectly);
	CPPUNIT_TEST(TruncateChars_UTF8String_TruncatesString);
	CPPUNIT_TEST(AppendChars_UTF8String_AppendsCharsCorrectly);
	CPPUNIT_TEST(RemoveChars_UTF8String_RemovesCharsCorrectly);
	CPPUNIT_TEST(InsertChars_UTF8String_InsertsCharsCorrectly);
	CPPUNIT_TEST(PrependChars_UTF8String_PrependsCharsCorrectly);
	CPPUNIT_TEST(CompareChars_UTF8String_ComparesCorrectly);
	CPPUNIT_TEST(CountBytes_UTF8String_CountsBytesCorrectly);
	CPPUNIT_TEST_SUITE_END();

public:
	void LengthAndCountChars_UTF8String_ReturnsCorrectValues()
	{
		BString string("ü-ä-ö");
		CPPUNIT_ASSERT_EQUAL(8, string.Length());
		CPPUNIT_ASSERT_EQUAL(5, string.CountChars());
		CPPUNIT_ASSERT_EQUAL(0, memcmp(string.String(), "ü-ä-ö", 8));
	}

	void ReplaceCharsSet_UTF8String_ReplacesChars()
	{
		BString string("ü-ä-ö");
		string.ReplaceCharsSet("üö", B_UTF8_ELLIPSIS);
		CPPUNIT_ASSERT_EQUAL(10, string.Length());
		CPPUNIT_ASSERT_EQUAL(5, string.CountChars());
		CPPUNIT_ASSERT_EQUAL(0, memcmp(string.String(), B_UTF8_ELLIPSIS "-ä-" B_UTF8_ELLIPSIS, 10));
	}

	void MoveCharsInto_UTF8String_MovesCharsAndLeavesEllipsis()
	{
		BString string(B_UTF8_ELLIPSIS "-ä-" B_UTF8_ELLIPSIS);
		BString ellipsis;
		string.MoveCharsInto(ellipsis, 4, 1);
		CPPUNIT_ASSERT_EQUAL(7, string.Length());
		CPPUNIT_ASSERT_EQUAL(4, string.CountChars());
		CPPUNIT_ASSERT_EQUAL(0, memcmp(string.String(), B_UTF8_ELLIPSIS "-ä-", 7));
		CPPUNIT_ASSERT_EQUAL(3, ellipsis.Length());
		CPPUNIT_ASSERT_EQUAL(1, ellipsis.CountChars());
		CPPUNIT_ASSERT_EQUAL(0, memcmp(ellipsis.String(), B_UTF8_ELLIPSIS, 3));
	}

	void RemoveCharsSet_UTF8String_RemovesSpecifiedChars()
	{
		BString string(B_UTF8_ELLIPSIS "-ä-");
		string.RemoveCharsSet("-" B_UTF8_ELLIPSIS);
		CPPUNIT_ASSERT_EQUAL(2, string.Length());
		CPPUNIT_ASSERT_EQUAL(1, string.CountChars());
		CPPUNIT_ASSERT_EQUAL(0, memcmp(string.String(), "ä", 2));
	}

	void SetToChars_UTF8String_SetsStringCorrectly()
	{
		BString string("ä");
		string.SetToChars("öäü" B_UTF8_ELLIPSIS "öäü", 5);
		CPPUNIT_ASSERT_EQUAL(11, string.Length());
		CPPUNIT_ASSERT_EQUAL(5, string.CountChars());
		CPPUNIT_ASSERT_EQUAL(0, memcmp(string.String(), "öäü" B_UTF8_ELLIPSIS "ö", 11));
	}

	void TruncateChars_UTF8String_TruncatesString()
	{
		BString string("öäü" B_UTF8_ELLIPSIS "ö");
		string.TruncateChars(4);
		CPPUNIT_ASSERT_EQUAL(9, string.Length());
		CPPUNIT_ASSERT_EQUAL(4, string.CountChars());
		CPPUNIT_ASSERT_EQUAL(0, memcmp(string.String(), "öäü" B_UTF8_ELLIPSIS, 9));
	}

	void AppendChars_UTF8String_AppendsCharsCorrectly()
	{
		BString string("öäü" B_UTF8_ELLIPSIS);
		string.AppendChars("öäü", 2);
		CPPUNIT_ASSERT_EQUAL(13, string.Length());
		CPPUNIT_ASSERT_EQUAL(6, string.CountChars());
		CPPUNIT_ASSERT_EQUAL(0, memcmp(string.String(), "öäü" B_UTF8_ELLIPSIS "öä", 13));
	}

	void RemoveChars_UTF8String_RemovesCharsCorrectly()
	{
		BString string("öäü" B_UTF8_ELLIPSIS "öä");
		string.RemoveChars(1, 3);
		CPPUNIT_ASSERT_EQUAL(6, string.Length());
		CPPUNIT_ASSERT_EQUAL(3, string.CountChars());
		CPPUNIT_ASSERT_EQUAL(0, memcmp(string.String(), "ööä", 6));
	}

	void InsertChars_UTF8String_InsertsCharsCorrectly()
	{
		BString string("ööä");
		string.InsertChars("öäü" B_UTF8_ELLIPSIS B_UTF8_ELLIPSIS "ä", 3, 2, 1);
		CPPUNIT_ASSERT_EQUAL(12, string.Length());
		CPPUNIT_ASSERT_EQUAL(5, string.CountChars());
		CPPUNIT_ASSERT_EQUAL(0, memcmp(string.String(), "ö" B_UTF8_ELLIPSIS B_UTF8_ELLIPSIS "öä", 12));
	}

	void PrependChars_UTF8String_PrependsCharsCorrectly()
	{
		BString string("ö" B_UTF8_ELLIPSIS B_UTF8_ELLIPSIS "öä");
		string.PrependChars("ää+üü", 3);
		CPPUNIT_ASSERT_EQUAL(17, string.Length());
		CPPUNIT_ASSERT_EQUAL(8, string.CountChars());
		CPPUNIT_ASSERT_EQUAL(0, memcmp(string.String(), "ää+ö" B_UTF8_ELLIPSIS B_UTF8_ELLIPSIS "öä", 17));
	}

	void CompareChars_UTF8String_ComparesCorrectly()
	{
		BString string("ää+ö" B_UTF8_ELLIPSIS B_UTF8_ELLIPSIS "öä");
		const char* compare = "ää+ö" B_UTF8_ELLIPSIS "different";
		CPPUNIT_ASSERT(string.CompareChars(compare, 5) == 0);
		CPPUNIT_ASSERT(string.CompareChars(compare, 6) != 0);
	}

	void CountBytes_UTF8String_CountsBytesCorrectly()
	{
		BString string("ää+ö" B_UTF8_ELLIPSIS B_UTF8_ELLIPSIS "öä");
		CPPUNIT_ASSERT_EQUAL(6, string.CountBytes(2, 3));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringUTF8Test, getTestSuiteName());
