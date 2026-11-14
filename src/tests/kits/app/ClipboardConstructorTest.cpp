/*
 * Copyright 2002-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <string.h>

#include <Application.h>
#include <Clipboard.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


class ClipboardConstructorTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ClipboardConstructorTest);
	CPPUNIT_TEST(NullName_Constructor_DefaultsToSystem);
	CPPUNIT_TEST(Name_Constructor_ReturnsCorrectName);
	CPPUNIT_TEST_SUITE_END();

public:
	void NullName_Constructor_DefaultsToSystem()
	{
		BApplication app("application/x-vnd.clipboardtest");
	#ifdef TEST_R5
		CPPUNIT_ASSERT(false);
	#endif
		BClipboard clip(NULL);
		CPPUNIT_ASSERT_EQUAL(0, strcmp(clip.Name(), "system"));
	}

	void Name_Constructor_ReturnsCorrectName()
	{
		BApplication app("application/x-vnd.clipboardtest");
		char name[18] = "BClipboard Case 2";
		BClipboard clip(name);

		CPPUNIT_ASSERT_EQUAL(0, strcmp(clip.Name(), name));
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ClipboardConstructorTest, getTestSuiteName());
