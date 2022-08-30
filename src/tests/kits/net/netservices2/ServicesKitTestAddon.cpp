/*
 * Copyright 2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "ExclusiveBorrowTest.h"
#include "HttpProtocolTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("NetServices2Kit");

	ExclusiveBorrowTest::AddTests(*suite);
	HttpProtocolTest::AddTests(*suite);
	HttpIntegrationTest::AddTests(*suite);

	return suite;
}
