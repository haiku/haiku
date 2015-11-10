/*
 * Copyright 2015 Julian Harnath <julian.harnath@rwth-aachen.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "SimpleTransformTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("AppServerUnitTests");

	SimpleTransformTest::AddTests(*suite);

	return suite;
}
