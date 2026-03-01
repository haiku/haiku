/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringConstructionTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringConstructionTest);
	CPPUNIT_TEST(Construct_Empty_CreatesEmptyString);
	CPPUNIT_TEST(Construct_FromCString_CreatesMatchingString);
	CPPUNIT_TEST(Construct_FromNull_CreatesEmptyString);
	CPPUNIT_TEST(Construct_FromBString_CreatesMatchingString);
	CPPUNIT_TEST(Construct_FromCStringWithLength_CreatesSubstring);
#if __cplusplus >= 201103L
	CPPUNIT_TEST(Construct_FromMovableBString_MovesString);
#endif
	CPPUNIT_TEST(Construct_FromCStringWithExcessLength_CreatesMatchingString);
	CPPUNIT_TEST_SUITE_END();

public:
	void Construct_Empty_CreatesEmptyString()
	{
		BString string;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), ""));
		CPPUNIT_ASSERT_EQUAL(0, string.Length());
	}

	void Construct_FromCString_CreatesMatchingString()
	{
		const char* str = "Something";
		BString string(str);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), str));
		CPPUNIT_ASSERT_EQUAL((unsigned)strlen(str), string.Length());
	}

	void Construct_FromNull_CreatesEmptyString()
	{
		BString string(NULL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), ""));
		CPPUNIT_ASSERT_EQUAL(0, string.Length());
	}

	void Construct_FromBString_CreatesMatchingString()
	{
		BString anotherString("Something Else");
		BString string(anotherString);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), anotherString.String()));
		CPPUNIT_ASSERT_EQUAL(anotherString.Length(), string.Length());
	}

	void Construct_FromCStringWithLength_CreatesSubstring()
	{
		const char* str = "Something";
		BString string(str, 5);
		CPPUNIT_ASSERT(strcmp(string.String(), str) != 0);
		CPPUNIT_ASSERT_EQUAL(0, strncmp(string.String(), str, 5));
		CPPUNIT_ASSERT_EQUAL(5, string.Length());
	}

#if __cplusplus >= 201103L
	void Construct_FromMovableBString_MovesString()
	{
		const char* str = "Something";
		BString movableString(str);
		BString string(std::move(movableString));
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), str));
		CPPUNIT_ASSERT_EQUAL((unsigned)strlen(str), string.Length());
		CPPUNIT_ASSERT_EQUAL(0, strcmp(movableString.String(), ""));
		CPPUNIT_ASSERT_EQUAL(0, movableString.Length());
	}
#endif

	void Construct_FromCStringWithExcessLength_CreatesMatchingString()
	{
		const char* str = "Something";
		BString string(str, 255);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), str));
		CPPUNIT_ASSERT_EQUAL((unsigned)strlen(str), string.Length());
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringConstructionTest, getTestSuiteName());
