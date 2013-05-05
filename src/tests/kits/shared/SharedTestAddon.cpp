/*
 * Copyright 2012, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "NaturalCompareTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("Shared");

	NaturalCompareTest::AddTests(*suite);

	return suite;
}
