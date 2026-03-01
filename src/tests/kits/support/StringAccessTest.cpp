/*
 * Copyright 2002-2026, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <String.h>
#include <UTF8.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringAccessTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringAccessTest);
	CPPUNIT_TEST(OperatorBracket_ValidIndex_ReturnsChar);
	CPPUNIT_TEST(SetByteAt_ValidIndex_ModifiesChar);
	CPPUNIT_TEST(ByteAt_ValidAndInvalidIndex_ReturnsCharOrZero);
	CPPUNIT_TEST(CountChars_WithEllipsis_ReturnsCorrectCount);
	CPPUNIT_TEST(CountChars_Ascii_ReturnsCorrectCount);
	CPPUNIT_TEST(CountChars_Combination_ReturnsCorrectCount);
	CPPUNIT_TEST(Access_EmptyString_ReturnsZeros);
	CPPUNIT_TEST(CountChars_InvalidUtf8_ReturnsCorrectCount);
	CPPUNIT_TEST(LockBuffer_WithCapacity_ReturnsValidPointer);
	CPPUNIT_TEST(UnlockBuffer_WithLength_TruncatesString);
	CPPUNIT_TEST(LockBuffer_EmptyString_ReturnsValidPointer);
#ifndef TEST_R5
	CPPUNIT_TEST(LockBuffer_ZeroLength_DoesNotCrash);
#endif
	CPPUNIT_TEST_SUITE_END();

public:
	void OperatorBracket_ValidIndex_ReturnsChar()
	{
		BString string("A simple string");
		CPPUNIT_ASSERT_EQUAL('A', string[0]);
		CPPUNIT_ASSERT_EQUAL(' ', string[1]);
	}

	void SetByteAt_ValidIndex_ModifiesChar()
	{
		BString string("A simple string");
		string.SetByteAt(0, 'a');
		CPPUNIT_ASSERT_EQUAL(0, strcmp(string.String(), "a simple string"));
	}

	void ByteAt_ValidAndInvalidIndex_ReturnsCharOrZero()
	{
		BString string("A simple string");
		CPPUNIT_ASSERT_EQUAL(0, string.ByteAt(-10));
		CPPUNIT_ASSERT_EQUAL(0, string.ByteAt(200));
		CPPUNIT_ASSERT_EQUAL(' ', string.ByteAt(1));
		CPPUNIT_ASSERT_EQUAL('e', string.ByteAt(7));
	}

	void CountChars_WithEllipsis_ReturnsCorrectCount()
	{
		BString string("Something" B_UTF8_ELLIPSIS);
		CPPUNIT_ASSERT_EQUAL(10, string.CountChars());
		CPPUNIT_ASSERT_EQUAL(strlen(string.String()), (unsigned)string.Length());
	}

	void CountChars_Ascii_ReturnsCorrectCount()
	{
		BString string2("ABCD");
		CPPUNIT_ASSERT_EQUAL(4, string2.CountChars());
		CPPUNIT_ASSERT_EQUAL(strlen(string2.String()), (unsigned)string2.Length());
	}

	void CountChars_Combination_ReturnsCorrectCount()
	{
		static char s[64];
		strcpy(s, B_UTF8_ELLIPSIS);
		strcat(s, B_UTF8_SMILING_FACE);
		BString string3(s);
		CPPUNIT_ASSERT_EQUAL(2, string3.CountChars());
		CPPUNIT_ASSERT_EQUAL(strlen(string3.String()), (unsigned)string3.Length());
	}

	void Access_EmptyString_ReturnsZeros()
	{
		BString empty;
		CPPUNIT_ASSERT_EQUAL(0, strcmp(empty.String(), ""));
		CPPUNIT_ASSERT_EQUAL(0, empty.Length());
		CPPUNIT_ASSERT_EQUAL(0, empty.CountChars());
	}

	void CountChars_InvalidUtf8_ReturnsCorrectCount()
	{
		BString invalid("some text with utf8 characters" B_UTF8_ELLIPSIS);
		invalid.Truncate(invalid.Length() - 1);
		CPPUNIT_ASSERT_EQUAL(31, invalid.CountChars());
	}

	void LockBuffer_WithCapacity_ReturnsValidPointer()
	{
		BString locked("a string");
		char* ptrstr = locked.LockBuffer(20);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(ptrstr, "a string"));
		strcat(ptrstr, " to be locked");
		locked.UnlockBuffer();
		CPPUNIT_ASSERT_EQUAL(0, strcmp(ptrstr, "a string to be locked"));
	}

	void UnlockBuffer_WithLength_TruncatesString()
	{
		BString locked2("some text");
		char* ptr = locked2.LockBuffer(3);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(ptr, "some text"));
		locked2.UnlockBuffer(4);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(locked2.String(), "some"));
		CPPUNIT_ASSERT_EQUAL(4, locked2.Length());
	}

	void LockBuffer_EmptyString_ReturnsValidPointer()
	{
		BString emptylocked;
		char* ptr = emptylocked.LockBuffer(10);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(ptr, ""));
		strcat(ptr, "pippo");
		emptylocked.UnlockBuffer();
		CPPUNIT_ASSERT_EQUAL(0, strcmp(emptylocked.String(), "pippo"));
	}

#ifndef TEST_R5
	void LockBuffer_ZeroLength_DoesNotCrash()
	{
		BString crashesR5;
		crashesR5.LockBuffer(0);
		crashesR5.UnlockBuffer(-1);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(crashesR5.String(), ""));
	}
#endif
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringAccessTest, getTestSuiteName());
