/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>
#include <StringList.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringSplitTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringSplitTest);
	CPPUNIT_TEST(SingleCharIgnoreEmpty_SplitsCorrectly);
	CPPUNIT_TEST(StringIgnoreEmpty_SplitsCorrectly);
	CPPUNIT_TEST(StringKeepEmpty_SplitsCorrectly);
	CPPUNIT_TEST(SingleCharKeepEmpty_SplitsCorrectly);
	CPPUNIT_TEST_SUITE_END();

public:
	void SingleCharIgnoreEmpty_SplitsCorrectly()
	{
		BString str1("test::string");
		BStringList stringList1;
		str1.Split(":", true, stringList1);
		CPPUNIT_ASSERT_EQUAL(2, stringList1.CountStrings());
	}

	void StringIgnoreEmpty_SplitsCorrectly()
	{
		BString str1("test::string");
		BStringList stringList2;
		str1.Split("::", true, stringList2);
		CPPUNIT_ASSERT_EQUAL(2, stringList2.CountStrings());
	}

	void StringKeepEmpty_SplitsCorrectly()
	{
		BString str1("test::string");
		BStringList stringList3;
		str1.Split("::", false, stringList3);
		CPPUNIT_ASSERT_EQUAL(2, stringList3.CountStrings());
	}

	void SingleCharKeepEmpty_SplitsCorrectly()
	{
		BString str1("test::string");
		BStringList stringList4;
		str1.Split(":", false, stringList4);
		CPPUNIT_ASSERT_EQUAL(3, stringList4.CountStrings());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringSplitTest, getTestSuiteName());
