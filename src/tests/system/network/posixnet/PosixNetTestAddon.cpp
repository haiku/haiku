/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "GetAddrInfo.h"
#include "SocketTests.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("PosixNet");

	GetAddrInfoTest::AddTests(*suite);
	SocketTests::AddTests(*suite);

	return suite;
}
