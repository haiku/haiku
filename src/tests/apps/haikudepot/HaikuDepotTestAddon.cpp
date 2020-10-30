/*
 * Copyright 2017-2020, Andrew Lindesay, apl@lindesay.co.nz
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "StandardMetaDataJsonEventListenerTest.h"
#include "DumpExportRepositoryJsonListenerTest.h"
#include "ValidationFailureTest.h"
#include "ValidationUtilsTest.h"
#include "StorageUtilsTest.h"
#include "TarArchiveServiceTest.h"
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
	StorageUtilsTest::AddTests(*suite);
	TarArchiveServiceTest::AddTests(*suite);

	return suite;
}
