/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "UtilityTest.h"

#include <stdlib.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "Utility.h"


UtilityTest::UtilityTest()
{
}


UtilityTest::~UtilityTest()
{
}


void
UtilityTest::TestTranslatePath()
{
	CPPUNIT_ASSERT_EQUAL(BString("/boot/home/test"),
		Utility::TranslatePath("$HOME/test"));
	CPPUNIT_ASSERT_EQUAL(BString("/boot/home/test"),
		Utility::TranslatePath("${HOME}/test"));
	CPPUNIT_ASSERT_EQUAL(BString("--/boot/home--"),
		Utility::TranslatePath("--${HOME}--"));
	CPPUNIT_ASSERT_EQUAL(BString("$(HOME)/test"),
		Utility::TranslatePath("$(HOME)/test"));
	CPPUNIT_ASSERT_EQUAL(BString("/boot/home/test"),
		Utility::TranslatePath("~/test"));
	CPPUNIT_ASSERT_EQUAL(BString("~baron/test"),
		Utility::TranslatePath("~baron/test"));
	CPPUNIT_ASSERT_EQUAL(BString("/~/test"),
		Utility::TranslatePath("/~/test"));
}


/*static*/ void
UtilityTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("UtilityTest");

	suite.addTest(new CppUnit::TestCaller<UtilityTest>(
		"UtilityTest::TestTranslatePath", &UtilityTest::TestTranslatePath));

	parent.addTest("UtilityTest", &suite);
}
