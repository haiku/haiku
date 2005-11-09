// MimeSnifferTest.cpp

#include "MimeSnifferTest.h"

#include <cppunit/Test.h>
#include <cppunit/TestSuite.h>
#include <cppunit/TestCaller.h>
#include <sniffer/Rule.h>
#include <sniffer/Parser.h>
#include <DataIO.h>
#include <Mime.h>
#include <String.h>		// BString
#include <TestUtils.h>

#include <stdio.h>
#include <string>

using namespace BPrivate::Storage::Sniffer;

// Suite
CppUnit::Test*
MimeSnifferTest::Suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite();
	typedef CppUnit::TestCaller<MimeSnifferTest> TC;
		
	suite->addTest( new TC("Mime Sniffer::Scanner Test",
						   &MimeSnifferTest::ScannerTest) );		
	suite->addTest( new TC("Mime Sniffer::Parser Test",
						   &MimeSnifferTest::ParserTest) );		
	suite->addTest( new TC("Mime Sniffer::Sniffer Test",
						   &MimeSnifferTest::SnifferTest) );		
						   
	return suite;
}		

// Scanner Test
void
MimeSnifferTest::ScannerTest() {
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
		// Signed integers
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
		},
		// Unsigned extended floats
		{ "1E25 ('ABCD')", 4,
			{	F(1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "1e25 ('ABCD')", 4,
			{	F(1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "1E+25 ('ABCD')", 4,
			{	F(1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "1e+25 ('ABCD')", 4,
			{	F(1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "1E-25 ('ABCD')", 4,
			{	F(1e-25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "1e-25 ('ABCD')", 4,
			{	F(1e-25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		// Positive signed extended floats
		{ "+1E25 ('ABCD')", 4,
			{	F(1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "+1e25 ('ABCD')", 4,
			{	F(1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "+1E+25 ('ABCD')", 4,
			{	F(1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "+1e+25 ('ABCD')", 4,
			{	F(1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "+1E-25 ('ABCD')", 4,
			{	F(1e-25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "+1e-25 ('ABCD')", 4,
			{	F(1e-25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		// Negative signed extended floats
		{ "-1E25 ('ABCD')", 4,
			{	F(-1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "-1e25 ('ABCD')", 4,
			{	F(-1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "-1E+25 ('ABCD')", 4,
			{	F(-1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "-1e+25 ('ABCD')", 4,
			{	F(-1e25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "-1E-25 ('ABCD')", 4,
			{	F(-1e-25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "-1e-25 ('ABCD')", 4,
			{	F(-1e-25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		// Miscellaneous extended floats
		{ ".1E-25 ('ABCD')", 4,
			{	F(0.1e-25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "-.1e-25 ('ABCD')", 4,
			{	F(-0.1e-25),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		// Signed floats
		{ "-1.0 ('ABCD')", 4,
			{	F(-1.0),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		{ "+1.0 ('ABCD')", 4,
			{	F(1.0),
				T(LeftParen),
				S("ABCD"),
				T(RightParen)
			}
		},
		// The uber test
		{ "0 -0 +0 1 -2 +3 0. -0. +0. 1. -2. +3. 0.0 -0.1 +0.2 1.0 -2.1 +3.2 "
		  "0.e0 0.e-1 0.e+2 1.e1 2.e-2 3.e+3 -1.e1 -2.e-2 -3.e+3 +1.e1 +2.e-2 +3.e+3 "
		  "0.012345 1.23456 ( ) [ ] | & : -i "
		  " \"abcxyzABCXYZ_ ( ) [ ] | & : -i \t\n \\\" ' \\012\\0\\377\\x00\\x12\\xab\\xCD\\xeF\\x1A\\xb2 \" "
		  " 'abcxyzABCXYZ_ ( ) [ ] | & : -i \t\n \" \\' \\012\\0\\377\\x00\\x12\\xab\\xCD\\xeF\\x1A\\xb2 ' "
		  " \\000abc_xyz123\"'\"'456 \\xA1a1 \\!\\?\\\\ "
		  " 0x00 0x12 0xabCD 0xaBcD 0x0123456789aBcDeFfEdCbA", 50,
		  	{	I(0), I(0), I(0), I(1), I(-2), I(3), F(0.0), F(0.0), F(0.0),
		  			F(1.0), F(-2.0), F(3.0), F(0.0), F(-0.1), F(0.2), F(1.0), F(-2.1), F(3.2),
		  		F(0.0), F(0.0e-1), F(0.0e2), F(1.0e1), F(2.0e-2), F(3.0e3),
		  			F(-1.0e1), F(-2.0e-2), F(-3.0e3), F(1.0e1), F(2.0e-2), F(3.0e3),
		  		F(0.012345), F(1.23456), T(LeftParen), T(RightParen), T(LeftBracket),
		  			T(RightBracket), T(Divider), T(Ampersand), T(Colon), T(CaseInsensitiveFlag),
		  		S(std::string("abcxyzABCXYZ_ ( ) [ ] | & : -i \t\n \" ' \012\0\377\x00\x12\xab\xCD\xeF\x1A\xb2 ", 49)),
		  		S(std::string("abcxyzABCXYZ_ ( ) [ ] | & : -i \t\n \" ' \012\0\377\x00\x12\xab\xCD\xeF\x1A\xb2 ", 49)),
		  		S(std::string("\000abc_xyz123\"'\"'456", 18)),
		  		S("\241a1"),
		  		S("!?\\"),
		  		S(std::string("\x00", 1)), S("\x12"), S("\xAB\xCD"), S("\xAB\xCD"),
		  			S("\x01\x23\x45\x67\x89\xAB\xCD\xEF\xFE\xDC\xBA")
		  	}
		},
	};
	
// Undefine our nasty macros
#undef T(type)
#undef S(str)
#undef I(val)
#undef F(val)

	const int testCaseCount = sizeof(testCases) / sizeof(test_case);
	for (int i = 0; i < testCaseCount; i++) {
		NextSubTest();
//		cout << endl << testCases[i].rule << endl;		
		TokenStream stream;
		try {
			stream.SetTo(testCases[i].rule);

			CHK(stream.InitCheck() == B_OK);
			for (int j = 0; j < testCases[i].tokenCount; j++) {
				const Token *token = stream.Get();
				CHK(token);
/*
				cout << tokenTypeToString(token->Type()) << endl;

				if (token->Type() == CharacterString) 
					cout << " token1 == " << token->String() << endl;
				if (testCases[i].tokens[j]->Type() == CharacterString)
					cout << " token2 == " << (testCases[i].tokens[j])->String() << endl;
					
				if (token->Type() == CharacterString) 
				{
					const std::string &str = token->String();
					printf("parser: ");
					for (int i = 0; i < str.length(); i++)
						printf("%x ", str[i]);
					printf("\n");
				}
				if (testCases[i].tokens[j]->Type() == CharacterString)
				{
					const std::string &str = (testCases[i].tokens[j])->String();
					printf("tester: ");
					for (int i = 0; i < str.length(); i++)
						printf("%x ", str[i]);
					printf("\n");
				}
	
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
				CHK(*token == *(testCases[i].tokens[j]));
				delete testCases[i].tokens[j];
			}
			CHK(stream.IsEmpty());
		} catch (Err *e) {
			CppUnit::Exception *err = new CppUnit::Exception(e->Msg());
			delete e;
			throw *err;
		}
	}
	
#endif	// !TEST_R5
}

// Parser Test
void
MimeSnifferTest::ParserTest() {
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
// These two rules are accepted by the R5 sniffer checker, but not
// by the parser. Thus, we're not accepting them with either.
//		{ "1.0 ('ABCD') | ('EFGH')", NULL },
//		{ "1.0 [0:3] ('ABCD') | [2:4] ('EFGH')", NULL },
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
// Also accepted by the R5 sniffer but not the R5 parser. We reject.
//		{ "0.8 [5:3] (\"abc\" & 'abc')", NULL },
		{ "1.0 ('ABCD')", NULL },
		{ ".2 ('ABCD')", NULL },
		{ "0. ('ABCD')", NULL },
		{ "1 ('ABCD')", NULL },
		{ "+1 ('ABCD')", NULL },
// We accept extended notation floating point numbers now, but
// not invalid priorities. 
//		{ "1E25 ('ABCD')", NULL },	
//		{ "1e25 ('ABCD')", NULL },
// R5 chokes on this rule :-(
#if !TEST_R5
		{ "1e-3 ('ABCD')", NULL },
#endif
		{ "+.003e2 ('ABCD')", NULL },
// This one too. See how much better our parser is? :-)
#if !TEST_R5
		{ "-123e-9999999999 ('ABCD')", NULL },	// Hooray for the stunning accuracy of floating point ;-)
#endif
		// invalid rules
		{ "0.0 ('')",
			"Sniffer pattern error: illegal empty pattern" },
		{ "('ABCD')",
			"Sniffer pattern error: match level expected" },
		{ "[0:3] ('ABCD')",
			"Sniffer pattern error: match level expected" },
		{ "0.8 [0:3] ( | 'abcdefghij')",
		  "Sniffer pattern error: missing pattern" },
		{ "0.8 [0:3] ('ABCDEFG' | )",
		  "Sniffer pattern error: missing pattern" },
		{ "[0:3] ('ABCD')",
			"Sniffer pattern error: match level expected" },
		{ "1.0 (ABCD')",
#if TEST_R5
			"Sniffer pattern error: misplaced single quote"
#else
			"Sniffer pattern error: invalid character 'A'"
#endif
		},
		{ "1.0 ('ABCD)",
#if TEST_R5
			"Sniffer pattern error: unterminated rule"
#else
			"Sniffer pattern error: unterminated single-quoted string"
#endif
		},
		{ "1.0 (ABCD)",
#if TEST_R5
			"Sniffer pattern error: missing pattern"
#else
			"Sniffer pattern error: invalid character 'A'"
#endif
		},
		{ "1.0 (ABCD 'ABCD')",
#if TEST_R5
			"Sniffer pattern error: missing pattern"
#else
			"Sniffer pattern error: invalid character 'A'"
#endif
		},
		{ "1.0 'ABCD')",
#if TEST_R5
			"Sniffer pattern error: missing pattern"
#else
			"Sniffer pattern error: missing pattern"
#endif
		},
		{ "1.0 ('ABCD'",
			"Sniffer pattern error: unterminated rule" },
		{ "1.0 'ABCD'",
#if TEST_R5
			"Sniffer pattern error: missing sniff pattern"
#else
			"Sniffer pattern error: missing pattern"
#endif
		},
		{ "0.5 [0:3] ('ABCD' | 'abcd' | [13] 'EFGH')", 
		  	"Sniffer pattern error: missing pattern" },
		{ "0.5('ABCD'|'abcd'|[13]'EFGH')",
		  	"Sniffer pattern error: missing pattern" },
		{ "0.5[0:3]([10]'ABCD'|[17]'abcd'|[13]'EFGH')",
		  	"Sniffer pattern error: missing pattern" },
		{ "0.8 [0x10:3] ('ABCDEFG' | 'abcdefghij')",
		  	"Sniffer pattern error: pattern offset expected" },
		{ "0.8 [0:A] ('ABCDEFG' | 'abcdefghij')",
#if TEST_R5
		  	"Sniffer pattern error: pattern range end expected"
#else
			"Sniffer pattern error: invalid character 'A'"
#endif
		},
		{ "0.8 [0:3] ('ABCDEFG' & 'abcdefghij')",
		  	"Sniffer pattern error: pattern and mask lengths do not match" },
		{ "0.8 [0:3] ('ABCDEFG' & 'abcdefg' & 'xyzwmno')",
#if TEST_R5
		  	"Sniffer pattern error: unterminated rule"
#else
			"Sniffer pattern error: expecting '|', ')', or possibly '&'"
#endif
		},
		{ "0.8 [0:3] (\\g&b & 'a')",
#if TEST_R5
			"Sniffer pattern error: missing mask"
#else
			"Sniffer pattern error: invalid character 'b'"
#endif
		},
		{ "0.8 [0:3] (\\19 & 'a')",
		  	"Sniffer pattern error: pattern and mask lengths do not match" },
		{ "0.8 [0:3] (0x345 & 'ab')",
		  	"Sniffer pattern error: bad hex literal" },
		{ "0.8 [0:3] (0x3457M & 'abc')",
#if TEST_R5
		  	"Sniffer pattern error: expecting '|' or '&'"
#else
			"Sniffer pattern error: invalid character 'M'"
#endif
		},
		{ "0.8 [0:3] (0x3457\\7 & 'abc')",
#if TEST_R5
		  	"Sniffer pattern error: expecting '|' or '&'"
#else
			"Sniffer pattern error: expecting '|', ')', or possibly '&'"
#endif
		},
		
		// Miscellaneous tests designed to hit every remaining
		// relevant "throw new Err()" statement in the scanner.
		// R5 versions will come later...
#if !TEST_R5
		{ "\x03  ", "Sniffer pattern error: invalid character '\x03'" },
		{ "\"blah", "Sniffer pattern error: unterminated double-quoted string" },
		{ "0xThisIsNotAHexCode", "Sniffer pattern error: incomplete hex code" },
		{ "0xAndNeitherIsThis:-)", "Sniffer pattern error: bad hex literal" },
		{ ".NotAFloat", "Sniffer pattern error: incomplete floating point number" },
		{ "-NotANumber", "Sniffer pattern error: incomplete signed number" },
		{ "+NotANumber", "Sniffer pattern error: incomplete signed number" },

		{ "0.0e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1.0e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ ".0e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "0e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.0e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.0e", "Sniffer pattern error: incomplete extended-notation floating point number" },
		
		{ "0.0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1.0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ ".0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.0e-", "Sniffer pattern error: incomplete extended-notation floating point number" },
		
		{ "0.0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1.0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ ".0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "1e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "-1.0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },
		{ "+1.0e+", "Sniffer pattern error: incomplete extended-notation floating point number" },

		{ "\\11\\", "Sniffer pattern error: incomplete escape sequence" },
		{ "\"Escape!! \\", "Sniffer pattern error: incomplete escape sequence" },
		{ "'Escape!! \\", "Sniffer pattern error: incomplete escape sequence" },

		{ "\\x", "Sniffer pattern error: incomplete escaped hex code" },
		{ "\\xNotAHexCode", "Sniffer pattern error: incomplete escaped hex code" },		
		{ "\\xAlsoNotAHexCode", "Sniffer pattern error: incomplete escaped hex code" },
		{ "\\x0", "Sniffer pattern error: incomplete escaped hex code" },
		
		{ "1.0 (\\377)", NULL },
		{ "\\400", "Sniffer pattern error: invalid octal literal (octals must be between octal 0 and octal 377 inclusive)" },
		{ "\\777", "Sniffer pattern error: invalid octal literal (octals must be between octal 0 and octal 377 inclusive)" },
		{ "1.0 (\\800)", NULL },
		
		{ NULL, "Sniffer pattern error: NULL pattern" },

		{ "-2", "Sniffer pattern error: invalid priority" },
		{ "+2", "Sniffer pattern error: invalid priority" },
		
		{ "1.0", "Sniffer pattern error: missing expression" },
#endif	// !TEST_R5


//		{ "1E-25 ('ABCD')", "Sniffer pattern error: missing pattern" },
			// I don't currently understand what's wrong with the above rule... R5
			// rejects it though, for some reason.
	};
	const int testCaseCount = sizeof(testCases) / sizeof(test_case);
	BMimeType type;
	for (int32 i = 0; i < testCaseCount; i++) {
//cout << endl << "----------------------------------------------------------------------" << endl;
		NextSubTest();
		test_case &testCase = testCases[i];
//cout << endl << testCase.rule << endl;
		BString parseError;
		status_t error = BMimeType::CheckSnifferRule(testCase.rule,
													 &parseError);
		if (testCase.error == NULL) {
			if (error != B_OK) {
				cout << endl << "This sucker's gonna fail..."
				     << endl << "RULE: '" << testCase.rule << "'"
				     << endl << "ERROR: "
				     << endl << parseError.String()
				     << endl;
			}
			CHK(error == B_OK);
		} else {

//			if (parseError.FindLast(testCase.error) >= 0) {
//				cout << endl << parseError.String(); // << endl;
//				cout << endl << testCase.error << endl;
//			}
//			cout << endl << parseError.String(); // << endl;
/*
			if (parseError.FindLast(testCase.error) >= 0) {
				cout << " -- OKAY" << endl;
			} else {
				cout << " -- NOGO" << endl;
				cout << testCase.error << endl;
			}
*/
if (testCase.rule && error != B_BAD_MIME_SNIFFER_RULE) {
printf("rule: `%s'", testCase.rule);
RES(error);
}
			CHK(error == (testCase.rule ? B_BAD_MIME_SNIFFER_RULE : B_BAD_VALUE));
			CHK(parseError.FindLast(testCase.error) >= 0);
		}
	}
}

void dumpStr(const std::string &string, const char *label = NULL) {
	if (label)
		printf("%s: ", label);
	for (uint i = 0; i < string.length(); i++)
		printf("%x ", string[i]);
	printf("\n");
}


void
MimeSnifferTest::SnifferTest() {
#if TEST_R5
	Outputf("(no tests actually performed for R5 version)\n");
#else	// TEST_R5
	const char *rules[] = {
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
	const int ruleCount = sizeof(rules)/sizeof(char*);
	struct test_case {
		const std::string data;
		const bool	result[ruleCount];
	} tests[] = {

//------------------------------
{
"#include <stdio.h>		\n\
#include <stdlib.h>		\n\
						\n\
int main() {			\n\
	return 0;			\n\
}						\n\
\n\
",	{	true, true, true, true, false,
		false, false, false, false, false, false, false,
		false, false, false, false,
		false, false, false,
		false, false, false, false
	}
},
//------------------------------
{
"	#include <stdio.h>		\n\
	#include <stdlib.h>		\n\
						\n\
	int main() {			\n\
		return 0;			\n\
	}						\n\
\n\
",	{	false, true, true, true, false,
		false, false, false, false, false, false, false,
		false, false, false, false,
		false, false, false,
		false, false, false, false
	}
},
//------------------------------
{
"#ifndef SOME_TEST_H		\n\
#define SOME_TEST_H			\n\
							\n\
void main();				\n\
							\n\
#endif	// SOME_TEST_H		\n\
							\n\
",	{	false, false, true, false, false,
		false, false, false, false, false, false, false,
		false, false, false, false,
		false, false, false,
		false, false, false, false
	}
},
//------------------------------
{
"//------------------		\n\
// SomeTest.cpp				\n\
//------------------		\n\
#include <stdio.h>			\n\
							\n\
int main() {				\n\
	return 0;				\n\
}							\n\
							\n\
",	{	false, false, false, true, false,
		false, false, false, false, false, false, false,
		false, false, false, true,
		//                   ^^^^ <= coincedence 
		false, false, false,
		false, false, false, false
	}
},
//------------------------------
{
"<html>									\n\
<body bgcolor='#ffffff'>				\n\
HTML is boring as hell		<br>		\n\
when i write it too much	<br>		\n\
my head starts to swell		<br>		\n\
<br>									\n\
HTML is stupid and dumb		<br>		\n\
running through traffic		<br>		\n\
is ten times as fun			<br>		\n\
</body>									\n\
</html>									\n\
",	{	false, false, false, false, true,
		false, false, false, false, false, false, false,
		false, false, false, false,
		false, false, false,
		false, false, false, false
	}
},
//---------  <= Ten characters in
{
"     rock&roll",		// 5,10
	{	false, false, false, false, false,
		true, false, true, true, false, false, true,
		false, false, false, false,
		false, false, false,
		false, false, false, false
	}
},
//---------  <= Ten characters in
{
"    rock&roll",		// 4,9
	{ 	false, false, false, false, false,
		true, true, true, true, false, true, false,
		false, false, false, false,
		true, false, false,
		true, true, true, false
	}
},
//---------  <= Ten characters in
{
"     roll&rock",		// 5,10
	{	false, false, false, false, false,
		false, true, true, true, false, true, false,
		false, false, false, false,
		false, true, false,
		false, false, false, false
	}
},
//---------  <= Ten characters in
{
"    roll&rock",		// 4,9
	{ 	false, false, false, false, false,
		true, true, true, true, false, true, true,
		false, false, false, false,
		false, false, true,
		true, true, false, true
	}
},
//---------  <= Ten characters in
{
"    ROCK&ROLL",		// 4,9
	{ 	false, false, false, false, false,
		false, false, false, false, false, false, false,
		false, false, false, false,
		false, false, false,
		true, true, true, false
	}
},
//---------  <= Ten characters in
{
"    rOlL&RoCk",		// 4,9
	{ 	false, false, false, false, false,
		false, false, false, false, false, false, false,
		false, false, false, false,
		false, false, false,
		true, true, false, true
	}
},
//------------------------------
{
"\xFF\xFF	FF FF",
	{	false, false, false, false, false,
		false, false, false, false, false, false, false,
		true, false, false, true,
		false, false, false,
		false, false, false, false
	}
},
//------------------------------
{
"\xFA\xFA	FA FA",
	{	false, false, false, false, false,
		false, false, false, false, false, false, false,
		true, false, false, false,
		false, false, false,
		false, false, false, false
	}
},
//------------------------------
{
"\xAF\xAF	AF AF",
	{	false, false, false, false, false,
		false, false, false, false, false, false, false,
		false, false, false, true,
		false, false, false,
		false, false, false, false
	}
},
//------------------------------
{
std::string("\033\000	033 000", 10),	// Otherwise, it thinks the NULL is the end of the string
	{	false, false, false, false, false,
		false, false, false, false, false, false, false,
		false, true, false, false,
		false, false, false,
		false, false, false, false
	}
},
//------------------------------
{
std::string("\000\034	000 034", 10),	// Otherwise, it thinks the NULL is the end of the string
	{	false, false, false, false, false,
		false, false, false, false, false, false, false,
		false, false, true, false,
		false, false, false,
		false, false, false, false
	}
},
//------------------------------
{
"\033\034	033 034",
	{	false, false, false, false, false,
		false, false, false, false, false, false, false,
		false, true, true, false,
		false, false, false,
		false, false, false, false
	}
},
	};	// tests[]
	const int32 testCount = sizeof(tests)/sizeof(test_case);
	
	for (int i = 0; i < testCount; i++) {
		if (i > 0)
			NextSubTestBlock();
		test_case &test = tests[i];
//		cout << "--------------------------------------------------------------------------------" << endl;
//		cout << test.data << endl;
		
		for (int j = 0; j < ruleCount; j++) {
			NextSubTest();
//			cout << "############################################################" << endl;
//			cout << rules[j] << endl;
//			cout << test.result[j] << endl;
			Rule rule;
			BString errorMsg;
			status_t err = parse(rules[j], &rule, &errorMsg);
//			dumpStr(test.data, "str ");
			if (err) {
//				cout << "PARSE FAILURE!!!" << endl;
//				cout << errorMsg.String() << endl;
			}
			CHK(err == B_OK);
			if (!err) {
				BMallocIO data;
				data.Write(test.data.data(), test.data.length());//strlen(test.data));
				bool match = rule.Sniff(&data);
//				cout << match << endl;
//				cout << "match == " << (match ? "yes" : "no") << ", "
//					 << ((match == test.result[j]) ? "SUCCESS" : "FAILURE") << endl;
				CHK(match == test.result[j]);			
			} 
		}
	}
#endif // !TEST_R5
}
