/*
 * Copyright 2017-2023, Andrew Lindesay, apl@lindesay.co.nz
 * Distributed under the terms of the MIT License.
 */


#include <TestSuite.h>
#include <TestSuiteAddon.h>

#include "StandardMetaDataJsonEventListenerTest.h"
#include "DataIOUtilsTest.h"
#include "DumpExportRepositoryJsonListenerTest.h"
#include "JwtTokenHelperTest.h"
#include "ValidationFailureTest.h"
#include "ValidationUtilsTest.h"
#include "StorageUtilsTest.h"
#include "TarArchiveServiceTest.h"


BTestSuite*
getTestSuite()
{
	BTestSuite* suite = new BTestSuite("HaikuDepot");

	StandardMetaDataJsonEventListenerTest::AddTests(*suite);
	DataIOUtilsTest::AddTests(*suite);
	DumpExportRepositoryJsonListenerTest::AddTests(*suite);
	DumpExportRepositoryJsonListenerTest::AddTests(*suite);
	JwtTokenHelperTest::AddTests(*suite);
	ValidationFailureTest::AddTests(*suite);
	ValidationUtilsTest::AddTests(*suite);
	StorageUtilsTest::AddTests(*suite);
	TarArchiveServiceTest::AddTests(*suite);

	return suite;
}
