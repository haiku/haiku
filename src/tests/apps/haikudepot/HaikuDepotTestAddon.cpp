/*
 * Copyright 2017, Andrew Lindesay, apl@lindesay.co.nz
 * Distributed under the terms of the MIT License.
 */

#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "StandardMetaDataJsonEventListenerTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("HaikuDepot");

	StandardMetaDataJsonEventListenerTest::AddTests(*suite);

	return suite;
}
