/*
 * Copyright 2024-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <algorithm>
#include <String.h>
#include <string.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "LocaleUtils.h"


class LocaleUtilsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(LocaleUtilsTest);
	CPPUNIT_TEST(TestLanguageIsBeforeFalseAfter);
	CPPUNIT_TEST(TestLanguageIsBeforeFalseEqual);
	CPPUNIT_TEST(TestLanguageIsBeforeTrueBefore);
	CPPUNIT_TEST(TestLanguageSorting);
	CPPUNIT_TEST(TestDeriveDefaultLanguageCodeOnly);
	CPPUNIT_TEST(TestDeriveDefaultLanguageSystemDefaultMoreSpecific);
	CPPUNIT_TEST(TestDeriveDefaultLanguageSystemDefaultLessSpecific);
	CPPUNIT_TEST_SUITE_END();

public:
	void TestDeriveDefaultLanguageSystemDefaultLessSpecific()
	{
		std::vector<LanguageRef> languages;
		LocaleUtils::SetForcedSystemDefaultLanguageID("de");

		languages.push_back(LanguageRef(new Language("zh", "Chinese", true), true));
		languages.push_back(LanguageRef(new Language("de_CH", "German (Swiss)", true), true));
		languages.push_back(LanguageRef(new Language("de__1996", "German (1996)", true), true));
		languages.push_back(LanguageRef(new Language("fr", "French", true), true));
		languages.push_back(LanguageRef(new Language("es", "Spanish", true), true));

		std::sort(languages.begin(), languages.end(), IsLanguageRefLess);

		LanguageRef defaultLanguage = LocaleUtils::DeriveDefaultLanguage(languages);

		CPPUNIT_ASSERT_EQUAL(BString("de"), BString(defaultLanguage->Code()));
		CPPUNIT_ASSERT_EQUAL(BString("CH"), BString(defaultLanguage->CountryCode()));
		CPPUNIT_ASSERT(NULL == defaultLanguage->ScriptCode());

		LocaleUtils::SetForcedSystemDefaultLanguageID("");
	}

	void TestDeriveDefaultLanguageSystemDefaultMoreSpecific()
	{
		std::vector<LanguageRef> languages;
		LocaleUtils::SetForcedSystemDefaultLanguageID("de_CH");

		languages.push_back(LanguageRef(new Language("zh", "Chinese", true), true));
		languages.push_back(LanguageRef(new Language("de", "German", true), true));
		languages.push_back(LanguageRef(new Language("fr", "French", true), true));

		std::sort(languages.begin(), languages.end(), IsLanguageRefLess);

		LanguageRef defaultLanguage = LocaleUtils::DeriveDefaultLanguage(languages);

		CPPUNIT_ASSERT_EQUAL(BString("de"), BString(defaultLanguage->Code()));
		CPPUNIT_ASSERT(NULL == defaultLanguage->CountryCode());
		CPPUNIT_ASSERT(NULL == defaultLanguage->ScriptCode());

		LocaleUtils::SetForcedSystemDefaultLanguageID("");
	}

	void TestDeriveDefaultLanguageCodeOnly()
	{
		std::vector<LanguageRef> languages;
		LocaleUtils::SetForcedSystemDefaultLanguageID("de");

		languages.push_back(LanguageRef(new Language("zh", "Chinese", true), true));
		languages.push_back(LanguageRef(new Language("de", "German", true), true));
		languages.push_back(LanguageRef(new Language("fr", "French", true), true));

		std::sort(languages.begin(), languages.end(), IsLanguageRefLess);

		LanguageRef defaultLanguage = LocaleUtils::DeriveDefaultLanguage(languages);

		CPPUNIT_ASSERT_EQUAL(BString("de"), BString(defaultLanguage->Code()));
		CPPUNIT_ASSERT(NULL == defaultLanguage->CountryCode());
		CPPUNIT_ASSERT(NULL == defaultLanguage->ScriptCode());

		LocaleUtils::SetForcedSystemDefaultLanguageID("");
	}

	void TestLanguageSorting()
	{
		std::vector<LanguageRef> languages;

		LanguageRef languageZh = LanguageRef(new Language("zh", "Chinese", true), true);
		LanguageRef languageDeCh = LanguageRef(new Language("de_CH", "German (Swiss)", true), true);
		LanguageRef languageDe1996 = LanguageRef(new Language("de__1996", "German (1996)", true), true);
		LanguageRef languageFr = LanguageRef(new Language("fr", "French", true), true);
		LanguageRef languageEs = LanguageRef(new Language("es", "Spanish", true), true);

		languages.push_back(languageZh);
		languages.push_back(languageDeCh);
		languages.push_back(languageDe1996);
		languages.push_back(languageFr);
		languages.push_back(languageEs);

		std::sort(languages.begin(), languages.end(), IsLanguageRefLess);

		LanguageRef language0 = languages[0];
		LanguageRef language1 = languages[1];
		LanguageRef language2 = languages[2];
		LanguageRef language3 = languages[3];
		LanguageRef language4 = languages[4];

		CPPUNIT_ASSERT_EQUAL(BString("de_CH"), BString(language0->ID()));
		CPPUNIT_ASSERT_EQUAL(BString("de__1996"), BString(language1->ID()));
		CPPUNIT_ASSERT_EQUAL(BString("es"), BString(language2->ID()));
		CPPUNIT_ASSERT_EQUAL(BString("fr"), BString(language3->ID()));
		CPPUNIT_ASSERT_EQUAL(BString("zh"), BString(language4->ID()));

		CPPUNIT_ASSERT_EQUAL(0, language0->Compare(*languageDeCh.Get()));
		CPPUNIT_ASSERT_EQUAL(0, language1->Compare(*languageDe1996.Get()));
		CPPUNIT_ASSERT_EQUAL(0, language2->Compare(*languageEs.Get()));
		CPPUNIT_ASSERT_EQUAL(0, language3->Compare(*languageFr.Get()));
		CPPUNIT_ASSERT_EQUAL(0, language4->Compare(*languageZh.Get()));
	}

	void TestLanguageIsBeforeTrueBefore()
	{
		LanguageRef languageZh = LanguageRef(new Language("zh", "Chinese", true), true);
		LanguageRef languageDeCh = LanguageRef(new Language("de_CH", "German (Swiss)", true), true);

		bool actual = (*(languageDeCh.Get()) < *(languageZh.Get()));

		CPPUNIT_ASSERT_EQUAL(true, actual);
	}

	void TestLanguageIsBeforeFalseEqual()
	{
		LanguageRef languageDeCh = LanguageRef(new Language("de_CH", "German (Swiss)", true), true);

		bool actual = (*(languageDeCh.Get()) < *(languageDeCh.Get()));

		CPPUNIT_ASSERT_EQUAL(false, actual);
	}

	void TestLanguageIsBeforeFalseAfter()
	{
		LanguageRef languageZh = LanguageRef(new Language("zh", "Chinese", true), true);
		LanguageRef languageDeCh = LanguageRef(new Language("de_CH", "German (Swiss)", true), true);

		bool actual = (*(languageZh.Get()) < *(languageDeCh.Get()));

		CPPUNIT_ASSERT_EQUAL(false, actual);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(LocaleUtilsTest, getTestSuiteName());
