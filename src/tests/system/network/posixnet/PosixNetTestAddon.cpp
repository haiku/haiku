/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "GetAddrInfo.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("PosixNet");

	GetAddrInfoTest::AddTests(*suite);

	return suite;
}

