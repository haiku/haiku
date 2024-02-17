/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LanguageModelTest.h"

#include <String.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include <string.h>

#include "LanguageModel.h"


LanguageModelTest::LanguageModelTest()
{
}


LanguageModelTest::~LanguageModelTest()
{
}


void
LanguageModelTest::TestSetPreferredLanguageCodeOnly()
{
	LanguageModel* model = new LanguageModel("de");
		// forced system default

	model->AddSupportedLanguage(new Language("zh", "Chinese", true));
	model->AddSupportedLanguage(new Language("de", "German", true));
	model->AddSupportedLanguage(new Language("fr", "French", true));

// ----------------------
	model->SetPreferredLanguageToSystemDefault();
// ----------------------

	LanguageRef preferredLanguage = model->PreferredLanguage();

	CPPUNIT_ASSERT_EQUAL(BString("de"), BString(preferredLanguage->Code()));
	CPPUNIT_ASSERT(NULL == preferredLanguage->CountryCode());
	CPPUNIT_ASSERT(NULL == preferredLanguage->ScriptCode());
}


/*! This test has a more specific system default language selecting a less
	specific supported language.
*/
void
LanguageModelTest::TestSetPreferredLanguageSystemDefaultMoreSpecific()
{
	LanguageModel* model = new LanguageModel("de_CH");
		// forced system default

	model->AddSupportedLanguage(new Language("zh", "Chinese", true));
	model->AddSupportedLanguage(new Language("de", "German", true));
	model->AddSupportedLanguage(new Language("fr", "French", true));

// ----------------------
	model->SetPreferredLanguageToSystemDefault();
// ----------------------

	LanguageRef preferredLanguage = model->PreferredLanguage();

	CPPUNIT_ASSERT_EQUAL(BString("de"), BString(preferredLanguage->Code()));
	CPPUNIT_ASSERT(NULL == preferredLanguage->CountryCode());
	CPPUNIT_ASSERT(NULL == preferredLanguage->ScriptCode());
}


/*! This test has a less specific system default language selecting a more
	specific supported language.
*/
void
LanguageModelTest::TestSetPreferredLanguageSystemDefaultLessSpecific()
{
	LanguageModel* model = new LanguageModel("de");
		// forced system default

	model->AddSupportedLanguage(new Language("zh", "Chinese", true));
	model->AddSupportedLanguage(new Language("de_CH", "German (Swiss)", true));
	model->AddSupportedLanguage(new Language("de__1996", "German (1996)", true));
	model->AddSupportedLanguage(new Language("fr", "French", true));
	model->AddSupportedLanguage(new Language("es", "Spanish", true));

// ----------------------
	model->SetPreferredLanguageToSystemDefault();
// ----------------------

	LanguageRef preferredLanguage = model->PreferredLanguage();

	CPPUNIT_ASSERT_EQUAL(BString("de"), BString(preferredLanguage->Code()));
	CPPUNIT_ASSERT_EQUAL(BString("CH"), BString(preferredLanguage->CountryCode()));
	CPPUNIT_ASSERT(NULL == preferredLanguage->ScriptCode());
}


/*static*/ void
LanguageModelTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("LanguageModelTest");

	suite.addTest(
		new CppUnit::TestCaller<LanguageModelTest>(
			"LanguageModelTest::TestSetPreferredLanguageCodeOnly",
			&LanguageModelTest::TestSetPreferredLanguageCodeOnly));

	suite.addTest(
		new CppUnit::TestCaller<LanguageModelTest>(
			"LanguageModelTest::TestSetPreferredLanguageSystemDefaultMoreSpecific",
			&LanguageModelTest::TestSetPreferredLanguageSystemDefaultMoreSpecific));

	suite.addTest(
		new CppUnit::TestCaller<LanguageModelTest>(
			"LanguageModelTest::TestSetPreferredLanguageSystemDefaultLessSpecific",
			&LanguageModelTest::TestSetPreferredLanguageSystemDefaultLessSpecific));

	parent.addTest("LanguageModelTest", &suite);
}