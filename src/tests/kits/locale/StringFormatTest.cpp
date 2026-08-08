/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <Locale.h>
#include <StringFormat.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class StringFormatTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StringFormatTest);
	CPPUNIT_TEST(EnUSLocale_TemplateWithNoPlural_NoPlural);
	CPPUNIT_TEST(EnUSLocale_TemplateWithPlural_Returns1Beer);
	CPPUNIT_TEST(EnUSLocale_TemplateWithPlural_Returns0Beers);
	CPPUNIT_TEST(EnUSLocale_TemplateWithPlural_ReturnsManyBeers);
	CPPUNIT_TEST(EnUSLocale_TemplateWithPlural_ReturnsNegativeBeers);
	CPPUNIT_TEST(EnUSLocale_TemplateWithPlural_ReturnsNegative1Beer);
	CPPUNIT_TEST(EnUSLocale_TemplateWithPluralAndNoPlaceholder_ReturnsALizard);
	CPPUNIT_TEST(EnUSLocale_TemplateWithPluralAndNoPlaceholder_ReturnsMoreLizards);
	CPPUNIT_TEST(EnUSLocale_TemplateWithPluralAndHex_ReturnsHex);
	CPPUNIT_TEST(FrFRLocale_TemplateWithPlural_ReturnsManyBeers);
	CPPUNIT_TEST(PlPLLocale_TemplateWithPlural_Returns1Obiekt);
	CPPUNIT_TEST(PlPLLocale_TemplateWithPlural_Returns3Obiekty);
	CPPUNIT_TEST(PlPLLocale_TemplateWithPlural_Returns5Obiektow);
	CPPUNIT_TEST(PlPLLocale_TemplateWithPlural_Returns23Obiekty);
	CPPUNIT_TEST(RuRULocale_TemplateWithPlural_Returns1Obiekt);
	CPPUNIT_TEST(RuRULocale_TemplateWithPlural_Returns2Obiekta);
	CPPUNIT_TEST(RuRULocale_TemplateWithPlural_Returns5Obiektow);
	CPPUNIT_TEST(InvalidTemplate_MissingClosingBrace_ReturnsNotOK);
	CPPUNIT_TEST(InvalidTemplate_ExtraComma_ReturnsNotOK);
	CPPUNIT_TEST(InvalidTemplate_MissingOther_ReturnsNotOK);
	CPPUNIT_TEST(InvalidTemplate_InvalidRule_ReturnsNotOK);
	CPPUNIT_TEST_SUITE_END();

	static const char* kEnglishTemplate;
	static const char* kPolishTemplate;
	static const char* kRussianTemplate;

	void _Template(const char* locale, const char* pattern, int32 number, const char* expected)
	{
		BString output;
		BLanguage language(locale);
		BStringFormat formatter(language, pattern);

		CPPUNIT_ASSERT_EQUAL(B_OK, formatter.Format(output, number));
		CPPUNIT_ASSERT_EQUAL(BString(expected), output);
	}

	void _Invalid(const char* pattern)
	{
		BString output;
		BStringFormat formatter(pattern);

		CPPUNIT_ASSERT(formatter.InitCheck() != B_OK);
		CPPUNIT_ASSERT(formatter.Format(output, 1) != B_OK);
		CPPUNIT_ASSERT(formatter.Format(output, 2) != B_OK);
	}

public:
	void EnUSLocale_TemplateWithNoPlural_NoPlural()
	{
		_Template("en_US", "A QA engineer walks into a bar.", 0,
			"A QA engineer walks into a bar.");
	}

	void EnUSLocale_TemplateWithPlural_Returns1Beer()
	{
		_Template("en_US", kEnglishTemplate, 1, "Orders 1 beer.");
	}

	void EnUSLocale_TemplateWithPlural_Returns0Beers()
	{
		_Template("en_US", kEnglishTemplate, 0, "Orders 0 beers.");
	}

	void EnUSLocale_TemplateWithPlural_ReturnsManyBeers()
	{
		_Template("en_US", kEnglishTemplate, 99999999, "Orders 99,999,999 beers.");
	}

	void EnUSLocale_TemplateWithPlural_ReturnsNegativeBeers()
	{
		_Template("en_US", kEnglishTemplate, -INT_MAX, "Orders -2,147,483,647 beers.");
	}

	void EnUSLocale_TemplateWithPlural_ReturnsNegative1Beer()
	{
		_Template("en_US", kEnglishTemplate, -1, "Orders -1 beer.");
	}

	void EnUSLocale_TemplateWithPluralAndNoPlaceholder_ReturnsALizard()
	{
		_Template("en_US", "Orders {0, plural, one{a lizard} other{more lizards}}.", 1,
			"Orders a lizard.");
	}

	void EnUSLocale_TemplateWithPluralAndNoPlaceholder_ReturnsMoreLizards()
	{
		_Template("en_US", "Orders {0, plural, one{a lizard} other{more lizards}}.", 2,
			"Orders more lizards.");
	}

	void EnUSLocale_TemplateWithPluralAndHex_ReturnsHex()
	{
		_Template("en_US", "Orders {0, plural, one{# \x8A} other{# \x02}}.", 2, "Orders 2 \x02.");
	}

	void FrFRLocale_TemplateWithPlural_ReturnsManyBeers()
	{
		_Template("fr_FR", "Commande {0, plural, one{# bière} other{# bières}}.",
			99999999, "Commande 99\xe2\x80\xaf""999\xe2\x80\xaf""999 bières.");
	}

	void PlPLLocale_TemplateWithPlural_Returns1Obiekt() {
		_Template("pl_PL", kPolishTemplate, 1, "Wybrano 1 obiekt");
	}

	void PlPLLocale_TemplateWithPlural_Returns3Obiekty() {
		_Template("pl_PL", kPolishTemplate, 3, "Wybrano 3 obiekty");
	}

	void PlPLLocale_TemplateWithPlural_Returns5Obiektow() {
		_Template("pl_PL", kPolishTemplate, 5, "Wybrano 5 obiektów");
	}

	void PlPLLocale_TemplateWithPlural_Returns23Obiekty() {
		_Template("pl_PL", kPolishTemplate, 23, "Wybrano 23 obiekty");
	}

	void RuRULocale_TemplateWithPlural_Returns1Obiekt() { _Template("ru_RU", kRussianTemplate, 1, "1 объект"); }
	void RuRULocale_TemplateWithPlural_Returns2Obiekta() { _Template("ru_RU", kRussianTemplate, 2, "2 объекта"); }
	void RuRULocale_TemplateWithPlural_Returns5Obiektow() { _Template("ru_RU", kRussianTemplate, 5, "5 объектов"); }

	void InvalidTemplate_MissingClosingBrace_ReturnsNotOK() { _Invalid("{0, plural, one{# dog} other{# dogs}"); }
	void InvalidTemplate_ExtraComma_ReturnsNotOK() { _Invalid("{0, plural, one{# dog}, other{# dogs}}"); }
	void InvalidTemplate_MissingOther_ReturnsNotOK() { _Invalid("{0, plural, one{# dog}"); }
	void InvalidTemplate_InvalidRule_ReturnsNotOK() { _Invalid("{0, invalid, one{# dog} other{# dogs}}"); }
};


const char* StringFormatTest::kEnglishTemplate = "Orders {0, plural, one{# beer} other{# beers}}.";

const char* StringFormatTest::kPolishTemplate = "{0, plural, one{Wybrano # obiekt} "
	"few{Wybrano # obiekty} many{Wybrano # obiektów} "
	"other{Wybrano # obiektu}}";

const char* StringFormatTest::kRussianTemplate = "{0, plural, one{# объект} "
	"few{# объекта} other{# объектов}}";


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StringFormatTest, getTestSuiteName());
