// ParserTest.cpp

#include <ParserTest.h>
#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>
#include <cppunit/TestCaller.h>
#include <sniffer/Parser.h>
#include <TestUtils.h>

// Suite
CppUnit::Test*
ParserTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<ParserTest> TC;
		
	suite->addTest( new TC("Mime Sniffer Parser::Scanner Test",
						   &ParserTest::ScannerTest) );		
						   
	return suite;
}		

// ParserRuleTest
void
ParserTest::ScannerTest() {
#if !TEST_R5

	using namespace Sniffer;

	// tests:
	// Internal TokenStream and CharStream classes

// Define some useful macros for dynamically allocating
// various Token classes
#define T(type) (new Token(type))
#define S(str) (new StringToken(str))
#define I(val) (new IntToken(val))
#define F(val) (new FloatToken(val))

	struct test_case {
		const char *rule;
		int tokenCount;
		Token *tokens[256];
	} testCases[] = {
		{ "'Hey'[]:", 4,
			{	S("Hey"),
				T(LeftBracket),
				T(RightBracket),
				T(Colon)
			}
		},		
		{ "1", 1, { I(1) } },
		{ "1.0", 1, { F(1.0) } },

		{ "1.0 (\"ABCD\")", 4, { F(1.0), T(LeftParen), S("ABCD"), T(RightParen) } },
		{ "1.0 ('ABCD')", 4, { F(1.0), T(LeftParen), S("ABCD"), T(RightParen) } },
		{ "  1.0 ('ABCD')  ", 4, { F(1.0), T(LeftParen), S("ABCD"), T(RightParen) } },
		{ "0.8 [0:3] ('ABCDEFG' | 'abcdefghij')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("ABCDEFG"),
				T(Divider),
				S("abcdefghij"),
				T(RightParen)
			}
		},
		{ "0.5([10]'ABCD'|[17]'abcd'|[13]'EFGH')", 17,
			{	F(0.5),
				T(LeftParen),
				T(LeftBracket),
				I(10),
				T(RightBracket),
				S("ABCD"),
				T(Divider),
				T(LeftBracket),
				I(17),
				T(RightBracket),
				S("abcd"),
				T(Divider),
				T(LeftBracket),
				I(13),
				T(RightBracket),
				S("EFGH"),
				T(RightParen)
			}		
		} /*,
		{ "0.5  \n   [0:3]  \t ('ABCD' \n | 'abcd' | 'EFGH')", NULL },
		{ "0.8 [  0  :  3  ] ('ABCDEFG' | 'abcdefghij')", NULL },
		{ "0.8 [0:3] ('ABCDEFG' & 'abcdefg')", NULL },
		{ "1.0 ('ABCD') | ('EFGH')", NULL },
		{ "1.0 [0:3] ('ABCD') | [2:4] ('EFGH')", NULL },
		{ "0.8 [0:3] (\\077Mkl0x34 & 'abcdefgh')", NULL },
		{ "0.8 [0:3] (\\077034 & 'abcd')", NULL },
		{ "0.8 [0:3] (\\077\\034 & 'ab')", NULL },
		{ "0.8 [0:3] (\\77\\034 & 'ab')", NULL },
		{ "0.8 [0:3] (\\7 & 'a')", NULL },
		{ "0.8 [0:3] (\"\\17\" & 'a')", NULL },
		{ "0.8 [0:3] ('\\17' & 'a')", NULL },
		{ "0.8 [0:3] (\\g & 'a')", NULL },
		{ "0.8 [0:3] (\\g&\\b)", NULL },
		{ "0.8 [0:3] (\\g\\&b & 'abc')", NULL },
		{ "0.8 [0:3] (0x3457 & 'ab')", NULL },
		{ "0.8 [0:3] (0xA4b7 & 'ab')", NULL },
		{ "0.8 [0:3] ('ab\"' & 'abc')", NULL },
		{ "0.8 [0:3] (\"ab\\\"\" & 'abc')", NULL },
		{ "0.8 [0:3] (\"ab\\A\" & 'abc')", NULL },
		{ "0.8 [0:3] (\"ab'\" & 'abc')", NULL },
		{ "0.8 [0:3] (\"ab\\\\\" & 'abc')", NULL },
		{ "0.8 [-5:-3] (\"abc\" & 'abc')", NULL },
		{ "0.8 [5:3] (\"abc\" & 'abc')", NULL },
		{ "1.2 ('ABCD')", NULL },
		{ ".2 ('ABCD')", NULL },
		{ "0. ('ABCD')", NULL },
		{ "-1 ('ABCD')", NULL },
		{ "+1 ('ABCD')", NULL },
		{ "1E25 ('ABCD')", NULL },
		{ "1e25 ('ABCD')", NULL },*/
	};
	
// Undefine our nasty macros
#undef T(type)
#undef S(str)
#undef I(val)
#undef F(val)

	const int testCaseCount = sizeof(testCases) / sizeof(test_case);
	for (int i = 0; i < testCaseCount; i++) {
		NextSubTest();
		TokenStream stream(testCases[i].rule);
		CHK(stream.InitCheck() == B_OK);
		for (int j = 0; j < testCases[i].tokenCount; j++) {
			Token *token = stream.Get();
			CHK(token);
//			cout << tokenTypeToString(token->Type()) << endl;
			CHK(*token == *(testCases[i].tokens[j]));
/*
			switch (token->Type()) {
				case CharacterString:
					cout << " string == " << token->String() << endl;
					break;
				case Integer:
					cout << " int == " << token->Int() << endl;
					break;
				case FloatingPoint:
					cout << " float == " << token->Float() << endl;
					break;
			}
*/
			delete testCases[i].tokens[j];
		}
		CHK(stream.IsEmpty());
	}
//
#else	// !TEST_R5
	Outputf("(no tests actually performed for R5 version)\n");
#endif	// !TEST_R5
}

