/*
 * Copyright 2023-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <string.h>
#include <BufferedDataIO.h>

#include <StorageDefs.h>
#include <SupportDefs.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "BasicTest.h"



class DataIOTest : public BasicTest {
	CPPUNIT_TEST_SUITE(DataIOTest);
	CPPUNIT_TEST(BufferedDataIOTest);
	CPPUNIT_TEST_SUITE_END();

public:
	void BufferedDataIOTest() {
		BMallocIO mallocIO;
		CPPUNIT_ASSERT_EQUAL(B_OK, mallocIO.SetSize(1024));

		BBufferedDataIO bufferedDataIO(mallocIO, 8, false);
		CPPUNIT_ASSERT_EQUAL(B_OK, bufferedDataIO.InitCheck());

		CPPUNIT_ASSERT(bufferedDataIO.Write("test ", 5) == 5);
		CPPUNIT_ASSERT(bufferedDataIO.Write("test ", 5) == 5);
		CPPUNIT_ASSERT(bufferedDataIO.Write("test ", 5) == 5);

		CPPUNIT_ASSERT_EQUAL(B_OK, bufferedDataIO.Flush());

		CPPUNIT_ASSERT(bufferedDataIO.Write("longer-test", 12) == 12);

//		CPPUNIT_ASSERT(mallocIO.Position() == 27);
//		CPPUNIT_ASSERT(memcmp(mallocIO.Buffer(), "test test test longer-test", 27) == 0);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(DataIOTest, getTestSuiteName());
