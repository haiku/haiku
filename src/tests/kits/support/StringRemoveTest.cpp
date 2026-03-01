/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringRemoveTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringRemoveTest);
	CPPUNIT_TEST(Truncate_Lazy);
	CPPUNIT_TEST(Truncate_NotLazy);
#ifndef TEST_R5
	CPPUNIT_TEST(Truncate_NegativeLength);
#endif
	CPPUNIT_TEST(Truncate_LongerLength);
	CPPUNIT_TEST(Truncate_EmptyString);
	CPPUNIT_TEST(Remove_ValidRange);
	CPPUNIT_TEST(Remove_EmptyString);
	CPPUNIT_TEST(Remove_BeyondEnd);
	CPPUNIT_TEST(Remove_ExceedsLength);
	CPPUNIT_TEST(Remove_NegativeIndex);
	CPPUNIT_TEST(RemoveFirst_BString_Match);
	CPPUNIT_TEST(RemoveFirst_BString_NoMatch);
	CPPUNIT_TEST(RemoveLast_BString_Match);
	CPPUNIT_TEST(RemoveLast_BString_NoMatch);
	CPPUNIT_TEST(RemoveAll_BString_Match);
	CPPUNIT_TEST(RemoveAll_BString_NoMatch);
	CPPUNIT_TEST(RemoveFirst_CString_Match);
	CPPUNIT_TEST(RemoveFirst_CString_NoMatch);
	CPPUNIT_TEST(RemoveFirst_CString_Null);
	CPPUNIT_TEST(RemoveLast_CString_Match);
	CPPUNIT_TEST(RemoveLast_CString_NoMatch);
	CPPUNIT_TEST(RemoveAll_CString_Match);
	CPPUNIT_TEST(RemoveAll_CString_NoMatch);
	CPPUNIT_TEST(RemoveSet_Match);
	CPPUNIT_TEST(RemoveSet_NoMatch);
	CPPUNIT_TEST(MoveInto_BString_ValidRange);
	CPPUNIT_TEST(MoveInto_BString_ExceedsLength);
	CPPUNIT_TEST(MoveInto_CString_ValidRange);
	CPPUNIT_TEST(MoveInto_CString_ExceedsLength);
	CPPUNIT_TEST_SUITE_END();

public:
	void Truncate_Lazy()
	{
		BString string1("This is a long string");
		string1.Truncate(14, true);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "This is a long"));
		CPPUNIT_ASSERT_EQUAL(14, string1.Length());
	}

	void Truncate_NotLazy()
	{
		BString string1("This is a long string");
		string1.Truncate(14, false);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "This is a long"));
		CPPUNIT_ASSERT_EQUAL(14, string1.Length());
	}
#ifndef TEST_R5
	void Truncate_NegativeLength()
	{
		// it crashes r5 implementation, but ours works fine here,
		// in this case, we just truncate to 0
		BString string1("This is a long string");
		string1.Truncate(-3);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), ""));
		CPPUNIT_ASSERT_EQUAL(0, string1.Length());
	}
#endif
	void Truncate_LongerLength()
	{
		BString string1("This is a long string");
		string1.Truncate(45);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "This is a long string"));
		CPPUNIT_ASSERT_EQUAL(21, string1.Length());
	}

	void Truncate_EmptyString()
	{
		BString string1;
		string1.Truncate(0);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), ""));
		CPPUNIT_ASSERT_EQUAL(0, string1.Length());
	}

	void Remove_ValidRange()
	{
		BString string1("a String");
		string1.Remove(2, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "a ring"));
	}

	void Remove_EmptyString()
	{
		BString string1;
		string1.Remove(2, 1);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), ""));
	}

	void Remove_BeyondEnd()
	{
		BString string1("a String");
		string1.Remove(20, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "a String"));
	}

	void Remove_ExceedsLength()
	{
		BString string1("a String");
		string1.Remove(4, 30);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "a St"));
	}

	void Remove_NegativeIndex()
	{
		BString string1("a String");
		string1.Remove(-3, 5);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "ing"));
	}

	void RemoveFirst_BString_Match()
	{
		BString string1("first second first");
		BString string2("first");
		string1.RemoveFirst(string2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), " second first"));
	}

	void RemoveFirst_BString_NoMatch()
	{
		BString string1("first second first");
		BString string2("noway");
		string1.RemoveFirst(string2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "first second first"));
	}

	void RemoveLast_BString_Match()
	{
		BString string1("first second first");
		BString string2("first");
		string1.RemoveLast(string2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "first second "));
	}

	void RemoveLast_BString_NoMatch()
	{
		BString string1("first second first");
		BString string2("noway");
		string1.RemoveLast(string2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "first second first"));
	}

	void RemoveAll_BString_Match()
	{
		BString string1("first second first");
		BString string2("first");
		string1.RemoveAll(string2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), " second "));
	}

	void RemoveAll_BString_NoMatch()
	{
		BString string1("first second first");
		BString string2("noway");
		string1.RemoveAll(string2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "first second first"));
	}

	void RemoveFirst_CString_Match()
	{
		BString string1("first second first");
		string1.RemoveFirst("first");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), " second first"));
	}

	void RemoveFirst_CString_NoMatch()
	{
		BString string1("first second first");
		string1.RemoveFirst("noway");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "first second first"));
	}

	void RemoveFirst_CString_Null()
	{
		BString string1("first second first");
		string1.RemoveFirst((char*)NULL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "first second first"));
	}

	void RemoveLast_CString_Match()
	{
		BString string1("first second first");
		string1.RemoveLast("first");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "first second "));
	}

	void RemoveLast_CString_NoMatch()
	{
		BString string1("first second first");
		string1.RemoveLast("noway");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "first second first"));
	}

	void RemoveAll_CString_Match()
	{
		BString string1("first second first");
		string1.RemoveAll("first");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), " second "));
	}

	void RemoveAll_CString_NoMatch()
	{
		BString string1("first second first");
		string1.RemoveAll("noway");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "first second first"));
	}

	void RemoveSet_Match()
	{
		BString string1("a sentence with (3) (642) numbers (2) in it");
		string1.RemoveSet("()3624 ");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "asentencewithnumbersinit"));
	}

	void RemoveSet_NoMatch()
	{
		BString string1("a string");
		string1.RemoveSet("1345");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "a string"));
	}

	void MoveInto_BString_ValidRange()
	{
		BString string1("some text");
		BString string2("string");
		string2.MoveInto(string1, 3, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "in"));
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string2.String(), "strg"));
	}

	void MoveInto_BString_ExceedsLength()
	{
		BString string1("some text");
		BString string2("string");
		string2.MoveInto(string1, 0, 200);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "string"));
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string2.String(), ""));
	}

	void MoveInto_CString_ValidRange()
	{
		char dest[100];
		memset(dest, 0, 100);
		BString string1("some text");
		string1.MoveInto(dest, 3, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(dest, "e "));
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "somtext"));
	}

	void MoveInto_CString_ExceedsLength()
	{
		char dest[100];
		BString string1("some text");
		memset(dest, 0, 100);
		string1.MoveInto(dest, 0, 50);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(dest, "some text"));
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), ""));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringRemoveTest, getTestSuiteName());
