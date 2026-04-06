/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "Utility.h"


class UtilityTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(UtilityTest);
	CPPUNIT_TEST(TestTranslatePath);
	CPPUNIT_TEST_SUITE_END();

public:
	void TestTranslatePath()
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
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(UtilityTest, getTestSuiteName());
