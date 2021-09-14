/*
 * Copyright 2021 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "HttpTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("NetServices2Kit");

	HttpTest::AddTests(*suite);

	return suite;
}
