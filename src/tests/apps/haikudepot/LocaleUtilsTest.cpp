/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LocaleUtilsTest.h"

#include <algorithm>

#include <String.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <string.h>

#include "LocaleUtils.h"


LocaleUtilsTest::LocaleUtilsTest()
{
}


LocaleUtilsTest::~LocaleUtilsTest()
{
}


void
LocaleUtilsTest::TestLanguageIsBeforeFalseAfter()
{
	LanguageRef languageZh = LanguageRef(new Language("zh", "Chinese", true), true);
	LanguageRef languageDeCh = LanguageRef(new Language("de_CH", "German (Swiss)", true), true);

	// ----------------------
	bool actual = languageZh < languageDeCh;
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(false, actual);
}


void
LocaleUtilsTest::TestLanguageIsBeforeFalseEqual()
{
	LanguageRef languageDeCh = LanguageRef(new Language("de_CH", "German (Swiss)", true), true);

	// ----------------------
	bool actual = languageDeCh < languageDeCh;
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(false, actual);
}


void
LocaleUtilsTest::TestLanguageIsBeforeTrueBefore()
{
	LanguageRef languageZh = LanguageRef(new Language("zh", "Chinese", true), true);
	LanguageRef languageDeCh = LanguageRef(new Language("de_CH", "German (Swiss)", true), true);

	// ----------------------
	bool actual = languageDeCh < languageZh;
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(true, actual);
}


void
LocaleUtilsTest::TestLanguageSorting()
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

	// ----------------------
	LanguageRef language0 = languages[0];
	LanguageRef language1 = languages[1];
	LanguageRef language2 = languages[2];
	LanguageRef language3 = languages[3];
	LanguageRef language4 = languages[4];
	// ----------------------

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


void
LocaleUtilsTest::TestDeriveDefaultLanguageCodeOnly()
{
	std::vector<LanguageRef> languages;
	LocaleUtils::SetForcedSystemDefaultLanguageID("de");

	languages.push_back(LanguageRef(new Language("zh", "Chinese", true), true));
	languages.push_back(LanguageRef(new Language("de", "German", true), true));
	languages.push_back(LanguageRef(new Language("fr", "French", true), true));

	std::sort(languages.begin(), languages.end(), IsLanguageRefLess);

	// ----------------------
	LanguageRef defaultLanguage = LocaleUtils::DeriveDefaultLanguage(languages);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(BString("de"), BString(defaultLanguage->Code()));
	CPPUNIT_ASSERT(NULL == defaultLanguage->CountryCode());
	CPPUNIT_ASSERT(NULL == defaultLanguage->ScriptCode());

	LocaleUtils::SetForcedSystemDefaultLanguageID("");
}


/*! This test has a more specific system default language selecting a less
	specific supported language.
*/
void
LocaleUtilsTest::TestDeriveDefaultLanguageSystemDefaultMoreSpecific()
{
	std::vector<LanguageRef> languages;
	LocaleUtils::SetForcedSystemDefaultLanguageID("de_CH");

	languages.push_back(LanguageRef(new Language("zh", "Chinese", true), true));
	languages.push_back(LanguageRef(new Language("de", "German", true), true));
	languages.push_back(LanguageRef(new Language("fr", "French", true), true));

	std::sort(languages.begin(), languages.end(), IsLanguageRefLess);

	// ----------------------
	LanguageRef defaultLanguage = LocaleUtils::DeriveDefaultLanguage(languages);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(BString("de"), BString(defaultLanguage->Code()));
	CPPUNIT_ASSERT(NULL == defaultLanguage->CountryCode());
	CPPUNIT_ASSERT(NULL == defaultLanguage->ScriptCode());

	LocaleUtils::SetForcedSystemDefaultLanguageID("");
}


/*! This test has a less specific system default language selecting a more
	specific supported language.
*/
void
LocaleUtilsTest::TestDeriveDefaultLanguageSystemDefaultLessSpecific()
{
	std::vector<LanguageRef> languages;
	LocaleUtils::SetForcedSystemDefaultLanguageID("de");

	languages.push_back(LanguageRef(new Language("zh", "Chinese", true), true));
	languages.push_back(LanguageRef(new Language("de_CH", "German (Swiss)", true), true));
	languages.push_back(LanguageRef(new Language("de__1996", "German (1996)", true), true));
	languages.push_back(LanguageRef(new Language("fr", "French", true), true));
	languages.push_back(LanguageRef(new Language("es", "Spanish", true), true));

	std::sort(languages.begin(), languages.end(), IsLanguageRefLess);

	// ----------------------
	LanguageRef defaultLanguage = LocaleUtils::DeriveDefaultLanguage(languages);
	// ----------------------

	CPPUNIT_ASSERT_EQUAL(BString("de"), BString(defaultLanguage->Code()));
	CPPUNIT_ASSERT_EQUAL(BString("CH"), BString(defaultLanguage->CountryCode()));
	CPPUNIT_ASSERT(NULL == defaultLanguage->ScriptCode());

	LocaleUtils::SetForcedSystemDefaultLanguageID("");
}


/*static*/ void
LocaleUtilsTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("LocaleUtilsTest");

	suite.addTest(
		new CppUnit::TestCaller<LocaleUtilsTest>("LocaleUtilsTest::TestLanguageIsBeforeFalseAfter",
			&LocaleUtilsTest::TestLanguageIsBeforeFalseAfter));

	suite.addTest(
		new CppUnit::TestCaller<LocaleUtilsTest>("LocaleUtilsTest::TestLanguageIsBeforeFalseEqual",
			&LocaleUtilsTest::TestLanguageIsBeforeFalseEqual));

	suite.addTest(
		new CppUnit::TestCaller<LocaleUtilsTest>("LocaleUtilsTest::TestLanguageIsBeforeTrueBefore",
			&LocaleUtilsTest::TestLanguageIsBeforeTrueBefore));

	suite.addTest(new CppUnit::TestCaller<LocaleUtilsTest>("LocaleUtilsTest::TestLanguageSorting",
		&LocaleUtilsTest::TestLanguageSorting));

	suite.addTest(new CppUnit::TestCaller<LocaleUtilsTest>(
		"LocaleUtilsTest::TestDeriveDefaultLanguageCodeOnly",
		&LocaleUtilsTest::TestDeriveDefaultLanguageCodeOnly));

	suite.addTest(new CppUnit::TestCaller<LocaleUtilsTest>(
		"LocaleUtilsTest::TestDeriveDefaultLanguageSystemDefaultMoreSpecific",
		&LocaleUtilsTest::TestDeriveDefaultLanguageSystemDefaultMoreSpecific));

	suite.addTest(new CppUnit::TestCaller<LocaleUtilsTest>(
		"LocaleUtilsTest::TestDeriveDefaultLanguageSystemDefaultLessSpecific",
		&LocaleUtilsTest::TestDeriveDefaultLanguageSystemDefaultLessSpecific));

	parent.addTest("LocaleUtilsTest", &suite);
}
