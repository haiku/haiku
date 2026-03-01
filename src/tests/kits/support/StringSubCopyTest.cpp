/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringSubCopyTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringSubCopyTest);
	CPPUNIT_TEST(BString_CopiesSubstring);
	CPPUNIT_TEST(CString_CopiesSubstring);
	CPPUNIT_TEST_SUITE_END();

public:
	void BString_CopiesSubstring()
	{
		BString string1;
		BString string2("Something");
		string2.CopyInto(string1, 4, 30);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "thing"));
	}

	void CString_CopiesSubstring()
	{
		char tmp[10];
		memset(tmp, 0, 10);
		BString string1("ABC");
		string1.CopyInto(tmp, 0, 4);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(tmp, "ABC"));
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string1.String(), "ABC"));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringSubCopyTest, getTestSuiteName());
