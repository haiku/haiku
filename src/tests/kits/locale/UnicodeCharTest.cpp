/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "UnicodeCharTest.h"

#include <String.h>
#include <UnicodeChar.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


UnicodeCharTest::UnicodeCharTest()
{
}


UnicodeCharTest::~UnicodeCharTest()
{
}


void
UnicodeCharTest::TestAscii()
{
	Result results[] = {
		{"\x1e", 0, 0, 0, 0, 1, 15, '\x1e', '\x1e'},
		{"\x1f", 0, 0, 0, 0, 1, 15, '\x1f', '\x1f'},
		{" ", 0, 0, 0, 0, 1, 12, ' ', ' '},
		{"!", 0, 0, 0, 0, 1, 23, '!', '!'},
		{"\"", 0, 0, 0, 0, 1, 23, '"', '"'},
		{"#", 0, 0, 0, 0, 1, 23, '#', '#'},
		{"$", 0, 0, 0, 0, 1, 25, '$', '$'},
		{"%", 0, 0, 0, 0, 1, 23, '%', '%'},
		{"&", 0, 0, 0, 0, 1, 23, '&', '&'},
		{"'", 0, 0, 0, 0, 1, 23, '\'', '\''},
		{"(", 0, 0, 0, 0, 1, 20, '(', '('},
		{")", 0, 0, 0, 0, 1, 21, ')', ')'},
		{"*", 0, 0, 0, 0, 1, 23, '*', '*'},
		{"+", 0, 0, 0, 0, 1, 24, '+', '+'},
		{",", 0, 0, 0, 0, 1, 23, ',', ','},
		{"-", 0, 0, 0, 0, 1, 19, '-', '-'},
		{".", 0, 0, 0, 0, 1, 23, '.', '.'},
		{"/", 0, 0, 0, 0, 1, 23, '/', '/'},
		{"0", 0, 1, 0, 0, 1, 9, '0', '0'},
		{"1", 0, 1, 0, 0, 1, 9, '1', '1'},
		{"2", 0, 1, 0, 0, 1, 9, '2', '2'},
		{"3", 0, 1, 0, 0, 1, 9, '3', '3'},
		{"4", 0, 1, 0, 0, 1, 9, '4', '4'},
		{"5", 0, 1, 0, 0, 1, 9, '5', '5'},
		{"6", 0, 1, 0, 0, 1, 9, '6', '6'},
		{"7", 0, 1, 0, 0, 1, 9, '7', '7'},
		{"8", 0, 1, 0, 0, 1, 9, '8', '8'},
		{"9", 0, 1, 0, 0, 1, 9, '9', '9'},
		{":", 0, 0, 0, 0, 1, 23, ':', ':'},
		{";", 0, 0, 0, 0, 1, 23, ';', ';'},
		{"<", 0, 0, 0, 0, 1, 24, '<', '<'},
		{"=", 0, 0, 0, 0, 1, 24, '=', '='},
		{">", 0, 0, 0, 0, 1, 24, '>', '>'},
		{"?", 0, 0, 0, 0, 1, 23, '?', '?'},
		{"@", 0, 0, 0, 0, 1, 23, '@', '@'},
		{"A", 1, 1, 0, 1, 1, 1, 'A', 'a'},
		{"B", 1, 1, 0, 1, 1, 1, 'B', 'b'},
		{"C", 1, 1, 0, 1, 1, 1, 'C', 'c'},
		{"D", 1, 1, 0, 1, 1, 1, 'D', 'd'},
		{"E", 1, 1, 0, 1, 1, 1, 'E', 'e'}
	};

	for (int32 i = 30; i < 70; i++) {
		NextSubTest();
		_TestChar(i, results[i - 30]);
	}
}


void
UnicodeCharTest::TestISO8859()
{
	uint32 chars[] = {(uint8)'\xe4', (uint8)'\xd6', (uint8)'\xdf',
		(uint8)'\xe8', (uint8)'\xe1', (uint8)'\xe9', 0};

	Result results[] = {
		{"ä", 1, 1, 1, 0, 1, 2, 196, 228},
		{"Ö", 1, 1, 0, 1, 1, 1, 214, 246},
		{"ß", 1, 1, 1, 0, 1, 2, 223, 223},
		{"è", 1, 1, 1, 0, 1, 2, 200, 232},
		{"á", 1, 1, 1, 0, 1, 2, 193, 225},
		{"é", 1, 1, 1, 0, 1, 2, 201, 233}
	};

	for(int i = 0; chars[i] != 0; i++) {
		NextSubTest();
		_TestChar(chars[i], results[i]);
	}
}


void
UnicodeCharTest::TestUTF8()
{
	const char *utf8chars[] = {"à", "ß", "ñ", "é", "ç", "ä", NULL};

	Result results[] = {
		{"à", 1, 1, 1, 0, 1, 2, 192, 224},
		{"ß", 1, 1, 1, 0, 1, 2, 223, 223},
		{"ñ", 1, 1, 1, 0, 1, 2, 209, 241},
		{"é", 1, 1, 1, 0, 1, 2, 201, 233},
		{"ç", 1, 1, 1, 0, 1, 2, 199, 231},
		{"ä", 1, 1, 1, 0, 1, 2, 196, 228}
	};

	for(int i = 0; utf8chars[i] != 0; i++) {
		NextSubTest();
		_TestChar(BUnicodeChar::FromUTF8(utf8chars[i]), results[i]);
	}
}


/*static*/ void
UnicodeCharTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("UnicodeCharTest");

	suite.addTest(new CppUnit::TestCaller<UnicodeCharTest>(
		"UnicodeCharTest::TestAscii", &UnicodeCharTest::TestAscii));
	suite.addTest(new CppUnit::TestCaller<UnicodeCharTest>(
		"UnicodeCharTest::TestISO8859", &UnicodeCharTest::TestISO8859));
	suite.addTest(new CppUnit::TestCaller<UnicodeCharTest>(
		"UnicodeCharTest::TestUTF8", &UnicodeCharTest::TestUTF8));

	parent.addTest("UnicodeCharTest", &suite);
}


void
UnicodeCharTest::_ToString(uint32 c, char *text)
{
	BUnicodeChar::ToUTF8(c, &text);
	text[0] = '\0';
}


void
UnicodeCharTest::_TestChar(uint32 i, Result result)
{
	char text[16];

	_ToString(i, text);
	CPPUNIT_ASSERT_EQUAL(BString(result.value), text);
	CPPUNIT_ASSERT_EQUAL(result.isAlpha, BUnicodeChar::IsAlpha(i));
	CPPUNIT_ASSERT_EQUAL(result.isAlNum, BUnicodeChar::IsAlNum(i));
	CPPUNIT_ASSERT_EQUAL(result.isLower, BUnicodeChar::IsLower(i));
	CPPUNIT_ASSERT_EQUAL(result.isUpper, BUnicodeChar::IsUpper(i));
	CPPUNIT_ASSERT_EQUAL(result.isDefined, BUnicodeChar::IsDefined(i));
	CPPUNIT_ASSERT_EQUAL(result.type, BUnicodeChar::Type(i));
	CPPUNIT_ASSERT_EQUAL(result.toUpper, BUnicodeChar::ToUpper(i));
	CPPUNIT_ASSERT_EQUAL(result.toLower, BUnicodeChar::ToLower(i));
}
