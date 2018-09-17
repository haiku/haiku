/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "DemangleTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("KernelDebuggerTest");

	DemangleTest::AddTests(*suite);

	return suite;
}

