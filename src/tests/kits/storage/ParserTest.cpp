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
#define T(type) (new Token(type, -1))
#define S(str) (new StringToken(str, -1))
#define I(val) (new IntToken(val, -1))
#define F(val) (new FloatToken(val, -1))

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
		},
		{ "0.5  \n   [0:3]  \t ('ABCD' \n | 'abcd' | 'EFGH')", 13,
			{	F(0.5),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("ABCD"),
				T(Divider),
				S("abcd"),
				T(Divider),
				S("EFGH"),
				T(RightParen)
			}		
		},
		{ "0.8 [  0  :  3  ] ('ABCDEFG' | 'abcdefghij')", 11,
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
		{ "0.8 [0:3] ('ABCDEFG' & 'abcdefg')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("ABCDEFG"),
				T(Ampersand),
				S("abcdefg"),
				T(RightParen)
			}			
		},
		{ "1.0 ('ABCD') | ('EFGH')", 8,
			{	F(1.0),
				T(LeftParen),
				S("ABCD"),
				T(RightParen),
				T(Divider),
				T(LeftParen),
				S("EFGH"),
				T(RightParen)
			}
		},
		{ "1.0 [0:3] ('ABCD') | [2:4] ('EFGH')", 18,
			{	F(1.0),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("ABCD"),
				T(RightParen),
				T(Divider),
				T(LeftBracket),
				I(2),
				T(Colon),
				I(4),
				T(RightBracket),
				T(LeftParen),
				S("EFGH"),
				T(RightParen)
			}
		},
		{ "0.8 [0:4] (\\077Mkj0x34 & 'abcdefgh')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(4),
				T(RightBracket),
				T(LeftParen),
				S("\077Mkj0x34"),
				T(Ampersand),
				S("abcdefgh"),
				T(RightParen)
			}
		},
		{ "0.8 [0:4] (\\077Mkj\\x34 & 'abcdefgh')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(4),
				T(RightBracket),
				T(LeftParen),
				S("\077Mkj\x34"),
				T(Ampersand),
				S("abcdefgh"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\\077034 & 'abcd')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\077034"),
				T(Ampersand),
				S("abcd"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\\077\\034 & 'ab')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\077\034"),
				T(Ampersand),
				S("ab"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\\77\\034 & 'ab')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\077\034"),
				T(Ampersand),
				S("ab"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\\7 & 'a')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\007"),
				T(Ampersand),
				S("a"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\"\\17\" & 'a')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\017"),
				T(Ampersand),
				S("a"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] ('\\17' & 'a')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\017"),
				T(Ampersand),
				S("a"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\\g & 'a')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("g"),
				T(Ampersand),
				S("a"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\\g&\\b)", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("g"),
				T(Ampersand),
				S("\b"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\\g\\&b & 'abc')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("g&b"),
				T(Ampersand),
				S("abc"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (0x3457 & 'ab')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\x34\x57"),
				T(Ampersand),
				S("ab"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\\x34\\x57 & 'ab')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\x34\x57"),
				T(Ampersand),
				S("ab"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (0xA4b7 & 'ab')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\xA4\xb7"),
				T(Ampersand),
				S("ab"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\\xA4\\xb7 & 'ab')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\xA4\xb7"),
				T(Ampersand),
				S("ab"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\"\\xA4\\xb7\" & 'ab')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\xA4\xb7"),
				T(Ampersand),
				S("ab"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\'\\xA4\\xb7\' & 'ab')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("\xA4\xb7"),
				T(Ampersand),
				S("ab"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] ('ab\"' & 'abc')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("ab\""),
				T(Ampersand),
				S("abc"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\"ab\\\"\" & 'abc')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("ab\""),
				T(Ampersand),
				S("abc"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\"ab\\A\" & 'abc')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("abA"),
				T(Ampersand),
				S("abc"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\"ab'\" & 'abc')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("ab'"),
				T(Ampersand),
				S("abc"),
				T(RightParen)
			}
		},
		{ "0.8 [0:3] (\"ab\\\\\" & 'abc')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(0),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("ab\\"),
				T(Ampersand),
				S("abc"),
				T(RightParen)
			}
		},
		{ "0.8 [-5:-3] (\"abc\" & 'abc')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(-5),
				T(Colon),
				I(-3),
				T(RightBracket),
				T(LeftParen),
				S("abc"),
				T(Ampersand),
				S("abc"),
				T(RightParen)
			}
		},
		{ "0.8 [5:3] (\"abc\" & 'abc')", 11,
			{	F(0.8),
				T(LeftBracket),
				I(5),
				T(Colon),
				I(3),
				T(RightBracket),
				T(LeftParen),
				S("abc"),
				T(Ampersand),
				S("abc"),
				T(RightParen)
			}
		},
		{ "1.2 ('ABCD')", 4,
			{	F(1.2),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ ".2 ('ABCD')", 4,
			{	F(0.2),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "0. ('ABCD')", 4,
			{	F(0.0),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "-1 ('ABCD')", 4,
			{	I(-1),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "+1 ('ABCD')", 4,
			{	I(1),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		}
// e notation is no longer supported, due to the fact that it's
// not useful in the context of a priority...
//		{ "1E25 ('ABCD')", NULL },
//		{ "1e25 ('ABCD')", NULL }
	};
	
// Undefine our nasty macros
#undef T(type)
#undef S(str)
#undef I(val)
#undef F(val)

	const int testCaseCount = sizeof(testCases) / sizeof(test_case);
	for (int i = 0; i < testCaseCount; i++) {
//		cout << testCases[i].rule << endl;		
		NextSubTest();
		TokenStream stream;
		try {
			stream.SetTo(testCases[i].rule);
		} catch (Err *e) {
			CppUnit::Exception *err = new CppUnit::Exception(e->Msg());
			delete e;
			throw *err;
		} 
		CHK(stream.InitCheck() == B_OK);
		for (int j = 0; j < testCases[i].tokenCount; j++) {
			Token *token = stream.Get();
			CHK(token);
//			cout << tokenTypeToString(token->Type()) << endl;
/*
			if (token->Type() == CharacterString)
				cout << " token1 == " << token->String() << endl;
			if (testCases[i].tokens[j]->Type() == CharacterString)
				cout << " token2 == " << (testCases[i].tokens[j])->String() << endl;
*/
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
//		cout << endl;
	}
#else	// !TEST_R5
	Outputf("(no tests actually performed for R5 version)\n");
#endif	// !TEST_R5
}

