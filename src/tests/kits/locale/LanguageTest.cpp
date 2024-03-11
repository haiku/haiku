/*
 * Copyright 2014-2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include "LanguageTest.h"

#include "Language.h"

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


LanguageTest::LanguageTest()
{
}


LanguageTest::~LanguageTest()
{
}


void
LanguageTest::TestLanguageParseJapanese()
{
	// GIVEN

	// WHEN
	BLanguage language("jp");

	// THEN
	CPPUNIT_ASSERT_EQUAL(BString("jp"), language.ID());
	CPPUNIT_ASSERT_EQUAL(BString("jp"), language.Code());
	CPPUNIT_ASSERT_EQUAL(BString(""), language.ScriptCode());
	CPPUNIT_ASSERT_EQUAL(BString(""), language.CountryCode());
}


void
LanguageTest::TestLanguageParseFrenchWithCountry()
{
	// GIVEN

	// WHEN
	BLanguage language("fr_FR");

	// THEN
	CPPUNIT_ASSERT_EQUAL(BString("fr_FR"), language.ID());
	CPPUNIT_ASSERT_EQUAL(BString("fr"), language.Code());
	CPPUNIT_ASSERT_EQUAL(BString(""), language.ScriptCode());
	CPPUNIT_ASSERT_EQUAL(BString("FR"), language.CountryCode());
}


void
LanguageTest::TestLanguageParseSerbianScriptAndCountry()
{
	// GIVEN

	// WHEN
	BLanguage language("sr_Latn_BA");

	// THEN
	CPPUNIT_ASSERT_EQUAL(BString("sr_Latn_BA"), language.ID());
	CPPUNIT_ASSERT_EQUAL(BString("sr"), language.Code());
	CPPUNIT_ASSERT_EQUAL(BString("BA"), language.CountryCode());
	CPPUNIT_ASSERT_EQUAL(BString("Latn"), language.ScriptCode());
}


void
LanguageTest::TestLanguageParseSerbianScriptAndCountryHyphens()
{
	// GIVEN

	// WHEN
	BLanguage language("sr-Latn-BA");

	// THEN
	CPPUNIT_ASSERT_EQUAL(BString("sr_Latn_BA"), language.ID());
	CPPUNIT_ASSERT_EQUAL(BString("sr"), language.Code());
	CPPUNIT_ASSERT_EQUAL(BString("BA"), language.CountryCode());
	CPPUNIT_ASSERT_EQUAL(BString("Latn"), language.ScriptCode());
}


void
LanguageTest::TestLanguageNameFrenchInEnglish()
{
	// GIVEN
	BLanguage languageFrench("fr_FR");
	BLanguage languageEnglish("en_US");
	BString name;

	// WHEN
	languageFrench.GetName(name, &languageEnglish);
		// get the name of French in English

	// THEN
	CPPUNIT_ASSERT_EQUAL(BString("French (France)"), name);
}


void
LanguageTest::TestLanguageNameFrenchInFrench()
{
	// GIVEN
	BLanguage languageFrench("fr_FR");
	BString name;

	// WHEN
	languageFrench.GetName(name, &languageFrench);
		// get the name of French in French

	// THEN
	CPPUNIT_ASSERT_EQUAL(BString("français (France)"), name);
}


void
LanguageTest::TestLanguagePropertiesFrench()
{
	// GIVEN
	BLanguage language("fr_FR");

	// WHEN

	// THEN
	CPPUNIT_ASSERT_EQUAL(BString("fr"), language.Code());
	CPPUNIT_ASSERT(language.Direction() == B_LEFT_TO_RIGHT);
}


/*static*/ void
LanguageTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("LanguageTest");

	suite.addTest(new CppUnit::TestCaller<LanguageTest>(
		"LanguageTest::TestLanguageParseJapanese",
		&LanguageTest::TestLanguageParseJapanese));
	suite.addTest(new CppUnit::TestCaller<LanguageTest>(
		"LanguageTest::TestLanguageParseFrenchWithCountry",
		&LanguageTest::TestLanguageParseFrenchWithCountry));
	suite.addTest(new CppUnit::TestCaller<LanguageTest>(
		"LanguageTest::TestLanguageParseSerbianScriptAndCountry",
		&LanguageTest::TestLanguageParseSerbianScriptAndCountry));
	suite.addTest(new CppUnit::TestCaller<LanguageTest>(
		"LanguageTest::TestLanguageParseSerbianScriptAndCountryHyphens",
		&LanguageTest::TestLanguageParseSerbianScriptAndCountryHyphens));

	suite.addTest(new CppUnit::TestCaller<LanguageTest>(
		"LanguageTest::TestLanguageNameFrenchInEnglish",
		&LanguageTest::TestLanguageNameFrenchInEnglish));
	suite.addTest(new CppUnit::TestCaller<LanguageTest>(
		"LanguageTest::TestLanguageNameFrenchInFrench",
		&LanguageTest::TestLanguageNameFrenchInFrench));
	suite.addTest(new CppUnit::TestCaller<LanguageTest>(
		"LanguageTest::TestLanguagePropertiesFrench",
		&LanguageTest::TestLanguagePropertiesFrench));

	parent.addTest("LanguageTest", &suite);
}
