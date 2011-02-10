/*
 * Copyright 2010-2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "NetworkAddressTest.h"
#include "NetworkInterfaceTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("NetAPI");

	NetworkAddressTest::AddTests(*suite);
	NetworkInterfaceTest::AddTests(*suite);

	return suite;
}
