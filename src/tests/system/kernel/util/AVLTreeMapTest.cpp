/*
 * Copyright 2003-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuiteAddon.h>
#include <cppunit/extensions/HelperMacros.h>

#include <AVLTreeMap.h>


class AVLTreeMapTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(AVLTreeMapTest);
	CPPUNIT_TEST(Test1);
	CPPUNIT_TEST_SUITE_END();

public:
	void Test1() {}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(AVLTreeMapTest, getTestSuiteName());
