/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringAssignTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringAssignTest);
	CPPUNIT_TEST(Assign_BString_CreatesMatchingString);
	CPPUNIT_TEST(Assign_CString_CreatesMatchingString);
#if __cplusplus >= 201103L
	CPPUNIT_TEST(Assign_MovableBString_MovesString);
#endif
	CPPUNIT_TEST(Assign_NullPointer_CreatesEmptyString);
	CPPUNIT_TEST(SetTo_NullPointer_CreatesEmptyString);
	CPPUNIT_TEST(SetTo_CString_CreatesMatchingString);
	CPPUNIT_TEST(SetTo_BString_CreatesMatchingString);
	CPPUNIT_TEST(SetTo_CharAndLength_CreatesRepeatedString);
	CPPUNIT_TEST(SetTo_CharAndZeroLength_CreatesEmptyString);
	CPPUNIT_TEST(SetTo_CStringAndLength_CreatesSubstring);
	CPPUNIT_TEST(Adopt_BString_AdoptsStringAndClearsOriginal);
	CPPUNIT_TEST(Adopt_BStringAndLength_AdoptsSubstringAndClearsOriginal);
#ifndef TEST_R5
	//CPPUNIT_TEST(SetTo_CharAndExcessLength_CreatesMatchingString);
	CPPUNIT_TEST(SetTo_CStringAndExcessLength_CreatesMatchingString);
#endif
	CPPUNIT_TEST_SUITE_END();

public:
	void Assign_BString_CreatesMatchingString()
	{
		BString string;
		BString string2("Something");
		string = string2;
		CPPUNIT_ASSERT_EQUAL(string2, string);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "Something"));
	}

	void Assign_CString_CreatesMatchingString()
	{
		BString str;
		str = "Something Else";
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "Something Else"));
	}

#if __cplusplus >= 201103L
	void Assign_MovableBString_MovesString()
	{
		BString movableString("Something movable");
		BString str;
		str = std::move(movableString);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "Something movable"));
		CPPUNIT_ASSERT_EQUAL(0, strcmp(movableString.String(), ""));
	}
#endif

	void Assign_NullPointer_CreatesEmptyString()
	{
		char* s = NULL;
		BString str;
		str = s;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), ""));
	}

	void SetTo_NullPointer_CreatesEmptyString()
	{
		char* s = NULL;
		BString str;
		str.SetTo(s);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), ""));
	}

	void SetTo_CString_CreatesMatchingString()
	{
		BString str;
		str.SetTo("BLA");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "BLA"));
	}

	void SetTo_BString_CreatesMatchingString()
	{
		BString string("Something");
		BString str;
		str.SetTo(string);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), string.String()));
	}

	void SetTo_CharAndLength_CreatesRepeatedString()
	{
		BString str;
		str.SetTo('C', 10);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "CCCCCCCCCC"));
	}

	void SetTo_CharAndZeroLength_CreatesEmptyString()
	{
		BString str("ASDSGAFA");
		str.SetTo('C', 0);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), ""));
	}

	void SetTo_CStringAndLength_CreatesSubstring()
	{
		BString str;
		str.SetTo("ABC", 10);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "ABC"));
	}

	void Adopt_BString_AdoptsStringAndClearsOriginal()
	{
		BString string2("Something");
		BString str;
		str.Adopt(string2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "Something"));
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string2.String(), ""));
	}

	void Adopt_BStringAndLength_AdoptsSubstringAndClearsOriginal()
	{
		BString newstring("SomethingElseAgain");
		BString str;
		str.Adopt(newstring, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "So"));
		CPPUNIT_ASSERT_EQUAL(2, str.Length());
		CPPUNIT_ASSERT_EQUAL(0, strcmp(newstring.String(), ""));
	}

#ifndef TEST_R5
	// TODO: The following test cases only work with hoard2, which will not
	// allow allocations via malloc() larger than the largest size-class
	// (see threadHeap::malloc(size_t). Other malloc implementations like
	// rpmalloc will allow arbitrarily large allocations via create_area().
	//
	// This test should be made more robust by breaking the dependency on
	// the allocator to simulate failures in another way. This may require
	// a tricky build configuration to avoid breaking the ABI of BString.
	void SetTo_CharAndExcessLength_CreatesMatchingString()
	{
		const int32 OUT_OF_MEM_VAL = 2 * 1000 * 1000 * 1000;
		BString str("dummy");
		str.SetTo('C', OUT_OF_MEM_VAL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "dummy"));
	}

	void SetTo_CStringAndExcessLength_CreatesMatchingString()
	{
		const int32 OUT_OF_MEM_VAL = 2 * 1000 * 1000 * 1000;
		BString str("dummy");
		str.SetTo("some more text", OUT_OF_MEM_VAL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str.String(), "some more text"));
	}
#endif
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringAssignTest, getTestSuiteName());
