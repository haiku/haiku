/*
 * Copyright 2017, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "KPathTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("KernelFS");

	KPathTest::AddTests(*suite);

	return suite;
}
