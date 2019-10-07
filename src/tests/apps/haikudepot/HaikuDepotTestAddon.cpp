/*
 * Copyright 2017-2019, Andrew Lindesay, apl@lindesay.co.nz
 * Distributed under the terms of the MIT License.
 */

#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "StandardMetaDataJsonEventListenerTest.h"
#include "DumpExportRepositoryJsonListenerTest.h"
#include "ValidationFailureTest.h"
#include "ValidationUtilsTest.h"
#include "ListTest.h"

BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("HaikuDepot");

	StandardMetaDataJsonEventListenerTest::AddTests(*suite);
	DumpExportRepositoryJsonListenerTest::AddTests(*suite);
	ValidationFailureTest::AddTests(*suite);
	ValidationUtilsTest::AddTests(*suite);
	ListTest::AddTests(*suite);

	return suite;
}
