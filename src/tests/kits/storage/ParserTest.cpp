// ParserTest.cpp

#include <ParserTest.h>
#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>
#include <cppunit/TestCaller.h>
#include <sniffer/Parser.h>
#include <Mime.h>
#include <String.h>		// BString
#include <TestUtils.h>

using namespace Sniffer;

// Suite
CppUnit::Test*
ParserTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<ParserTest> TC;
		
	suite->addTest( new TC("Mime Sniffer Parser::Scanner Test",
						   &ParserTest::ScannerTest) );		
	suite->addTest( new TC("Mime Sniffer Parser::Parser Test",
						   &ParserTest::ParserEngineTest) );		
						   
	return suite;
}		

// Scanner Test
void
ParserTest::ScannerTest() {
#if TEST_R5
	Outputf("(no tests actually performed for R5 version)\n");
#else	// TEST_R5


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
#endif	// !TEST_R5
}

// Parser Test
void
ParserTest::ParserEngineTest() {
#if TEST_R5
	Outputf("(no tests actually performed for R5 version)\n");
#else	// TEST_R5
	// test a couple of valid and invalid rules
	struct test_case {
		const char	*rule;
		const char	*error;	// NULL, if valid
	} testCases[] = {
		// valid rules
		{ "1.0 (\"ABCD\")", NULL },
		{ "1.0 ('ABCD')", NULL },
		{ "  1.0 ('ABCD')  ", NULL },
		{ "0.8 [0:3] ('ABCDEFG' | 'abcdefghij')", NULL },
		{ "0.5([10]'ABCD'|[17]'abcd'|[13]'EFGH')", NULL } ,
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
//		{ "1E25 ('ABCD')", NULL },
//		{ "1e25 ('ABCD')", NULL },
		// invalid rules
		{ "0.0 ('')", "Sniffer pattern error: illegal empty pattern" },
		{ "('ABCD')", "Sniffer pattern error: match level expected" },
		{ "[0:3] ('ABCD')", "Sniffer pattern error: match level expected" },
		{ "0.8 [0:3] ( | 'abcdefghij')",
		  "Sniffer pattern error: missing pattern" },
		{ "0.8 [0:3] ('ABCDEFG' | )",
		  "Sniffer pattern error: missing pattern" },
		{ "[0:3] ('ABCD')", "Sniffer pattern error: match level expected" },
		{ "1.0 (ABCD')", "Sniffer pattern error: misplaced single quote" },
		{ "1.0 ('ABCD)", "Sniffer pattern error: unterminated rule" },
		{ "1.0 (ABCD)", "Sniffer pattern error: missing pattern" },
		{ "1.0 (ABCD 'ABCD')", "Sniffer pattern error: missing pattern" },
		{ "1.0 'ABCD')", "Sniffer pattern error: missing pattern" },
		{ "1.0 ('ABCD'", "Sniffer pattern error: unterminated rule" },
		{ "1.0 'ABCD'", "Sniffer pattern error: missing sniff pattern" },
		{ "0.5 [0:3] ('ABCD' | 'abcd' | [13] 'EFGH')", 
		  "Sniffer pattern error: missing pattern" },
		{ "0.5('ABCD'|'abcd'|[13]'EFGH')",
		  "Sniffer pattern error: missing pattern" },
		{ "0.5[0:3]([10]'ABCD'|[17]'abcd'|[13]'EFGH')",
		  "Sniffer pattern error: missing pattern" },
		{ "0.8 [0x10:3] ('ABCDEFG' | 'abcdefghij')",
		  "Sniffer pattern error: pattern offset expected" },
		{ "0.8 [0:A] ('ABCDEFG' | 'abcdefghij')",
		  "Sniffer pattern error: pattern range end expected" },
		{ "0.8 [0:3] ('ABCDEFG' & 'abcdefghij')",
		  "Sniffer pattern error: pattern and mask lengths do not match" },
		{ "0.8 [0:3] ('ABCDEFG' & 'abcdefg' & 'xyzwmno')",
		  "Sniffer pattern error: unterminated rule" },
		{ "0.8 [0:3] (\\g&b & 'a')", "Sniffer pattern error: missing mask" },
		{ "0.8 [0:3] (\\19 & 'a')",
		  "Sniffer pattern error: pattern and mask lengths do not match" },
		{ "0.8 [0:3] (0x345 & 'ab')",
		  "Sniffer pattern error: bad hex literal" },
		{ "0.8 [0:3] (0x3457M & 'abc')",
		  "Sniffer pattern error: expecting '|' or '&'" },
		{ "0.8 [0:3] (0x3457\\7 & 'abc')",
		  "Sniffer pattern error: expecting '|' or '&'" },
		{ "1E-25 ('ABCD')", "Sniffer pattern error: missing pattern" },
	};
	const int testCaseCount = sizeof(testCases) / sizeof(test_case);
	BMimeType type;
	for (int32 i = 0; i < testCaseCount; i++) {
		NextSubTest();
		test_case &testCase = testCases[i];
//		cout << endl << testCase.rule << endl;
		BString parseError;
		status_t error = BMimeType::CheckSnifferRule(testCase.rule,
													 &parseError);
		if (testCase.error == NULL) {
if (error != B_OK)
cout << "error: " << parseError.String() << endl;
			CHK(error == B_OK);
		} else {
			// This really isn't doing a proper test at the moment,
			// but it's more convenient this way while I'm developing...
			if (error) {
				cout << endl << parseError.String() << endl;
				CHK(error == B_BAD_MIME_SNIFFER_RULE);
				CHK(parseError.FindLast(testCase.error) >= 0);
			}
		}
	}

/*
	struct test_case {
		const char *rule;
		int thingThatMakesTheDamnTestNotCrash;
	} testCases[] = {
		{ "1.0", 0 },
		{ "[]", 0 }
	};

	const int testCaseCount = sizeof(testCases) / sizeof(test_case);
	for (int i = 0; i < testCaseCount; i++) {
		cout << i << endl;
		NextSubTest();

		Parser parser;
		Rule rule;
		BString str;
		
		status_t err = RES(parser.Parse(testCases[i].rule, &rule, &str));
		if (err)
			cout << "Error: " << str.String() << endl;;
	}
	cout << "DONE!" << endl;

*/

#endif	// !TEST_R5
}












