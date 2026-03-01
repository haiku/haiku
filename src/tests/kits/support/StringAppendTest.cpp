/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringAppendTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringAppendTest);
	CPPUNIT_TEST(PlusEquals_BString_AppendsString);
	CPPUNIT_TEST(PlusEquals_CString_AppendsString);
	CPPUNIT_TEST(PlusEquals_CStringToEmpty_AppendsString);
	CPPUNIT_TEST(PlusEquals_NullPointer_IgnoresAppend);
	CPPUNIT_TEST(PlusEquals_Char_AppendsChar);
	CPPUNIT_TEST(Append_BString_AppendsString);
	CPPUNIT_TEST(Append_CString_AppendsString);
	CPPUNIT_TEST(Append_CStringToEmpty_AppendsString);
	CPPUNIT_TEST(Append_NullPointer_IgnoresAppend);
	CPPUNIT_TEST(Append_BStringAndLength_AppendsSubstring);
	CPPUNIT_TEST(Append_CStringAndLength_AppendsString);
	CPPUNIT_TEST(Append_NullPointerAndLength_IgnoresAppend);
	CPPUNIT_TEST(Append_CharAndLength_AppendsMultipleChars);
#ifndef TEST_R5
	//CPPUNIT_TEST(Append_CharAndExcessLength_AppendsMultipleChars);
	CPPUNIT_TEST(Append_CStringAndExcessLength_AppendsString);
#endif
	CPPUNIT_TEST_SUITE_END();

public:
	void PlusEquals_BString_AppendsString()
	{
		BString str1("BASE");
		BString str2("APPENDED");
		str1 += str2;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "BASEAPPENDED"));
	}

	void PlusEquals_CString_AppendsString()
	{
		BString str1("Base");
		str1 += "APPENDED";
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "BaseAPPENDED"));
	}

	void PlusEquals_CStringToEmpty_AppendsString()
	{
		BString str1;
		str1 += "APPENDEDTONOTHING";
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "APPENDEDTONOTHING"));
	}

	void PlusEquals_NullPointer_IgnoresAppend()
	{
		char* tmp = NULL;
		BString str1("Base");
		str1 += tmp;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "Base"));
	}

	void PlusEquals_Char_AppendsChar()
	{
		BString str1("Base");
		str1 += 'C';
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "BaseC"));
	}

	void Append_BString_AppendsString()
	{
		BString str1("BASE");
		BString str2("APPENDED");
		str1.Append(str2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "BASEAPPENDED"));
	}

	void Append_CString_AppendsString()
	{
		BString str1("Base");
		str1.Append("APPENDED");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "BaseAPPENDED"));
	}

	void Append_CStringToEmpty_AppendsString()
	{
		BString str1;
		str1.Append("APPENDEDTONOTHING");
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "APPENDEDTONOTHING"));
	}

	void Append_NullPointer_IgnoresAppend()
	{
		char* tmp = NULL;
		BString str1("Base");
		str1.Append(tmp);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "Base"));
	}

	void Append_BStringAndLength_AppendsSubstring()
	{
		BString str1("BASE");
		BString str2("APPENDED");
		str1.Append(str2, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "BASEAP"));
	}

	void Append_CStringAndLength_AppendsString()
	{
		BString str1("Base");
		str1.Append("APPENDED", 40);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "BaseAPPENDED"));
		CPPUNIT_ASSERT_EQUAL(strlen("BaseAPPENDED"), (unsigned)str1.Length());
	}

	void Append_NullPointerAndLength_IgnoresAppend()
	{
		char* tmp = NULL;
		BString str1("BLABLA");
		str1.Append(tmp, 2);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "BLABLA"));
	}

	void Append_CharAndLength_AppendsMultipleChars()
	{
		BString str1("Base");
		str1.Append('C', 5);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "BaseCCCCC"));
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
	void Append_CharAndExcessLength_AppendsMultipleChars()
	{
		const int32 OUT_OF_MEM_VAL = 2 * 1000 * 1000 * 1000;
		BString str1("Base");
		str1.Append('C', OUT_OF_MEM_VAL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "Base"));
	}

	void Append_CStringAndExcessLength_AppendsString()
	{
		const int32 OUT_OF_MEM_VAL = 2 * 1000 * 1000 * 1000;
		BString str1("Base");
		str1.Append("some more text", OUT_OF_MEM_VAL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(str1.String(), "Basesome more text"));
	}
#endif
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringAppendTest, getTestSuiteName());
