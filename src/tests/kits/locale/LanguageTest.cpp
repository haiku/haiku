/*
 * Copyright 2014 Haiku, Inc.
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
LanguageTest::TestLanguage()
{
	BLanguage language("fr_FR");
	BString name;
	language.GetName(name);
	CPPUNIT_ASSERT_EQUAL(BString("français (France)"), name);
	CPPUNIT_ASSERT_EQUAL(BString("fr"), language.Code());
	CPPUNIT_ASSERT(language.Direction() == B_LEFT_TO_RIGHT);
}


/*static*/ void
LanguageTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("LanguageTest");

	suite.addTest(new CppUnit::TestCaller<LanguageTest>(
		"LanguageTest::TestLanguage", &LanguageTest::TestLanguage));

	parent.addTest("LanguageTest", &suite);
}
