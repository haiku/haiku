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
