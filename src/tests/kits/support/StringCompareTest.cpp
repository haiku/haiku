/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringCompareTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringCompareTest);
	CPPUNIT_TEST(LessThan_BString_ReturnsTrueForSmaller);
	CPPUNIT_TEST(LessThanOrEqual_BString_ReturnsTrueForSmaller);
	CPPUNIT_TEST(LessThanOrEqual_BString_ReturnsTrueForEqual);
	CPPUNIT_TEST(Equals_BString_ReturnsTrueForEqual);
	CPPUNIT_TEST(Equals_BString_ReturnsFalseForUnequal);
	CPPUNIT_TEST(GreaterThanOrEqual_BString_ReturnsTrueForGreater);
	CPPUNIT_TEST(GreaterThanOrEqual_BString_ReturnsTrueForEqual);
	CPPUNIT_TEST(GreaterThan_BString_ReturnsTrueForGreater);
	CPPUNIT_TEST(NotEquals_BString_ReturnsFalseForEqual);
	CPPUNIT_TEST(NotEquals_BString_ReturnsTrueForUnequal);
	CPPUNIT_TEST(LessThan_CString_ReturnsTrueForSmaller);
	CPPUNIT_TEST(LessThanOrEqual_CString_ReturnsTrueForSmallerOrEqual);
	CPPUNIT_TEST(Equals_CString_ReturnsTrueForEqual);
	CPPUNIT_TEST(Equals_CString_ReturnsFalseForUnequal);
	CPPUNIT_TEST(GreaterThanOrEqual_CString_ReturnsTrueForGreaterOrEqual);
	CPPUNIT_TEST(GreaterThan_CString_ReturnsTrueForGreater);
	CPPUNIT_TEST(NotEquals_CString_ReturnsFalseForEqual);
	CPPUNIT_TEST(NotEquals_CString_ReturnsTrueForUnequal);
	CPPUNIT_TEST_SUITE_END();

public:
	void LessThan_BString_ReturnsTrueForSmaller()
	{
		BString string1("11111_a");
		BString string2("22222_b");
		CPPUNIT_ASSERT(string1 < string2);
	}

	void LessThanOrEqual_BString_ReturnsTrueForSmaller()
	{
		BString string1("11111_a");
		BString string2("22222_b");
		CPPUNIT_ASSERT(string1 <= string2);
	}

	void LessThanOrEqual_BString_ReturnsTrueForEqual()
	{
		BString string1("11111");
		BString string2("11111");
		CPPUNIT_ASSERT(string1 <= string2);
	}

	void Equals_BString_ReturnsTrueForEqual()
	{
		BString string1("string");
		BString string2("string");
		CPPUNIT_ASSERT(string1 == string2);
	}

	void Equals_BString_ReturnsFalseForUnequal()
	{
		BString string1("text");
		BString string2("string");
		CPPUNIT_ASSERT((string1 == string2) == false);
	}

	void GreaterThanOrEqual_BString_ReturnsTrueForGreater()
	{
		BString string1("BBBBB");
		BString string2("AAAAA");
		CPPUNIT_ASSERT(string1 >= string2);
	}

	void GreaterThanOrEqual_BString_ReturnsTrueForEqual()
	{
		BString string1("11111");
		BString string2("11111");
		CPPUNIT_ASSERT(string1 >= string2);
	}

	void GreaterThan_BString_ReturnsTrueForGreater()
	{
		BString string1("BBBBB");
		BString string2("AAAAA");
		CPPUNIT_ASSERT(string1 > string2);
	}

	void NotEquals_BString_ReturnsFalseForEqual()
	{
		BString string1("string");
		BString string2("string");
		CPPUNIT_ASSERT((string1 != string2) == false);
	}

	void NotEquals_BString_ReturnsTrueForUnequal()
	{
		BString string1("text");
		BString string2("string");
		CPPUNIT_ASSERT(string1 != string2);
	}

	void LessThan_CString_ReturnsTrueForSmaller()
	{
		BString string1("AAAAA");
		CPPUNIT_ASSERT(string1 < "BBBBB");
	}

	void LessThanOrEqual_CString_ReturnsTrueForSmallerOrEqual()
	{
		BString string1("AAAAA");
		CPPUNIT_ASSERT(string1 <= "BBBBB");
		CPPUNIT_ASSERT(string1 <= "AAAAA");
	}

	void Equals_CString_ReturnsTrueForEqual()
	{
		BString string1("AAAAA");
		CPPUNIT_ASSERT(string1 == "AAAAA");
	}

	void Equals_CString_ReturnsFalseForUnequal()
	{
		BString string1("AAAAA");
		CPPUNIT_ASSERT((string1 == "BBBB") == false);
	}

	void GreaterThanOrEqual_CString_ReturnsTrueForGreaterOrEqual()
	{
		BString string1("BBBBB");
		CPPUNIT_ASSERT(string1 >= "AAAAA");
		CPPUNIT_ASSERT(string1 >= "BBBBB");
	}

	void GreaterThan_CString_ReturnsTrueForGreater()
	{
		BString string1("BBBBB");
		CPPUNIT_ASSERT(string1 > "AAAAA");
	}

	void NotEquals_CString_ReturnsFalseForEqual()
	{
		BString string1("AAAAA");
		CPPUNIT_ASSERT((string1 != "AAAAA") == false);
	}

	void NotEquals_CString_ReturnsTrueForUnequal()
	{
		BString string1("AAAAA");
		CPPUNIT_ASSERT(string1 != "BBBB");
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringCompareTest, getTestSuiteName());
