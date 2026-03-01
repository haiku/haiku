/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringFormatAppendTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringFormatAppendTest);
	CPPUNIT_TEST(Append_CString_AppendsString);
	CPPUNIT_TEST(Append_BString_AppendsString);
	CPPUNIT_TEST(Append_Char_AppendsChars);
	CPPUNIT_TEST(Append_Int_AppendsInt);
	CPPUNIT_TEST(Append_NegativeInt_AppendsInt);
	CPPUNIT_TEST(Append_UnsignedInt_AppendsUnsignedInt);
	CPPUNIT_TEST(Append_Uint32_AppendsUint32);
	CPPUNIT_TEST(Append_Int32_AppendsInt32);
	CPPUNIT_TEST(Append_NegativeInt32_AppendsInt32);
	CPPUNIT_TEST(Append_Uint64_AppendsUint64);
	CPPUNIT_TEST(Append_Int64_AppendsInt64);
	CPPUNIT_TEST(Append_NegativeInt64_AppendsInt64);
	CPPUNIT_TEST(Append_Float_AppendsFloat);
	CPPUNIT_TEST(Append_MultipleTypes_AppendsAll);
	CPPUNIT_TEST_SUITE_END();

public:
	void Append_CString_AppendsString()
	{
		BString string("some");
		string << " " << "text";
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "some text"));
	}

	void Append_BString_AppendsString()
	{
		BString string("some ");
		BString string2("text");
		string << string2;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "some text"));
	}

	void Append_Char_AppendsChars()
	{
		BString string("str");
		string << 'i' << 'n' << 'g';
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "string"));
	}

	void Append_Int_AppendsInt()
	{
		BString string("level ");
		string << (int)42;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "level 42"));
	}

	void Append_NegativeInt_AppendsInt()
	{
		BString string("error ");
		string << (int)-1;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "error -1"));
	}

	void Append_UnsignedInt_AppendsUnsignedInt()
	{
		BString string("number ");
		string << (unsigned int)296;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "number 296"));
	}

	void Append_Uint32_AppendsUint32()
	{
		BString string;
		string << (uint32)102456;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "102456"));
	}

	void Append_Int32_AppendsInt32()
	{
		BString string;
		string << (int32)112456;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "112456"));
	}

	void Append_NegativeInt32_AppendsInt32()
	{
		BString string;
		string << (int32)-112475;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "-112475"));
	}

	void Append_Uint64_AppendsUint64()
	{
		BString string;
		string << (uint64)1145267987;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "1145267987"));
	}

	void Append_Int64_AppendsInt64()
	{
		BString string;
		string << (int64)112456;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "112456"));
	}

	void Append_NegativeInt64_AppendsInt64()
	{
		BString string;
		string << (int64)-112475;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "-112475"));
	}

	void Append_Float_AppendsFloat()
	{
		BString string;
		string << (float)34.542;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "34.54"));
	}

	void Append_MultipleTypes_AppendsAll()
	{
		BString s;
		s << "This" << ' ' << "is" << ' ' << 'a' << ' ' << "test" << ' ' << "sentence";
		CPPUNIT_ASSERT_EQUAL(0, strcmp(s.String(), "This is a test sentence"));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringFormatAppendTest, getTestSuiteName());
