/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "MessageFormatTest.h"

#include <Locale.h>
#include <MessageFormat.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


MessageFormatTest::MessageFormatTest()
{
}


MessageFormatTest::~MessageFormatTest()
{
}


void
MessageFormatTest::TestFormat()
{
	BString output;

	struct Test {
		const char* locale;
		const char* pattern;
		int32 number;
		const char* expected;
	};

	static const char* polishTemplate = "{0, plural, one{Wybrano # obiekt} "
		"few{Wybrano # obiekty} many{Wybrano # obiektów} "
		"other{Wybrano # obyektu}}";

	// There are 4 rules in russian: one (1, 21, ...), few (2-4, 22-24, ...),
	// many (anything else), and other (non-integer numbers). When formatting
	// integers only, either both many and other must be there (with other
	// not being used), or one/few/other must be used.
	static const char* russianTemplate = "{0, plural, one{# объект} "
		"few{# объекта} other{# объектов}}";

	static const Test tests[] = {
		{"en_US", "A QA engineer walks into a bar.", 0,
			"A QA engineer walks into a bar."},
		{"en_US", "Orders {0, plural, one{# beer} other{# beers}}.", 1,
			"Orders 1 beer."},
		{"en_US", "Orders {0, plural, one{# beer} other{# beers}}.", 0,
			"Orders 0 beers."},
		{"en_US", "Orders {0, plural, one{# beer} other{# beers}}.", 99999999,
			"Orders 99,999,999 beers."},
		{"en_US", "Orders {0, plural, one{# beer} other{# beers}}.", -INT_MAX,
			"Orders -2,147,483,647 beers."},
		{"en_US", "Orders {0, plural, one{# beer} other{# beers}}.", -1,
			"Orders -1 beer."},
		{"en_US", "Orders {0, plural, one{a lizard} other{more lizards}}.", 1,
			"Orders a lizard."},
		{"en_US", "Orders {0, plural, one{a lizard} other{more lizards}}.", 2,
			"Orders more lizards."},
		{"en_US", "Orders {0, plural, one{# \x8A} other{# \x02}}.", 2,
			"Orders 2 \x02."},
		{"fr_FR", "Commande {0, plural, one{# bière} other{# bières}}.",
			99999999, "Commande 99 999 999 bières."},
		{"pl_PL", polishTemplate, 1, "Wybrano 1 obiekt"},
		{"pl_PL", polishTemplate, 3, "Wybrano 3 obiekty"},
		{"pl_PL", polishTemplate, 5, "Wybrano 5 obiektów"},
		{"pl_PL", polishTemplate, 23, "Wybrano 23 obiekty"},
		{"ru_RU", russianTemplate, 1, "1 объект"},
		{"ru_RU", russianTemplate, 2, "2 объекта"},
		{"ru_RU", russianTemplate, 5, "5 объектов"},
		{NULL, NULL, 0, NULL}
	};

	for (int i = 0; tests[i].pattern != NULL; i++) {
		status_t result;
		NextSubTest();
		output.Truncate(0);
		BLanguage language(tests[i].locale);
		BMessageFormat formatter(language, tests[i].pattern);

		result = formatter.Format(output, tests[i].number);
		CPPUNIT_ASSERT_EQUAL(B_OK, result);
		CPPUNIT_ASSERT_EQUAL(BString(tests[i].expected), output);
	}
}


void
MessageFormatTest::TestBogus()
{
	struct Test {
		const char* pattern;
	};

	static const Test tests[] = {
		{ "{0, plural, one{# dog} other{# dogs}" }, // Missing closing brace
		{ "{0, plural, one{# dog}, other{# dogs}}" }, // Extra comma
		{ "{0, plural, one{# dog}" }, // Missing "other"
		//{ "{4099, plural, one{# dog} other{# dogs}}" }, // Out of bounds arg
		{ "{0, invalid, one{# dog} other{# dogs}}" }, // Invalid rule
		{ NULL }
	};

	for (int i = 0; tests[i].pattern != NULL; i++) {
		NextSubTest();

		status_t result;
		BString output;

		BMessageFormat formatter(tests[i].pattern);
		CPPUNIT_ASSERT(formatter.InitCheck() != B_OK);

		result = formatter.Format(output, 1);
		CPPUNIT_ASSERT(result != B_OK);

		result = formatter.Format(output, 2);
		CPPUNIT_ASSERT(result != B_OK);
	}
}


/*static*/ void
MessageFormatTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("MessageFormatTest");

	suite.addTest(new CppUnit::TestCaller<MessageFormatTest>(
		"MessageFormatTest::TestFormat", &MessageFormatTest::TestFormat));
	suite.addTest(new CppUnit::TestCaller<MessageFormatTest>(
		"MessageFormatTest::TestBogus", &MessageFormatTest::TestBogus));

	parent.addTest("MessageFormatTest", &suite);
}
