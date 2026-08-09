/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#include "StorageUtils.h"


class StorageUtilsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(StorageUtilsTest);
	CPPUNIT_TEST(SwapExtensionOnPathComponent);
	CPPUNIT_TEST_SUITE_END();

public:
	void SwapExtensionOnPathComponent()
	{
		const BString input = "/paved/path.tea";

		BString output = StorageUtils::SwapExtensionOnPathComponent(input, "chai");

		const BString expected = "/paved/path.chai";
		CPPUNIT_ASSERT_EQUAL(expected, output);
	}
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(StorageUtilsTest, getTestSuiteName());
