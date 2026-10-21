/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <iostream>

#include <DataIO.h>
#include <Mime.h>
#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "sniffer/Data.h"
#include "sniffer/Err.h"
#include "sniffer/Parser.h"
#include "sniffer/Rule.h"


using namespace BPrivate::Storage::Sniffer;


class TokenArrayDeleter
{
	Token** fArray;
	size_t fCount;

public:
	TokenArrayDeleter(Token **array, size_t arraySize)
		:
		fArray(array),
		fCount(arraySize / sizeof(Token*))
	{}

	~TokenArrayDeleter() {
		for(size_t i = 0; i < fCount; i++) {
			delete fArray[i];
		}
	}
};


class MimeSnifferTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(MimeSnifferTest);
#ifndef TEST_R5
	CPPUNIT_TEST(BasicTokens_Scanner_ReturnsExpected);
	CPPUNIT_TEST(Numbers_Scanner_ReturnsExpected);
	CPPUNIT_TEST(Parenthesized_Scanner_ReturnsExpected);
	CPPUNIT_TEST(Ranges_Scanner_ReturnsExpected);
	CPPUNIT_TEST(Escapes_Scanner_ReturnsExpected);
	CPPUNIT_TEST(Hex_Scanner_ReturnsExpected);
	CPPUNIT_TEST(SignedNumbers_Scanner_ReturnsExpected);
	CPPUNIT_TEST(ExtendedFloats_Scanner_ReturnsExpected);
	CPPUNIT_TEST(UberTest_Scanner_ReturnsExpected);
	CPPUNIT_TEST(Dividers_Scanner_ReturnsExpected);
	CPPUNIT_TEST(Ampersand_Scanner_ReturnsExpected);
	CPPUNIT_TEST(CSourceFile_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(CSourceFileIndented_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(CHeaderFile_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(CommentedCSourceFile_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(HtmlFile_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(RockOnly_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(RockThenRoll_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(RollThenRockPadded_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(RollThenRock_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(UpperCaseRockRoll_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(MixedCaseRockRoll_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(MaskMatchFullBytes_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(MaskMatchHighNibble_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(MaskMatchLowBits_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(OctalSecondByteMasked_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(OctalFirstByteMasked_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(OctalBothBytesMatch_Sniffer_ReturnsExpected);
	CPPUNIT_TEST(InvalidPatterns_Parser_ReturnsExpected);
	CPPUNIT_TEST(Null_Parser_ReturnsExpected);
#endif
	CPPUNIT_TEST(ValidRules_Parser_ReturnsExpected);
	CPPUNIT_TEST(InvalidMatchLevel_Parser_ReturnsExpected);
	CPPUNIT_TEST(InvalidPatterns_Parser_ReturnsExpected);
	CPPUNIT_TEST(InvalidUnterminated_Parser_ReturnsExpected);
	CPPUNIT_TEST(InvalidRanges_Parser_ReturnsExpected);
	CPPUNIT_TEST(InvalidConjunctions_Parser_ReturnsExpected);
	CPPUNIT_TEST(InvalidHex_Parser_ReturnsExpected);
	CPPUNIT_TEST_SUITE_END();

	Token* _T(TokenType type) { return new Token(type, -1); }
	StringToken* _S(const std::string& str) { return new StringToken(str, -1); }
	IntToken* _I(int32 value) { return new IntToken(value, -1); }
	FloatToken* _F(double value) { return new FloatToken(value, -1); }

	void _ScannerTestHelper(const char* rule, int tokenCount, Token** tokens)
	{
		TokenStream stream;
		try {
			stream.SetTo(rule);
			CPPUNIT_ASSERT_EQUAL(B_OK, stream.InitCheck());
			for (int j = 0; j < tokenCount; j++) {
				const Token* token = stream.Get();
				CPPUNIT_ASSERT(token);
				CPPUNIT_ASSERT(*token == *(tokens[j]));
			}
			CPPUNIT_ASSERT(stream.IsEmpty());
		} catch (Err* e) {
			CppUnit::Exception* err = new CppUnit::Exception(e->Msg());
			delete e;
			throw *err;
		}
	}

	void _ParserTestHelper(const char* rule, const char* expectedError)
	{
		BString parseError;
		status_t error = BMimeType::CheckSnifferRule(rule, &parseError);
		if (expectedError == NULL) {
			if (error != B_OK) {
				std::cout << std::endl
						<< "This sucker's gonna fail..." << std::endl
						<< "RULE: '" << rule << "'" << std::endl
						<< "ERROR: " << std::endl
						<< parseError.String() << std::endl;
			}
			CPPUNIT_ASSERT_EQUAL(B_OK, error);
		} else {
			CPPUNIT_ASSERT(error == (rule ? B_BAD_MIME_SNIFFER_RULE : B_BAD_VALUE));
			CPPUNIT_ASSERT(parseError.FindLast(expectedError) >= 0);
		}
	}

	void _SnifferTestHelper(const std::string& dataStr, const bool* expected)
	{
		const char* rules[] = {
			// General tests
			"1.0 ('#include')",
			"0.0 [0:32] ('#include')",
			"0.e-230 [0:32] (\\#include | \\#ifndef)",
			".2 ([0:32] \"#include\" | [0] '#define' | [0:200] 'int main(')",
			"1.0 [0:32] ('<html>' | '<head>' | '<body>')",
			// Range tests
			"1.0 [0:9] ('rock')",
			"1.0 ([0:9] 'roll')",
			"1.0 ([0:9] 'rock' | [0:9] 'roll')",
			"1.0 [0:9] ('rock' | 'roll')",
			"1.0 ([0] 'rock')",
			"1.0 ([0] 'rock' | [0:9] 'roll')",
			"1.0 ([9] 'rock' | [10] 'roll')",
			// Mask, octal, and hex tests
			"1.0 (\\xFF\\xFF & '\\xF0\\xF0')",
			"1.0 ('\\33\\34' & \\xFF\\x00)",
			"1.0 (\\33\\34 & \"\\x00\\xFF\")",
			"1.0 (\\xFF & \\x05)",
			// Conjunctions
			"1.0 ([4] 'rock') ([9] 'roll')",
			"1.0 [5] ('roll') [10] ('rock')",
			"1.0 [4] ('rock' | 'roll') ([9] 'rock' | [10] 'roll')",
			// Case insensitivity tests
			"1.0 [4] (-i 'Rock' | 'Roll')",
			"1.0 [9] ('Rock' | -i 'Roll')",
			"1.0 (-i [4] 'Rock' | [9] 'Roll')",
			"1.0 ([9] 'Rock' | -i [4] 'Roll')",
		};
		const int32 ruleCount = sizeof(rules) / sizeof(rules[0]);

		for (int32 i = 0; i < ruleCount; i++) {
			Rule rule;
			BString errorMsg;
			status_t err = parse(rules[i], &rule, &errorMsg);
			CPPUNIT_ASSERT_EQUAL(B_OK, err);
			if (!err) {
				Data data;
				data.from = 0;
				data.buffer = (const uint8*)dataStr.data();
				data.length = dataStr.length();
				bool match = rule.Sniff(data);
				CPPUNIT_ASSERT_EQUAL_MESSAGE(rules[i], expected[i], match);
			}
		}
	}

public:
	void BasicTokens_Scanner_ReturnsExpected()
	{
		Token* t1[] = {_S("Hey"), _T(LeftBracket), _T(RightBracket), _T(Colon)};
		TokenArrayDeleter d1(t1, sizeof(t1));
		_ScannerTestHelper("'Hey'[]:", 4, t1);
	}

	void Numbers_Scanner_ReturnsExpected()
	{
		Token* t1[] = {_I(1)};
		TokenArrayDeleter d1(t1, sizeof(t1));
		_ScannerTestHelper("1", 1, t1);
		Token* t2[] = {_F(1.0)};
		TokenArrayDeleter d2(t2, sizeof(t2));
		_ScannerTestHelper("1.0", 1, t2);
		// priority format variants
		Token* t3[] = {_F(1.2), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d3(t3, sizeof(t3));
		_ScannerTestHelper("1.2 ('ABCD')", 4, t3);
		Token* t4[] = {_F(0.2), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d4(t4, sizeof(t4));
		_ScannerTestHelper(".2 ('ABCD')", 4, t4);
		Token* t5[] = {_F(0.0), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d5(t5, sizeof(t5));
		_ScannerTestHelper("0. ('ABCD')", 4, t5);
	}

	void Parenthesized_Scanner_ReturnsExpected()
	{
		Token* t1[] = {_F(1.0), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d1(t1, sizeof(t1));
		_ScannerTestHelper("1.0 (\"ABCD\")", 4, t1);
		Token* t2[] = {_F(1.0), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d2(t2, sizeof(t2));
		_ScannerTestHelper("1.0 ('ABCD')", 4, t2);
		Token* t3[] = {_F(1.0), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d3(t3, sizeof(t3));
		_ScannerTestHelper("  1.0 ('ABCD')  ", 4, t3);
	}

	void Ranges_Scanner_ReturnsExpected()
	{
		Token* t1[] = {_F(0.8),		_T(LeftBracket),  _I(0),		 _T(Colon),
					   _I(3),		_T(RightBracket), _T(LeftParen), _S("ABCDEFG"),
					   _T(Divider), _S("abcdefghij"), _T(RightParen)};
		TokenArrayDeleter d1(t1, sizeof(t1));
		_ScannerTestHelper("0.8 [0:3] ('ABCDEFG' | 'abcdefghij')", 11, t1);

		Token* t2[] = {_F(0.5),	   _T(LeftParen), _T(LeftBracket), _I(10), _T(RightBracket),
					   _S("ABCD"), _T(Divider),	  _T(LeftBracket), _I(17), _T(RightBracket),
					   _S("abcd"), _T(Divider),	  _T(LeftBracket), _I(13), _T(RightBracket),
					   _S("EFGH"), _T(RightParen)};
		TokenArrayDeleter d2(t2, sizeof(t2));
		_ScannerTestHelper("0.5([10]'ABCD'|[17]'abcd'|[13]'EFGH')", 17, t2);

		// whitespace handling
		Token* t3[] = {_F(0.5),		 _T(LeftBracket), _I(0),		  _T(Colon),	_I(3),
					   _T(RightBracket), _T(LeftParen),  _S("ABCD"), _T(Divider), _S("abcd"),
					   _T(Divider),		 _S("EFGH"),	 _T(RightParen)};
		TokenArrayDeleter d3(t3, sizeof(t3));
		_ScannerTestHelper("0.5  \n   [0:3]  \t ('ABCD' \n | 'abcd' | 'EFGH')", 13, t3);

		Token* t4[] = {_F(0.8),		_T(LeftBracket),  _I(0),		 _T(Colon),
					   _I(3),		_T(RightBracket), _T(LeftParen), _S("ABCDEFG"),
					   _T(Divider), _S("abcdefghij"), _T(RightParen)};
		TokenArrayDeleter d4(t4, sizeof(t4));
		_ScannerTestHelper("0.8 [  0  :  3  ] ('ABCDEFG' | 'abcdefghij')", 11, t4);

		// alternate whitespace around divider-separated ranges
		Token* t5[] = {
			_F(1.0),		  _T(LeftBracket), _I(0),		  _T(Colon),	  _I(3),
			_T(RightBracket), _T(LeftParen),   _S("ABCD"),	  _T(RightParen), _T(Divider),
			_T(LeftBracket),  _I(2),		   _T(Colon),	  _I(4),		  _T(RightBracket),
			_T(LeftParen),	  _S("EFGH"),	   _T(RightParen)};
		_ScannerTestHelper("1.0 [0:3] ('ABCD') | [2:4] ('EFGH')", 18, t5);

		// negative start and descending range
		Token* t6[] = {_F(0.8),		  _T(LeftBracket),	_I(-5),		   _T(Colon),
					   _I(-3),		  _T(RightBracket), _T(LeftParen), _S("abc"),
					   _T(Ampersand), _S("abc"),		_T(RightParen)};
		TokenArrayDeleter d6(t6, sizeof(t6));
		_ScannerTestHelper("0.8 [-5:-3] (\"abc\" & 'abc')", 11, t6);

		Token* t7[] = {_F(0.8),		  _T(LeftBracket),	_I(5),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("abc"),
					   _T(Ampersand), _S("abc"),		_T(RightParen)};
		TokenArrayDeleter d7(t7, sizeof(t7));
		_ScannerTestHelper("0.8 [5:3] (\"abc\" & 'abc')", 11, t7);
	}

	void Dividers_Scanner_ReturnsExpected()
	{
		Token* t1[] = {_F(1.0),		_T(LeftParen), _S("ABCD"), _T(RightParen),
					   _T(Divider), _T(LeftParen), _S("EFGH"), _T(RightParen)};
		TokenArrayDeleter d1(t1, sizeof(t1));
		_ScannerTestHelper("1.0 ('ABCD') | ('EFGH')", 8, t1);
	}

	void Ampersand_Scanner_ReturnsExpected()
	{
		Token* t1[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("ABCDEFG"),
					   _T(Ampersand), _S("abcdefg"),	_T(RightParen)};
		TokenArrayDeleter d1(t1, sizeof(t1));
		_ScannerTestHelper("0.8 [0:3] ('ABCDEFG' & 'abcdefg')", 11, t1);
	}

	void Escapes_Scanner_ReturnsExpected()
	{
		Token* t1[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(4),		  _T(RightBracket), _T(LeftParen), _S("\077Mkj0x34"),
					   _T(Ampersand), _S("abcdefgh"),	_T(RightParen)};
		TokenArrayDeleter d1(t1, sizeof(t1));
		_ScannerTestHelper("0.8 [0:4] (\\077Mkj0x34 & 'abcdefgh')", 11, t1);

		Token* t2[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(4),		  _T(RightBracket), _T(LeftParen), _S("\077Mkj\x34"),
					   _T(Ampersand), _S("abcdefgh"),	_T(RightParen)};
		TokenArrayDeleter d2(t2, sizeof(t2));
		_ScannerTestHelper("0.8 [0:4] (\\077Mkj\\x34 & 'abcdefgh')", 11, t2);

		Token* t3[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("\077034"),
					   _T(Ampersand), _S("abcd"),		_T(RightParen)};
		TokenArrayDeleter d3(t3, sizeof(t3));
		_ScannerTestHelper("0.8 [0:3] (\\077034 & 'abcd')", 11, t3);

		// octal escape variants
		Token* t4[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("\077\034"),
					   _T(Ampersand), _S("ab"),			_T(RightParen)};
		TokenArrayDeleter d4(t4, sizeof(t4));
		_ScannerTestHelper("0.8 [0:3] (\\077\\034 & 'ab')", 11, t4);

		Token* t5[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("\077\034"),
					   _T(Ampersand), _S("ab"),			_T(RightParen)};
		TokenArrayDeleter d5(t5, sizeof(t5));
		_ScannerTestHelper("0.8 [0:3] (\\77\\034 & 'ab')", 11, t5);

		Token* t6[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("\007"),
					   _T(Ampersand), _S("a"),			_T(RightParen)};
		TokenArrayDeleter d6(t6, sizeof(t6));
		_ScannerTestHelper("0.8 [0:3] (\\7 & 'a')", 11, t6);

		Token* t7[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("\017"),
					   _T(Ampersand), _S("a"),			_T(RightParen)};
		TokenArrayDeleter d7(t7, sizeof(t7));
		_ScannerTestHelper("0.8 [0:3] (\"\\17\" & 'a')", 11, t7);
		_ScannerTestHelper("0.8 [0:3] ('\\17' & 'a')", 11, t7);

		// non-numeric escape characters
		Token* t8[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("g"),
					   _T(Ampersand), _S("a"),			_T(RightParen)};
		TokenArrayDeleter d8(t8, sizeof(t8));
		_ScannerTestHelper("0.8 [0:3] (\\g & 'a')", 11, t8);

		Token* t9[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("g"),
					   _T(Ampersand), _S("\b"),			_T(RightParen)};
		TokenArrayDeleter d9(t9, sizeof(t9));
		_ScannerTestHelper("0.8 [0:3] (\\g&\\b)", 11, t9);

		Token* t10[] = {_F(0.8),		   _T(LeftBracket), _I(0),		_T(Colon),
						_I(3),			   _T(RightBracket), _T(LeftParen), _S("g&b"),
						_T(Ampersand),	   _S("abc"),		_T(RightParen)};
		TokenArrayDeleter d10(t10, sizeof(t10));
		_ScannerTestHelper("0.8 [0:3] (\\g\\&b & 'abc')", 11, t10);

		// string edge cases
		Token* t11[] = {_F(0.8),		   _T(LeftBracket), _I(0),		_T(Colon),
						_I(3),			   _T(RightBracket), _T(LeftParen), _S("ab\""),
						_T(Ampersand),	   _S("abc"),		_T(RightParen)};
		TokenArrayDeleter d11(t11, sizeof(t11));
		_ScannerTestHelper("0.8 [0:3] ('ab\"' & 'abc')", 11, t11);
		_ScannerTestHelper("0.8 [0:3] (\"ab\\\"\" & 'abc')", 11, t11);

		Token* t12[] = {_F(0.8),		   _T(LeftBracket), _I(0),		_T(Colon),
						_I(3),			   _T(RightBracket), _T(LeftParen), _S("abA"),
						_T(Ampersand),	   _S("abc"),		_T(RightParen)};
		TokenArrayDeleter d12(t12, sizeof(t12));
		_ScannerTestHelper("0.8 [0:3] (\"ab\\A\" & 'abc')", 11, t12);

		Token* t13[] = {_F(0.8),		   _T(LeftBracket), _I(0),		_T(Colon),
						_I(3),			   _T(RightBracket), _T(LeftParen), _S("ab'"),
						_T(Ampersand),	   _S("abc"),		_T(RightParen)};
		TokenArrayDeleter d13(t13, sizeof(t13));
		_ScannerTestHelper("0.8 [0:3] (\"ab'\" & 'abc')", 11, t13);

		Token* t14[] = {_F(0.8),		   _T(LeftBracket), _I(0),		_T(Colon),
						_I(3),			   _T(RightBracket), _T(LeftParen), _S("ab\\"),
						_T(Ampersand),	   _S("abc"),		_T(RightParen)};
		TokenArrayDeleter d14(t14, sizeof(t14));
		_ScannerTestHelper("0.8 [0:3] (\"ab\\\\\" & 'abc')", 11, t14);
	}

	void Hex_Scanner_ReturnsExpected()
	{
		Token* t1[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("\x34\x57"),
					   _T(Ampersand), _S("ab"),			_T(RightParen)};
		TokenArrayDeleter d1(t1, sizeof(t1));
		_ScannerTestHelper("0.8 [0:3] (0x3457 & 'ab')", 11, t1);

		Token* t2[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("\xA4\xb7"),
					   _T(Ampersand), _S("ab"),			_T(RightParen)};
		TokenArrayDeleter d2(t2, sizeof(t2));
		_ScannerTestHelper("0.8 [0:3] (0xA4b7 & 'ab')", 11, t2);

		// hex escape sequences
		Token* t3[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("\x34\x57"),
					   _T(Ampersand), _S("ab"),			_T(RightParen)};
		TokenArrayDeleter d3(t3, sizeof(t3));
		_ScannerTestHelper("0.8 [0:3] (\\x34\\x57 & 'ab')", 11, t3);

		Token* t4[] = {_F(0.8),		  _T(LeftBracket),	_I(0),		   _T(Colon),
					   _I(3),		  _T(RightBracket), _T(LeftParen), _S("\xA4\xb7"),
					   _T(Ampersand), _S("ab"),			_T(RightParen)};
		TokenArrayDeleter d4(t4, sizeof(t4));
		_ScannerTestHelper("0.8 [0:3] (\\xA4\\xb7 & 'ab')", 11, t4);
		_ScannerTestHelper("0.8 [0:3] (\"\\xA4\\xb7\" & 'ab')", 11, t4);
		_ScannerTestHelper("0.8 [0:3] (\'\\xA4\\xb7\' & 'ab')", 11, t4);
	}

	void SignedNumbers_Scanner_ReturnsExpected()
	{
		Token* t1[] = {_I(-1), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d1(t1, sizeof(t1));
		_ScannerTestHelper("-1 ('ABCD')", 4, t1);
		Token* t2[] = {_I(1), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d2(t2, sizeof(t2));
		_ScannerTestHelper("+1 ('ABCD')", 4, t2);
		Token* t3[] = {_F(-1.0), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d3(t3, sizeof(t3));
		_ScannerTestHelper("-1.0 ('ABCD')", 4, t3);
		Token* t4[] = {_F(1.0), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d4(t4, sizeof(t4));
		_ScannerTestHelper("+1.0 ('ABCD')", 4, t4);
		// negative priority formats with exponents
		Token* t5[] = {_F(-1e25), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d5(t5, sizeof(t5));
		_ScannerTestHelper("-1E25 ('ABCD')", 4, t5);
		_ScannerTestHelper("-1e25 ('ABCD')", 4, t5);
		_ScannerTestHelper("-1E+25 ('ABCD')", 4, t5);
		_ScannerTestHelper("-1e+25 ('ABCD')", 4, t5);
		_ScannerTestHelper("-1E-25 ('ABCD')", 4, t5);
		_ScannerTestHelper("-1e-25 ('ABCD')", 4, t5);
		Token* t6[] = {_F(-0.1e-25), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d6(t6, sizeof(t6));
		_ScannerTestHelper("-.1E-25 ('ABCD')", 4, t6);
		_ScannerTestHelper("-.1e-25 ('ABCD')", 4, t6);
	}

	void ExtendedFloats_Scanner_ReturnsExpected()
	{
		Token* t1[] = {_F(1e25), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d1(t1, sizeof(t1));
		_ScannerTestHelper("1E25 ('ABCD')", 4, t1);
		_ScannerTestHelper("1e25 ('ABCD')", 4, t1);
		_ScannerTestHelper("1E+25 ('ABCD')", 4, t1);
		_ScannerTestHelper("1e+25 ('ABCD')", 4, t1);
		_ScannerTestHelper("+1E25 ('ABCD')", 4, t1);
		_ScannerTestHelper("+1e25 ('ABCD')", 4, t1);
		_ScannerTestHelper("+1E+25 ('ABCD')", 4, t1);
		_ScannerTestHelper("+1e+25 ('ABCD')", 4, t1);
		Token* t2[] = {_F(1e-25), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d2(t2, sizeof(t2));
		_ScannerTestHelper("1e-25 ('ABCD')", 4, t2);
		_ScannerTestHelper("1E-25 ('ABCD')", 4, t2);
		_ScannerTestHelper("+1E-25 ('ABCD')", 4, t2);
		_ScannerTestHelper("+1e-25 ('ABCD')", 4, t2);
		Token* t3[] = {_F(0.1e-25), _T(LeftParen), _S("ABCD"), _T(RightParen)};
		TokenArrayDeleter d3(t3, sizeof(t3));
		_ScannerTestHelper(".1E-25 ('ABCD')", 4, t3);
		_ScannerTestHelper(".1e-25 ('ABCD')", 4, t3);
	}

	void UberTest_Scanner_ReturnsExpected()
	{
		Token* t1[] = {
			_I(0),
			_I(0),
			_I(0),
			_I(1),
			_I(-2),
			_I(3),
			_F(0.0),
			_F(0.0),
			_F(0.0),
			_F(1.0),
			_F(-2.0),
			_F(3.0),
			_F(0.0),
			_F(-0.1),
			_F(0.2),
			_F(1.0),
			_F(-2.1),
			_F(3.2),
			_F(0.0),
			_F(0.0e-1),
			_F(0.0e2),
			_F(1.0e1),
			_F(2.0e-2),
			_F(3.0e3),
			_F(-1.0e1),
			_F(-2.0e-2),
			_F(-3.0e3),
			_F(1.0e1),
			_F(2.0e-2),
			_F(3.0e3),
			_F(0.012345),
			_F(1.23456),
			_T(LeftParen),
			_T(RightParen),
			_T(LeftBracket),
			_T(RightBracket),
			_T(Divider),
			_T(Ampersand),
			_T(Colon),
			_T(CaseInsensitiveFlag),
			_S(std::string(
				"abcxyzABCXYZ_ ( ) [ ] | & : -i \t\n \" ' \012\0\377\x00\x12\xab\xCD\xeF\x1A\xb2 ",
				49)),
			_S(std::string(
				"abcxyzABCXYZ_ ( ) [ ] | & : -i \t\n \" ' \012\0\377\x00\x12\xab\xCD\xeF\x1A\xb2 ",
				49)),
			_S(std::string("\000abc_xyz123\"'\"'456", 18)),
			_S("\241a1"),
			_S("!?\\"),
			_S(std::string("\x00", 1)),
			_S("\x12"),
			_S("\xAB\xCD"),
			_S("\xAB\xCD"),
			_S("\x01\x23\x45\x67\x89\xAB\xCD\xEF\xFE\xDC\xBA")};
		TokenArrayDeleter d1(t1, sizeof(t1));
		_ScannerTestHelper(
			"0 -0 +0 1 -2 +3 0. -0. +0. 1. -2. +3. 0.0 -0.1 +0.2 1.0 -2.1 +3.2 "
			"0.e0 0.e-1 0.e+2 1.e1 2.e-2 3.e+3 -1.e1 -2.e-2 -3.e+3 +1.e1 +2.e-2 +3.e+3 "
			"0.012345 1.23456 ( ) [ ] | & : -i "
			" \"abcxyzABCXYZ_ ( ) [ ] | & : -i \t\n \\\" ' "
			"\\012\\0\\377\\x00\\x12\\xab\\xCD\\xeF\\x1A\\xb2 \" "
			" 'abcxyzABCXYZ_ ( ) [ ] | & : -i \t\n \" \\' "
			"\\012\\0\\377\\x00\\x12\\xab\\xCD\\xeF\\x1A\\xb2 ' "
			" \\000abc_xyz123\"'\"'456 \\xA1a1 \\!\\?\\\\ "
			" 0x00 0x12 0xabCD 0xaBcD 0x0123456789aBcDeFfEdCbA",
			50, t1);
	}

	void ValidRules_Parser_ReturnsExpected()
	{
		_ParserTestHelper("1.0 (\"ABCD\")", NULL);
		_ParserTestHelper("1.0 ('ABCD')", NULL);
		_ParserTestHelper("  1.0 ('ABCD')  ", NULL);
		_ParserTestHelper("0.8 [0:3] ('ABCDEFG' | 'abcdefghij')", NULL);
		_ParserTestHelper("0.5([10]'ABCD'|[17]'abcd'|[13]'EFGH')", NULL);
		_ParserTestHelper("0.5  \n   [0:3]  \t ('ABCD' \n | 'abcd' | 'EFGH')", NULL);
		_ParserTestHelper("0.8 [  0  :  3  ] ('ABCDEFG' | 'abcdefghij')", NULL);
		_ParserTestHelper("0.8 [0:3] ('ABCDEFG' & 'abcdefg')", NULL);
		_ParserTestHelper("0.8 [0:3] (\\077Mkl0x34 & 'abcdefgh')", NULL);
		_ParserTestHelper("0.8 [0:3] (\\077034 & 'abcd')", NULL);
		_ParserTestHelper("0.8 [0:3] (\\077\\034 & 'ab')", NULL);
		_ParserTestHelper("0.8 [0:3] (\\77\\034 & 'ab')", NULL);
		_ParserTestHelper("0.8 [0:3] (\\7 & 'a')", NULL);
		_ParserTestHelper("0.8 [0:3] (\"\\17\" & 'a')", NULL);
		_ParserTestHelper("0.8 [0:3] ('\\17' & 'a')", NULL);
		_ParserTestHelper("0.8 [0:3] (\\g & 'a')", NULL);
		_ParserTestHelper("0.8 [0:3] (\\g&\\b)", NULL);
		_ParserTestHelper("0.8 [0:3] (\\g\\&b & 'abc')", NULL);
		_ParserTestHelper("0.8 [0:3] (0x3457 & 'ab')", NULL);
		_ParserTestHelper("0.8 [0:3] (0xA4b7 & 'ab')", NULL);
		_ParserTestHelper("0.8 [0:3] ('ab\"' & 'abc')", NULL);
		_ParserTestHelper("0.8 [0:3] (\"ab\\\"\" & 'abc')", NULL);
		_ParserTestHelper("0.8 [0:3] (\"ab\\A\" & 'abc')", NULL);
		_ParserTestHelper("0.8 [0:3] (\"ab'\" & 'abc')", NULL);
		_ParserTestHelper("0.8 [0:3] (\"ab\\\\\" & 'abc')", NULL);
		_ParserTestHelper("0.8 [-5:-3] (\"abc\" & 'abc')", NULL);
		_ParserTestHelper("1.0 ('ABCD')", NULL);
		_ParserTestHelper(".2 ('ABCD')", NULL);
		_ParserTestHelper("0. ('ABCD')", NULL);
		_ParserTestHelper("1 ('ABCD')", NULL);
		_ParserTestHelper("+1 ('ABCD')", NULL);
		_ParserTestHelper("+.003e2 ('ABCD')", NULL);
#if !TEST_R5
		_ParserTestHelper("1e-3 ('ABCD')", NULL);
		_ParserTestHelper("-123e-9999999999 ('ABCD')", NULL);
#endif
		_ParserTestHelper("1.0 (\\377)", NULL);
		_ParserTestHelper("1.0 (\\800)", NULL);
	}

	void InvalidMatchLevel_Parser_ReturnsExpected()
	{
		_ParserTestHelper("('ABCD')", "Sniffer pattern error: match level expected");
		_ParserTestHelper("[0:3] ('ABCD')", "Sniffer pattern error: match level expected");
		_ParserTestHelper("0.5('ABCD'|'abcd'|[13]'EFGH')",
						  "Sniffer pattern error: missing pattern");
		_ParserTestHelper("0.5[0:3]([10]'ABCD'|[17]'abcd'|[13]'EFGH')",
						  "Sniffer pattern error: missing pattern");
		_ParserTestHelper("0.8 [0x10:3] ('ABCDEFG' | 'abcdefghij')",
						  "Sniffer pattern error: pattern offset expected");
		_ParserTestHelper("-2", "Sniffer pattern error: invalid priority");
		_ParserTestHelper("+2", "Sniffer pattern error: invalid priority");
	}

	void InvalidPatterns_Parser_ReturnsExpected()
	{
		_ParserTestHelper("0.0 ('')", "Sniffer pattern error: illegal empty pattern");
		_ParserTestHelper("0.8 [0:3] ( | 'abcdefghij')", "Sniffer pattern error: missing pattern");
		_ParserTestHelper("0.8 [0:3] ('ABCDEFG' | )", "Sniffer pattern error: missing pattern");
		_ParserTestHelper("0.5 [0:3] ('ABCD' | 'abcd' | [13] 'EFGH')",
						  "Sniffer pattern error: missing pattern");
		_ParserTestHelper("1.0", "Sniffer pattern error: missing expression");
		_ParserTestHelper("1.0 (ABCD')",
#if TEST_R5
						  "Sniffer pattern error: misplaced single quote"
#else
						  "Sniffer pattern error: invalid character 'A'"
#endif
		);
		_ParserTestHelper("1.0 (ABCD)",
#if TEST_R5
						  "Sniffer pattern error: missing pattern"
#else
						  "Sniffer pattern error: invalid character 'A'"
#endif
		);
		_ParserTestHelper("1.0 (ABCD 'ABCD')",
#if TEST_R5
						  "Sniffer pattern error: missing pattern"
#else
						  "Sniffer pattern error: invalid character 'A'"
#endif
		);
		_ParserTestHelper("1.0 'ABCD')", "Sniffer pattern error: missing pattern");
		_ParserTestHelper("1.0 'ABCD'",
#if TEST_R5
						  "Sniffer pattern error: missing sniff pattern"
#else
						  "Sniffer pattern error: missing pattern"
#endif
		);
	}

	void InvalidUnterminated_Parser_ReturnsExpected()
	{
		_ParserTestHelper("1.0 ('ABCD)",
#if TEST_R5
						  "Sniffer pattern error: unterminated rule"
#else
						  "Sniffer pattern error: unterminated single-quoted string"
#endif
		);
		_ParserTestHelper("1.0 ('ABCD'", "Sniffer pattern error: unterminated rule");
#if !TEST_R5
		_ParserTestHelper("\"blah", "Sniffer pattern error: unterminated double-quoted string");
		_ParserTestHelper("\"Escape!! \\", "Sniffer pattern error: incomplete escape sequence");
		_ParserTestHelper("'Escape!! \\", "Sniffer pattern error: incomplete escape sequence");
#endif
	}

	void InvalidRanges_Parser_ReturnsExpected()
	{
		_ParserTestHelper("0.8 [0:A] ('ABCDEFG' | 'abcdefghij')",
#if TEST_R5
						  "Sniffer pattern error: pattern range end expected"
#else
						  "Sniffer pattern error: invalid character 'A'"
#endif
		);
	}

	void InvalidConjunctions_Parser_ReturnsExpected()
	{
		_ParserTestHelper("0.8 [0:3] ('ABCDEFG' & 'abcdefghij')",
						  "Sniffer pattern error: pattern and mask lengths do not match");
		_ParserTestHelper("0.8 [0:3] ('ABCDEFG' & 'abcdefg' & 'xyzwmno')",
#if TEST_R5
						  "Sniffer pattern error: unterminated rule"
#else
						  "Sniffer pattern error: expecting '|', ')', or possibly '&'"
#endif
		);
		_ParserTestHelper("0.8 [0:3] (\\g&b & 'a')",
#if TEST_R5
						  "Sniffer pattern error: missing mask"
#else
						  "Sniffer pattern error: invalid character 'b'"
#endif
		);
		_ParserTestHelper("0.8 [0:3] (\\19 & 'a')",
						  "Sniffer pattern error: pattern and mask lengths do not match");
	}

	void InvalidHex_Parser_ReturnsExpected()
	{
		_ParserTestHelper("0.8 [0:3] (0x345 & 'ab')", "Sniffer pattern error: bad hex literal");
		_ParserTestHelper("0.8 [0:3] (0x3457M & 'abc')",
#if TEST_R5
						  "Sniffer pattern error: expecting '|' or '&'"
#else
						  "Sniffer pattern error: invalid character 'M'"
#endif
		);
		_ParserTestHelper("0.8 [0:3] (0x3457\\7 & 'abc')",
#if TEST_R5
						  "Sniffer pattern error: expecting '|' or '&'"
#else
						  "Sniffer pattern error: expecting '|', ')', or possibly '&'"
#endif
		);
#if !TEST_R5
		_ParserTestHelper("0xThisIsNotAHexCode", "Sniffer pattern error: incomplete hex code");
		_ParserTestHelper("0xAndNeitherIsThis:-)", "Sniffer pattern error: bad hex literal");
		_ParserTestHelper("\\x", "Sniffer pattern error: incomplete escaped hex code");
		_ParserTestHelper("\\xNotAHexCode", "Sniffer pattern error: incomplete escaped hex code");
		_ParserTestHelper("\\xAlsoNotAHexCode",
						  "Sniffer pattern error: incomplete escaped hex code");
		_ParserTestHelper("\\x0", "Sniffer pattern error: incomplete escaped hex code");
#endif
	}

	void InvalidMiscellaneous_Parser_ReturnsExpected()
	{
		_ParserTestHelper("\x03  ", "Sniffer pattern error: invalid character '\x03'");
		_ParserTestHelper(".NotAFloat", "Sniffer pattern error: incomplete floating point number");
		_ParserTestHelper("-NotANumber", "Sniffer pattern error: incomplete signed number");
		_ParserTestHelper("+NotANumber", "Sniffer pattern error: incomplete signed number");

		_ParserTestHelper(
			"0.0e", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"1.0e", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			".0e", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"0e", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"1e", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"-1e", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"+1e", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"-1.e", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"+1.e", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"-1.0e", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"+1.0e", "Sniffer pattern error: incomplete extended-notation floating point number");

		_ParserTestHelper(
			"0.0e-", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"1.0e-", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			".0e-", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"0e-", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"1e-", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"-1e-", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"+1e-", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"-1.e-", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"+1.e-", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"-1.0e-", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"+1.0e-", "Sniffer pattern error: incomplete extended-notation floating point number");

		_ParserTestHelper(
			"0.0e+", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"1.0e+", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			".0e+", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"0e+", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"1e+", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"-1e+", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"+1e+", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"-1.e+", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"+1.e+", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"-1.0e+", "Sniffer pattern error: incomplete extended-notation floating point number");
		_ParserTestHelper(
			"+1.0e+", "Sniffer pattern error: incomplete extended-notation floating point number");

		_ParserTestHelper("\\11\\", "Sniffer pattern error: incomplete escape sequence");
		_ParserTestHelper(
			"\\400",
			"Sniffer pattern error: invalid octal literal (octals must be between octal 0 and "
			"octal 377 inclusive)");
		_ParserTestHelper(
			"\\777",
			"Sniffer pattern error: invalid octal literal (octals must be between octal 0 and "
			"octal 377 inclusive)");
	}

	void Null_Parser_ReturnsExpected()
	{
		_ParserTestHelper(NULL, "Sniffer pattern error: NULL pattern");
	}


	// Each Sniffer_* case below checks one data sample against all rules;
	// expected[] holds the expected result per rule, in kRules order:
	// 5 general, 7 range, 4 mask/octal/hex, 3 conjunction, and 4
	// case-insensitivity rules.
	void CSourceFile_Sniffer_ReturnsExpected()
	{
		const std::string data
			= "#include <stdio.h>		\n"
			  "#include <stdlib.h>		\n"
			  "						\n"
			  "int main() {			\n"
			  "	return 0;			\n"
			  "}						\n"
			  "\n";
		const bool expected[] = {
			true, true, true, true, false,
			false, false, false, false, false, false, false,
			false, false, false, false,
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void CSourceFileIndented_Sniffer_ReturnsExpected()
	{
		const std::string data
			= "	#include <stdio.h>		\n"
			  "	#include <stdlib.h>		\n"
			  "						\n"
			  "	int main() {			\n"
			  "		return 0;			\n"
			  "	}						\n"
			  "\n";
		const bool expected[] = {
			false, true, true, true, false,
			false, false, false, false, false, false, false,
			false, false, false, false,
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void CHeaderFile_Sniffer_ReturnsExpected()
	{
		const std::string data
			= "#ifndef SOME_TEST_H		\n"
			  "#define SOME_TEST_H		\n"
			  "							\n"
			  "void main();				\n"
			  "							\n"
			  "#endif	// SOME_TEST_H		\n"
			  "							\n";
		const bool expected[] = {
			false, false, true, false, false,
			false, false, false, false, false, false, false,
			false, false, false, false,
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void CommentedCSourceFile_Sniffer_ReturnsExpected()
	{
		const std::string data
			= "//------------------		\n"
			  "// SomeTest.cpp			\n"
			  "//------------------		\n"
			  "#include <stdio.h>		\n"
			  "							\n"
			  "int main() {				\n"
			  "	return 0;				\n"
			  "}							\n"
			  "							\n";
		const bool expected[] = {
			false, false, false, true, false,
			false, false, false, false, false, false, false,
			false, false, false, true,
			//                   ^^^^ <= coincidence
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void HtmlFile_Sniffer_ReturnsExpected()
	{
		const std::string data
			= "<html>						\n"
			  "<body bgcolor='#ffffff'>	\n"
			  "HTML is boring as hell		<br>	\n"
			  "when i write it too much	<br>	\n"
			  "my head starts to swell		<br>	\n"
			  "<br>						\n"
			  "HTML is stupid and dumb		<br>	\n"
			  "running through traffic		<br>	\n"
			  "is ten times as fun			<br>	\n"
			  "</body>						\n"
			  "</html>						\n";
		const bool expected[] = {
			false, false, false, false, true,
			false, false, false, false, false, false, false,
			false, false, false, false,
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void RockOnly_Sniffer_ReturnsExpected()
	{
		const std::string data("	 rock&");
		const bool expected[] = {
			false, false, false, false, false,
			true, false, true, true, false, false, true,
			false, false, false, false,
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void RockThenRoll_Sniffer_ReturnsExpected()
	{
		const std::string data("	rock&roll");
		const bool expected[] = {
			false, false, false, false, false,
			true, true, true, true, false, true, false,
			false, false, false, false,
			true, false, false,
			true, true, true, false
		};
		_SnifferTestHelper(data, expected);
	}

	void RollThenRockPadded_Sniffer_ReturnsExpected()
	{
		const std::string data("	 roll&rock");
		const bool expected[] = {
			false, false, false, false, false,
			false, true, true, true, false, true, false,
			false, false, false, false,
			false, true, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void RollThenRock_Sniffer_ReturnsExpected()
	{
		const std::string data("	roll&rock");
		const bool expected[] = {
			false, false, false, false, false,
			true, true, true, true, false, true, true,
			false, false, false, false,
			false, false, true,
			true, false, false, true
		};
		_SnifferTestHelper(data, expected);
	}

	void UpperCaseRockRoll_Sniffer_ReturnsExpected()
	{
		const std::string data("	ROCK&ROLL");
		const bool expected[] = {
			false, false, false, false, false,
			false, false, false, false, false, false, false,
			false, false, false, false,
			false, false, false,
			true, true, true, false
		};
		_SnifferTestHelper(data, expected);
	}

	void MixedCaseRockRoll_Sniffer_ReturnsExpected()
	{
		const std::string data("	rOlL&RoCk");
		const bool expected[] = {
			false, false, false, false, false,
			false, false, false, false, false, false, false,
			false, false, false, false,
			false, false, false,
			true, false, false, true
		};
		_SnifferTestHelper(data, expected);
	}

	void MaskMatchFullBytes_Sniffer_ReturnsExpected()
	{
		const std::string data("\xFF\xFF	FF FF");
		const bool expected[] = {
			false, false, false, false, false,
			false, false, false, false, false, false, false,
			true, false, false, true,
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void MaskMatchHighNibble_Sniffer_ReturnsExpected()
	{
		const std::string data("\xFA\xFA	FA FA");
		const bool expected[] = {
			false, false, false, false, false,
			false, false, false, false, false, false, false,
			true, false, false, false,
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void MaskMatchLowBits_Sniffer_ReturnsExpected()
	{
		const std::string data("\xAF\xAF	AF AF");
		const bool expected[] = {
			false, false, false, false, false,
			false, false, false, false, false, false, false,
			false, false, false, true,
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void OctalSecondByteMasked_Sniffer_ReturnsExpected()
	{
		const std::string data("\033\000	033 000", 10);
			// Otherwise, it thinks the NULL is the end of the string
		const bool expected[] = {
			false, false, false, false, false,
			false, false, false, false, false, false, false,
			false, true, false, false,
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void OctalFirstByteMasked_Sniffer_ReturnsExpected()
	{
		const std::string data("\000\034	000 034", 10);
			// Otherwise, it thinks the NULL is the end of the string
		const bool expected[] = {
			false, false, false, false, false,
			false, false, false, false, false, false, false,
			false, false, true, false,
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}

	void OctalBothBytesMatch_Sniffer_ReturnsExpected()
	{
		const std::string data("\033\034	033 034");
		const bool expected[] = {
			false, false, false, false, false,
			false, false, false, false, false, false, false,
			false, true, true, false,
			false, false, false,
			false, false, false, false
		};
		_SnifferTestHelper(data, expected);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MimeSnifferTest, getTestSuiteName());
