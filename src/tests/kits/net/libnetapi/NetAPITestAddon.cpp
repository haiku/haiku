/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "NetworkAddressTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("NetAPI");

	NetworkAddressTest::AddTests(*suite);

	return suite;
}
