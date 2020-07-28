/*
 * Copyright 2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "StorageUtilsTest.h"

#include <String.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

#include "StorageUtils.h"


StorageUtilsTest::StorageUtilsTest()
{
}


StorageUtilsTest::~StorageUtilsTest()
{
}


void
StorageUtilsTest::TestSwapExtensionOnPathComponent()
{
	const BString input = "/paved/path.tea";

// ----------------------
	BString output = StorageUtils::SwapExtensionOnPathComponent(input, "chai");
// ----------------------

	const BString expected = "/paved/path.chai";
	CPPUNIT_ASSERT_EQUAL(expected, output);
}


/*static*/ void
StorageUtilsTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite("StorageUtilsTest");

	suite.addTest(
		new CppUnit::TestCaller<StorageUtilsTest>(
			"StorageUtilsTest::TestSwapExtensionOnPathComponent",
			&StorageUtilsTest::TestSwapExtensionOnPathComponent));

	parent.addTest("StorageUtilsTest", &suite);
}