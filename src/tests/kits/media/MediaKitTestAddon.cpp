/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "AreaTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("MediaKit");

	AreaTest::AddTests(*suite);

	return suite;
}

