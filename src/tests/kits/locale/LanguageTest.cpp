/*
 * Copyright 2014-2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <Language.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class LanguageTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(LanguageTest);
	CPPUNIT_TEST(Japanese_Properties_ReturnsExpected);
	CPPUNIT_TEST(FrenchInFrance_Properties_ReturnsExpected);
	CPPUNIT_TEST(SerbianInBosniaWithLatinScript_Properties_ReturnsExpected);
	CPPUNIT_TEST(SerbianInBosniaWithLatinScriptAndHyphenSeparator_Properties_ReturnsExpected);
	CPPUNIT_TEST(FrenchInEnglish_Name_ReturnsFrenchFrance);
	CPPUNIT_TEST(FrenchInFrench_Name_ReturnsFrancaisFrance);
	CPPUNIT_TEST_SUITE_END();

public:
	void Japanese_Properties_ReturnsExpected()
	{
		BLanguage language("jp");

		CPPUNIT_ASSERT_EQUAL(BString("jp"), language.ID());
		CPPUNIT_ASSERT_EQUAL(BString("jp"), language.Code());
		CPPUNIT_ASSERT_EQUAL(BString(""), language.ScriptCode());
		CPPUNIT_ASSERT_EQUAL(BString(""), language.CountryCode());
	}

	void FrenchInFrance_Properties_ReturnsExpected()
	{
		BLanguage language("fr_FR");

		CPPUNIT_ASSERT_EQUAL(BString("fr_FR"), language.ID());
		CPPUNIT_ASSERT_EQUAL(BString("fr"), language.Code());
		CPPUNIT_ASSERT_EQUAL(BString(""), language.ScriptCode());
		CPPUNIT_ASSERT_EQUAL(BString("FR"), language.CountryCode());
		CPPUNIT_ASSERT(language.Direction() == B_LEFT_TO_RIGHT);
	}

	void SerbianInBosniaWithLatinScript_Properties_ReturnsExpected()
	{
		BLanguage language("sr_Latn_BA");

		CPPUNIT_ASSERT_EQUAL(BString("sr_Latn_BA"), language.ID());
		CPPUNIT_ASSERT_EQUAL(BString("sr"), language.Code());
		CPPUNIT_ASSERT_EQUAL(BString("BA"), language.CountryCode());
		CPPUNIT_ASSERT_EQUAL(BString("Latn"), language.ScriptCode());
	}

	void SerbianInBosniaWithLatinScriptAndHyphenSeparator_Properties_ReturnsExpected()
	{
		BLanguage language("sr-Latn-BA");

		CPPUNIT_ASSERT_EQUAL(BString("sr_Latn_BA"), language.ID());
		CPPUNIT_ASSERT_EQUAL(BString("sr"), language.Code());
		CPPUNIT_ASSERT_EQUAL(BString("BA"), language.CountryCode());
		CPPUNIT_ASSERT_EQUAL(BString("Latn"), language.ScriptCode());
	}

	void FrenchInEnglish_Name_ReturnsFrenchFrance()
	{
		BLanguage languageFrench("fr_FR");
		BLanguage languageEnglish("en_US");

		BString name;
		languageFrench.GetName(name, &languageEnglish);

		CPPUNIT_ASSERT_EQUAL(BString("French (France)"), name);
	}

	void FrenchInFrench_Name_ReturnsFrancaisFrance()
	{
		BLanguage languageFrench("fr_FR");

		BString name;
		languageFrench.GetName(name, &languageFrench);

		CPPUNIT_ASSERT_EQUAL(BString("français (France)"), name);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(LanguageTest, getTestSuiteName());
